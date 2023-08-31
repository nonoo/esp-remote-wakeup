#include "main.h"
#include "led.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

static const char *TAG = "wifi";

#define APP_WIFI_STA_NAME "wifi"

static esp_netif_t *wifi_sta_netif = NULL;
static SemaphoreHandle_t wifi_semph_get_ip4_addrs = NULL;
static SemaphoreHandle_t wifi_semph_get_ip6_addrs = NULL;

static esp_err_t wifi_connect(void);

static void wifi_handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ESP_LOGW(TAG, "wi-fi disconnected, trying to reconnect...");
	led_handle_wifi_disconnected();
	esp_err_t err = esp_wifi_connect();
	if (err == ESP_ERR_WIFI_NOT_STARTED)
		return;
	ESP_ERROR_CHECK(err);
}

static void wifi_handler_on_wifi_connect(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	esp_netif_create_ip6_linklocal(esp_netif);
}

static bool wifi_is_our_netif(esp_netif_t *esp_netif) {
	return (strncmp(APP_WIFI_STA_NAME, esp_netif_get_desc(esp_netif), strlen(APP_WIFI_STA_NAME) - 1) == 0);
}

static void wifi_handler_on_sta_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
	if (!wifi_is_our_netif(event->esp_netif))
		return;

	ESP_LOGI(TAG, "got ipv4 event: interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
	xSemaphoreGive(wifi_semph_get_ip4_addrs);
	led_handle_wifi_connected();
}

static void wifi_handler_on_sta_got_ipv6(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
	if (!wifi_is_our_netif(event->esp_netif))
		return;

	const char *ipv6_addr_types_to_str[6] = {
		"ESP_IP6_ADDR_IS_UNKNOWN",
		"ESP_IP6_ADDR_IS_GLOBAL",
		"ESP_IP6_ADDR_IS_LINK_LOCAL",
		"ESP_IP6_ADDR_IS_SITE_LOCAL",
		"ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
		"ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
	};

	esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
	ESP_LOGI(TAG, "got ipv6 event: interface \"%s\" address: " IPV6STR ", type: %s", esp_netif_get_desc(event->esp_netif),
		IPV62STR(event->ip6_info.ip), ipv6_addr_types_to_str[ipv6_type]);

	if (ipv6_type == ESP_IP6_ADDR_IS_LINK_LOCAL)
		xSemaphoreGive(wifi_semph_get_ip6_addrs);
}

static esp_err_t wifi_connect(void) {
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_handler_on_wifi_disconnect, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_handler_on_sta_got_ip, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_handler_on_wifi_connect, wifi_sta_netif));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &wifi_handler_on_sta_got_ipv6, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_SSID,
			.password = CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_PASSWORD,
			.scan_method = WIFI_ALL_CHANNEL_SCAN
		},
	};

	esp_err_t ret = ESP_FAIL;
	while (ret != ESP_OK) {
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
		ESP_LOGI(TAG, "connecting to ssid: %s...", wifi_config.sta.ssid);
		ret = esp_wifi_connect();
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "connect failed, ret:%x %s", ret, esp_err_to_name(ret));
			vTaskDelay(pdMS_TO_TICKS(5000));
			continue;
		}

		ESP_LOGI(TAG, "waiting for ip4 addr...");
		if (xSemaphoreTake(wifi_semph_get_ip4_addrs, pdMS_TO_TICKS(10000)) == pdFALSE) {
			ESP_LOGE(TAG, "failed to get ip4 addr");
			esp_wifi_disconnect();
			ret = ESP_FAIL;
			continue;
		}
		ESP_LOGI(TAG, "waiting for ip6 addr...");
		if (xSemaphoreTake(wifi_semph_get_ip6_addrs, pdMS_TO_TICKS(10000)) == pdFALSE) {
			ESP_LOGE(TAG, "failed to get ip4 addr");
			// Continuing anyway.
		}
		ESP_LOGI(TAG, "connected");
	}
	return ret;
}

void wifi_init(void) {
	ESP_LOGI(TAG, "initializing wifi");
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
	esp_netif_config.if_desc = APP_WIFI_STA_NAME;
	esp_netif_config.route_prio = 128;
	wifi_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
	esp_wifi_set_default_wifi_sta_handlers();

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_semph_get_ip4_addrs = xSemaphoreCreateBinary();
	wifi_semph_get_ip6_addrs = xSemaphoreCreateBinary();
	wifi_connect();
}

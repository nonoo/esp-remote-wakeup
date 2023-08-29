#include "main.h"
#include "usb.h"

#include <esp_log.h>
#include <esp_http_server.h>

static const char *TAG = "httpd";

static esp_err_t httpd_wakeup_get_handler(httpd_req_t *req) {
	bool auth_ok = false;

	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		char *buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "found url query => %s", buf);
			char param[sizeof(CONFIG_ESP_WAKEUP_KEYPRESS_HTTPD_PASSWORD)] = { 0 };
			if (httpd_query_key_value(buf, "pass", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "found url query parameter => pass=%s", param);
				if (strcmp(param, CONFIG_ESP_WAKEUP_KEYPRESS_HTTPD_PASSWORD) == 0)
					auth_ok = true;
			}
		}
		free(buf);
	}

	if (auth_ok) {
		ESP_LOGI(TAG, "auth ok, sending keypress");
		usb_request_keypress_send(false);

		char *resp;
		int rc = asprintf(&resp, "{\"keypress_sent\": true}");
		if (rc < 0) {
			ESP_LOGE(TAG, "asprintf() returned: %d", rc);
			return ESP_FAIL;
		}
		if (!resp) {
			ESP_LOGE(TAG, "nomem for response");
			return ESP_ERR_NO_MEM;
		}
		httpd_resp_send(req, resp, strlen(resp));
		free(resp);
	} else {
		ESP_LOGE(TAG, "invalid auth");
		httpd_resp_set_status(req, "401 Unauthorized");
		httpd_resp_set_type(req, "application/json");
		httpd_resp_set_hdr(req, "Connection", "keep-alive");
		httpd_resp_send(req, NULL, 0);
	}

	return ESP_OK;
}

void httpd_init(void) {
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.lru_purge_enable = true;

	ESP_LOGI(TAG, "starting server on port %d", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(TAG, "error starting server");
		return;
	}

	static const httpd_uri_t wakeup = {
		.uri = "/wakeup",
		.method = HTTP_GET,
		.handler = httpd_wakeup_get_handler
	};
	httpd_register_uri_handler(server, &wakeup);

	ESP_LOGI(TAG, "server started");
}

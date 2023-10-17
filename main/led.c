#include "led.h"
#include "led_strip_encoder.h"
#include "main.h"

#include <esp_log.h>
#include <driver/rmt_tx.h>
#include <freertos/FreeRTOS.h>
#include <queue.h>

#define LED_RMT_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (LED strip needs a high resolution)
#if CONFIG_ESP_WAKEUP_KEYPRESS_LED_NOTIFY_MODE
#define LED_MAX_BRIGHTNESS_PERCENT	100
#else
#define LED_MAX_BRIGHTNESS_PERCENT	1
#endif

static const char *TAG = "led";

#define LED_EVENT_WIFI_CONNECTED		(1 << 0)
#define LED_EVENT_WIFI_DISCONNECTED		(1 << 1)
#define LED_EVENT_KEYPRESS_ON			(1 << 2)
#define LED_EVENT_KEYPRESS_OFF			(1 << 3)
static QueueHandle_t led_queue;

static uint8_t led_strip_pixels[3];
static rmt_channel_handle_t led_chan;
static rmt_encoder_handle_t led_encoder;

static void led_set(int r, int g, int b) {
	led_strip_pixels[0] = g * LED_MAX_BRIGHTNESS_PERCENT / 100.0;
	led_strip_pixels[1] = r * LED_MAX_BRIGHTNESS_PERCENT / 100.0;
	led_strip_pixels[2] = b * LED_MAX_BRIGHTNESS_PERCENT / 100.0;

	rmt_transmit_config_t tx_config = { 0, };
	ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
	ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}

void led_handle_keypress_on(void) {
	uint8_t eventid = LED_EVENT_KEYPRESS_ON;
	xQueueSend(led_queue, &eventid, portMAX_DELAY);
}

void led_handle_keypress_off(void) {
	uint8_t eventid = LED_EVENT_KEYPRESS_OFF;
	xQueueSend(led_queue, &eventid, portMAX_DELAY);
}

void led_handle_wifi_disconnected(void) {
	uint8_t eventid = LED_EVENT_WIFI_DISCONNECTED;
	xQueueSend(led_queue, &eventid, portMAX_DELAY);
}

void led_handle_wifi_connected(void) {
	uint8_t eventid = LED_EVENT_WIFI_CONNECTED;
	xQueueSend(led_queue, &eventid, portMAX_DELAY);
}

static void led_task(void *pvParameters) {
	ESP_LOGI(TAG, "led task started");

	bool keypress_on = false;
	bool wifi_disconnected = false;

	while (1) {
		uint8_t received_event;
		xQueueReceive(led_queue, &received_event, portMAX_DELAY);

		switch (received_event) {
			case LED_EVENT_WIFI_CONNECTED:
				ESP_LOGI(TAG, "got wifi connected event");
				wifi_disconnected = false;
				break;
			case LED_EVENT_WIFI_DISCONNECTED:
				ESP_LOGI(TAG, "got wifi disconnected event");
				wifi_disconnected = true;
				break;
			case LED_EVENT_KEYPRESS_ON:
				ESP_LOGI(TAG, "got keypress on event");
				keypress_on = true;
				break;
			case LED_EVENT_KEYPRESS_OFF:
				ESP_LOGI(TAG, "got keypress off event");
				keypress_on = false;
				break;
		}

		if (wifi_disconnected) {
			ESP_LOGI(TAG, "setting led state wifi disconnected");
			led_set(255, 0, 0);
		} else {
			if (keypress_on) {
				ESP_LOGI(TAG, "setting led state keypress on");
#if CONFIG_ESP_WAKEUP_KEYPRESS_LED_NOTIFY_MODE
				for (int i = 0; i < 30; i++) {
					led_set(0, 0, 0);
					vTaskDelay(pdMS_TO_TICKS(100));
					led_set(255, 255, 255);
					vTaskDelay(pdMS_TO_TICKS(100));
				}
#else
				led_set(0, 0, 255);
#endif
			} else {
				ESP_LOGI(TAG, "setting led state wifi connected");
#if CONFIG_ESP_WAKEUP_KEYPRESS_LED_NOTIFY_MODE
				led_set(0, 0, 0);
#else
				led_set(0, 255, 0);
#endif
			}
		}
	}
}

void led_init(void) {
	led_queue = xQueueCreate(5, sizeof(uint8_t));

	rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
		.gpio_num = CONFIG_ESP_WAKEUP_KEYPRESS_LED_STRIP_GPIO_NUM,
		.mem_block_symbols = 64, // increase the block size can make the LED less flickering
		.resolution_hz = LED_RMT_STRIP_RESOLUTION_HZ,
		.trans_queue_depth = 4, // set the number of transactions that can be pending in the background
	};
	ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

	led_strip_encoder_config_t encoder_config = {
		.resolution = LED_RMT_STRIP_RESOLUTION_HZ,
	};
	ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));
	ESP_ERROR_CHECK(rmt_enable(led_chan));

	xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);

	led_handle_wifi_disconnected();

	ESP_LOGI(TAG, "init done");
}

#include "usb.h"
#include "wifi.h"
#include "gpio.h"
#include "httpd.h"
#include "led.h"
#include "main.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void app_main(void) {
	esp_log_level_set("*", ESP_LOG_INFO);

	led_init();
	gpio_init();
	usb_init();
	wifi_init();
	httpd_init();

	vTaskSuspend(NULL);
}

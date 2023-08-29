#include "main.h"
#include "usb.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>

#define APP_BUTTON (GPIO_NUM_0)

static void IRAM_ATTR gpio_isr_handler(void *arg) {
	usb_request_keypress_send(true);
}

void gpio_init(void) {
	// Initialize button that will trigger HID reports
	const gpio_config_t boot_button_config = {
		.pin_bit_mask = BIT64(APP_BUTTON),
		.mode = GPIO_MODE_INPUT,
		.intr_type = GPIO_INTR_POSEDGE,
		.pull_up_en = true,
		.pull_down_en = false,
	};
	ESP_ERROR_CHECK(gpio_config(&boot_button_config));
	gpio_install_isr_service(0);
	gpio_isr_handler_add(APP_BUTTON, gpio_isr_handler, NULL);
}

#include "main.h"
#include "led.h"

#include <tinyusb.h>
#include <class/hid/hid_device.h>
#include <esp_log.h>
#include <event_groups.h>

static const char *TAG = "usb";

#define USB_EVENT_KEYPRESS	(1 << 0)
static EventGroupHandle_t usb_event_group = NULL;

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

const uint8_t hid_report_descriptor[] = {
	TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD))
};

const char *hid_string_descriptor[5] = {
	// Array of pointer to string descriptors
	(char[]) {
 0x09, 0x04
},  // 0: is supported language is English (0x0409)
"ESP",						// 1: Manufacturer
"Wakeup Keyboard Device",	// 2: Product
"123456",					// 3: Serials, should use chip ID
"Example HID interface",	// 4: HID
};

static const uint8_t hid_configuration_descriptor[] = {
	// Configuration number, interface count, string index, total length, attribute, power in mA
	TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

	// Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
	TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
	// We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
	return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
}

void usb_request_keypress_send(bool from_isr) {
	if (from_isr)
		xEventGroupSetBitsFromISR(usb_event_group, USB_EVENT_KEYPRESS, NULL);
	else
		xEventGroupSetBits(usb_event_group, USB_EVENT_KEYPRESS);
}

static void usb_task(void *pvParameters) {
	ESP_LOGI(TAG, "usb task started");

	while (1) {
		EventBits_t bits = xEventGroupWaitBits(usb_event_group, USB_EVENT_KEYPRESS, pdTRUE, pdFALSE, portMAX_DELAY);
		if (bits & USB_EVENT_KEYPRESS) {
			xEventGroupClearBits(usb_event_group, USB_EVENT_KEYPRESS);

#if !CONFIG_ESP_WAKEUP_KEYPRESS_LED_NOTIFY_MODE
			if (tud_mounted()) {
#endif
				ESP_LOGI(TAG, "sending wakeup signal");
				led_handle_keypress_on();

				tud_remote_wakeup();

				// uint8_t keycode[6] = { HID_KEY_A };
				// tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
				vTaskDelay(pdMS_TO_TICKS(50));
				// tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);

				led_handle_keypress_off();
#if !CONFIG_ESP_WAKEUP_KEYPRESS_LED_NOTIFY_MODE
			} else
				ESP_LOGI(TAG, "not mounted, not sending wakeup signal");
#endif
		}
	}
}

void usb_init(void) {
	ESP_LOGI(TAG, "usb init");
	usb_event_group = xEventGroupCreate();
	const tinyusb_config_t tusb_cfg = {
		.device_descriptor = NULL,
		.string_descriptor = hid_string_descriptor,
		.string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
		.external_phy = false,
		.configuration_descriptor = hid_configuration_descriptor,
	};

	ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
	if (xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL) != pdPASS)
		ESP_LOGE(TAG, "error creating usb task");
	ESP_LOGI(TAG, "usb init done");
}

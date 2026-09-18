#include "reboot.h"
#include "main.h"

#include <esp_log.h>

#if CONFIG_ESP_WAKEUP_KEYPRESS_REBOOT_INTERVAL_HOURS > 0
#include <esp_system.h>
#include <esp_timer.h>

static const char *TAG = "reboot";

static void reboot_timer_cb(void *arg) {
	ESP_LOGI(TAG, "reboot interval reached, restarting...");
	esp_restart();
}
#endif

void reboot_init(void) {
#if CONFIG_ESP_WAKEUP_KEYPRESS_REBOOT_INTERVAL_HOURS > 0
	ESP_LOGI(TAG, "reboot interval set to %d hours, starting timer", CONFIG_ESP_WAKEUP_KEYPRESS_REBOOT_INTERVAL_HOURS);

	const esp_timer_create_args_t reboot_timer_args = {
		.callback = &reboot_timer_cb,
		.name = "reboot_timer"
	};
	esp_timer_handle_t reboot_timer;
	ESP_ERROR_CHECK(esp_timer_create(&reboot_timer_args, &reboot_timer));
	ESP_ERROR_CHECK(esp_timer_start_once(reboot_timer, (uint64_t)CONFIG_ESP_WAKEUP_KEYPRESS_REBOOT_INTERVAL_HOURS * 3600ULL * 1000000ULL));
#endif
}

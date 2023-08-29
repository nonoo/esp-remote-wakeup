#ifndef LED_H_
#define LED_H_

void led_handle_keypress_on(void);
void led_handle_keypress_off(void);
void led_handle_wifi_disconnected(void);
void led_handle_wifi_connected(void);

void led_init(void);

#endif // LED_H_

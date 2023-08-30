# ESP Wakeup Keypress

A firmware for ESP32 devices to act as a USB keyboard and wake up the computer
when a HTTP request is received.

The story behind this is that my computer won't power on for Wake on LAN magic
packets for some unkown Linux/UEFI reason (all needed settings are enabled for
the network adapter, it's not a configuration issue). ESP Wakeup Keypress is
the solution I came up with - I can make a GET request to the ESP device and
it wakes up the computer for me.

## Usage

- Run `00-init.sh`, this will download and install ESP-IDF and all its
  dependencies
- Run `00-set-target.sh esp32s3` (replace `esp32s3` with your target)
- Edit the following settings in `sdkconfig`:

  - CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_SSID
  - CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_PASSWORD
  - CONFIG_ESP_WAKEUP_KEYPRESS_HTTPD_PASSWORD
  - CONFIG_ESP_WAKEUP_KEYPRESS_LED_STRIP_GPIO_NUM

I'm using a ESP32-S3-DevKitC which has the LED strip at GPIO48, but the latest
release of the devkit (ESP32-S3-DevKitC-1) has it on GPIO38.

- Run `01-build.sh`
- Create the file `defport-flash` and put `/dev/ttyACM0` into it (or the port
  where your ESP can be programmed)
- Put your ESP into bootloader mode by pressing and holding the GPIO0 button
  while pressing the reset button
- Run `02-flash.sh`
- Create the file `defport-monitor` and put `/dev/ttyUSB0` into it (or the port
  where your ESP serial console is)
- Run `03-monitor.sh`
- Press the reset button on your board to exit bootloader mode

Now if you press the button on your board, or open the URL
`http://esp-wakeup-keypress.localdomain/wakeup?pass=wakeup` an wakeup signal is
sent to your computer. You may need to change the `pass=wakeup` in the URL
if you've modified CONFIG_ESP_WAKEUP_KEYPRESS_HTTPD_PASSWORD in the
`sdkconfig` file.

## Troubleshooting

If your computer does not wake up for the virtual keypress, then create this
shell script at `/lib/systemd/system-sleep/00-esp-wakeup-enable.sh`:

```bash
#!/bin/bash

# Action script to enable wake after suspend by keyboard or mouse

if [ "$1" = post ]; then
    KB="$(lsusb -tvv | grep -A 1 303a:4004 | awk 'NR==2 {print $1}')"
    echo enabled > ${KB}/power/wakeup
fi

if [ "$1" = pre ]; then
    KB="$(lsusb -tvv | grep -A 1 303a:4004 | awk 'NR==2 {print $1}')"
    echo enabled > ${KB}/power/wakeup
fi
```

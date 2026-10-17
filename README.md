# ESP8266 AC IR Control

Control a split AC from Home Assistant using an ESP8266 NodeMCU and an IR LED.
ESPHome firmware; the AC is driven with raw IR timing captures (38 kHz), one
code per mode/temperature/swing combination.

## Hardware
- ESP8266 NodeMCU
- IR LED (940 nm) on GPIO4 (D2), ideally through a transistor for range
- DHT11 on GPIO5 (D1), optional (room temperature/humidity in HA)

## Build & flash
```
cp secrets.yaml.example secrets.yaml   # fill in wifi + api key
esphome run ac.yaml                    # first flash over USB, then OTA
```

## Home Assistant
The device shows up in HA via mDNS (the ESPHome integration). Add the "AC"
climate entity, the temperature/humidity sensors come along.

## Notes
- The on_state guard skips the first state sync after boot, so an ESP reboot
  or HA restart never transmits a stray command.

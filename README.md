# Temperature and Humidity Monitor for ESP32

This project provides a solution for monitoring temperature and humidity using an ESP32 microcontroller. It can be used in conjunction with [co2-temp-monitor](https://github.com/DrA1ex/co2-temp-monitor) for comprehensive environmental monitoring.

## Schematic

Connect the components based on the pin configurations defined in [/src/hardware.h](/src/hardware.h). If you prefer using custom pins, modify the file accordingly.

Note: Depending on your specific setup, additional components like diodes, capacitors, transistors and resistors might be necessary, but I don't use any.

![Temperature Humidity Monitor](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/252d2d2c-b5dc-4729-8d1b-844ee0317eaf)

## Build

This project is designed to be developed using [PlatformIO](https://platformio.org/?utm_source=platformio&utm_medium=piohome). If you don't have it installed yet, you'll need to [install it](https://docs.platformio.org/en/latest/core/installation/index.html) first.

## Configuration

1. **SSL Certificate**
   - Place your SSL CA certificate in `/src/certs/api.pem`.

2. **Credentials**
   - Configure your specific credentials in [/src/credentials.h](/src/credentials.h).
      - `API_URL`: Should contain the URL to the receiver's POST method, for example: `https://example.com/receiver/sensor`.
      - `API_KEY`: This key will be sent in the `API-Key` header and can be used by the receiver to verify the sender.

3. **Hardware Configuration**
   - Adjust pin configurations in [/src/hardware.h](/src/hardware.h) to match your hardware setup.

## Web UI

To configure monitor visit Web UI at adress `http://<YOUR-ESP32-IP>/`

![UI](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/d995ae59-028e-4cdd-9960-5aada7aa3bea)

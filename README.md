# Temperature and Humidity Monitor for ESP32

This project provides a solution for monitoring temperature and humidity using an ESP32 microcontroller. It can be used in conjunction with [co2-temp-monitor](https://github.com/DrA1ex/co2-temp-monitor) for comprehensive environmental monitoring.

## Schematic

To easily use this project, follow these steps to connect the components:

1. Open the [/src/hardware.h](/src/hardware.h) file and configure the pin connections based on the provided pin configurations.
   If you prefer using custom pins, make the necessary modifications in this file.

2. In this project, I have substituted the MQ-135 sensor with the MHZ-19B sensor. Additionally, a DC-DC converter is used to power both the controller and the fan from a single DC-Adapter.

**Note:** Depending on your specific setup, you might need additional components such as diodes, capacitors, transistors, and resistors. However, for this particular project, I haven't use any of these additional components.

![Temperature Humidity Monitor](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/c39c1a66-210f-49ed-9787-668ba9ce9be8)

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

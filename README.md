# Temperature and Humidity Monitor for ESP32

This project provides a solution for monitoring temperature and humidity using an ESP32 microcontroller. It can be used in conjunction with [co2-temp-monitor](https://github.com/DrA1ex/co2-temp-monitor) for comprehensive environmental monitoring.

## Schematic

To use this project, you will need to build a circuit based on the pin configuration provided in the [/src/hardware.h](/src/hardware.h) file. If you prefer to use custom pins, you can make the necessary modifications in this file.

The following image showcases the Temperature Humidity Monitor:

![Temperature Humidity Monitor](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/c39c1a66-210f-49ed-9787-668ba9ce9be8)

For improved accuracy in measuring CO2 levels, the MQ-135 sensor has been replaced with the MHZ-19B sensor. Additionally, a DC-DC converter is used to power both the controller and the fan from a single DC-Adapter.

Please keep in mind that depending on your specific setup, you may require additional components such as diodes, capacitors, transistors, and resistors.

You also have the option to replicate the scheme and circuit that I used. The following images provide details:

![EasyEDA](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/e070e5ef-3617-4df2-a027-a31c3beceb32)
![Circuit 1](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/b8d0e082-8bfb-4f34-bf44-1e6f64d66fc8)
![Circuit 2](https://github.com/DrA1ex/temp-monitor-esp32/assets/1194059/5850c6ee-b798-4c13-8ff0-22e01d19ab2f)

If you require the Gerber file for this project, you can download it from [here](https://github.com/DrA1ex/temp-monitor-esp32/files/13700695/Gerber_temp_monit_esp_2023-12-18.zip).

Note: The linear stabilizer (7805) may require passive or active cooling. If desired, you can replace it with any impulse converter. In my implementation, I used [this](https://ali.onl/2fg6) module.


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

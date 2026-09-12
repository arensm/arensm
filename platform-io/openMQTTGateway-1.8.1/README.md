
**Stand: 13. September 2026**

## Aktuelle Erweiterungen

## Lokale Anpassungen in `main/main.ino`

Gegenueber dem offiziellen Stand von OpenMQTTGateway 1.8.1 wurde der WLAN-
Start in `setupWiFiFromBuild()` fuer die manuelle WLAN-Konfiguration erweitert:

```cpp
WiFi.persistent(false); // hinzugefuegt
WiFi.mode(WIFI_STA);    // bereits im Original vorhanden
WiFi.disconnect();      // hinzugefuegt
delay(250);             // hinzugefuegt
```

- `WiFi.persistent(false)` verhindert, dass WLAN-Zugangsdaten und Aenderungen
  der WLAN-Konfiguration dauerhaft in den Flash beziehungsweise NVS geschrieben
  werden. Dadurch bestimmen weiterhin die beim Build hinterlegten Zugangsdaten
  die Verbindung und unnoetige Schreibzugriffe auf den Flash werden vermieden.
- `WiFi.disconnect()` beendet eine eventuell noch aktive oder vom vorherigen
  Start uebernommene Verbindung. `WiFiMulti` beginnt damit aus einem definierten
  Zustand und verbindet sich gezielt mit den in der Firmware hinterlegten Netzen.
- `delay(250)` gibt dem asynchron arbeitenden WLAN-Stack Zeit, den Wechsel in den
  Station-Modus und das Trennen vollstaendig abzuschliessen, bevor die
  Zugangspunkte registriert und der neue Verbindungsversuch gestartet werden.

`WiFi.mode(WIFI_STA)` gehoert bereits zum Originalcode und ist oben nur
aufgefuehrt, um die vollstaendige Startreihenfolge nachvollziehbar zu machen.
Die Anpassung wird ausschließlich kompiliert, wenn `ESPWifiManualSetup`
aktiviert ist. Ziel ist ein reproduzierbarer und zuverlaessiger WLAN-Start ohne
Einfluss zuvor gespeicherter Verbindungsdaten.

## Lokale Build-Konfiguration mit `.env` und `prod_env.ini`

Zugangsdaten und andere installationsbezogene Werte liegen nicht im
Quellcode. Sie werden lokal in der von Git ignorierten Datei `.env` als
Umgebungsvariablen gepflegt:

```bash
export OMG_WIFI_SSID="..."
export OMG_WIFI_PASSWORD="..."
export OMG_MQTT_SERVER="..."
export OMG_MQTT_PORT="1883"
export OMG_MQTT_USER="..."
export OMG_MQTT_PASS="..."
export OMG_GATEWAY_PASS="..."
export OMG_OTA_PASSWORD="..."
```

Vor jedem PlatformIO-Aufruf müssen diese Werte in die aktuelle Shell geladen
werden:

```bash
source .env
```

Die ebenfalls von Git ignorierte `prod_env.ini` enthält das lokale
Hardware-, Build- und Upload-Profil. PlatformIO liest sie über den Eintrag
`*_env.ini` in `extra_configs` automatisch ein. Das Standardziel ist
`nodemcuv2-rf-cc1101-d2`; es erweitert das vorhandene
`nodemcuv2-rf-cc1101`-Profil:

- `RF_RECEIVER_GPIO=4` legt den Datenausgang des CC1101-Empfängers auf D2
  beziehungsweise GPIO 4 des NodeMCU.
- `ESPWifiManualSetup=true` aktiviert die manuelle WLAN-Konfiguration und
  damit auch die oben beschriebene Anpassung in `setupWiFiFromBuild()`.
- WLAN-, MQTT-, Gateway- und OTA-Zugangsdaten werden mit
  `${sysenv.OMG_...}` aus der zuvor geladenen `.env` übernommen.
- Gateway-Name und OTA-Hostname sind fest auf `RF2MQTT` gesetzt, damit das
  Gerät im Netzwerk und in MQTT eindeutig wiedererkennbar ist.
- Der serielle Upload verwendet `/dev/cu.usbserial-0001` mit 115200 Baud.

Für spätere Aktualisierungen existiert zusätzlich das Ziel
`nodemcuv2-rf-cc1101-d2-ota`. Es erbt sämtliche Build-Einstellungen des
seriellen Profils und verwendet `espota` auf Port 8266. Das OTA-Passwort
kommt aus `OMG_OTA_PASSWORD`; die Zieladresse ist in `prod_env.ini`
hinterlegt.

```bash
# Standardziel bauen
pio run

# Erstinstallation oder Wiederherstellung über USB
pio run -t upload

# Aktualisierung über WLAN
pio run -e nodemcuv2-rf-cc1101-d2-ota -t upload
```

Diese Trennung hält private Werte aus dem Repository heraus und bewahrt die
lokale Gerätekonfiguration bei Aktualisierungen des OpenMQTTGateway-
Quellstands. Da die Werte über Build-Flags in die Firmware übernommen werden,
sind sie trotz der ausgelagerten Dateien Bestandteil der erzeugten Binärdatei
und müssen weiterhin vertraulich behandelt werden.


[![Community forum](https://img.shields.io/badge/community-forum-brightgreen.svg)](https://community.openmqttgateway.com)

![Build](https://github.com/1technophile/OpenMQTTGateway/workflows/Build/badge.svg?branch=development)
![Check Code Format](https://github.com/1technophile/OpenMQTTGateway/workflows/Check%20Code%20Format/badge.svg?branch=development)

[![OpenMQTTGateway capabilities](https://github.com/1technophile/OpenMQTTGateway/blob/development/docs/.vuepress/public/img/OpenMQTTGateway.png)](https://community.openmqttgateway.com)

OpenMQTTGateway aims to unify various technologies and protocols into a single firmware. This reduces the need for multiple physical bridges and streamlines diverse technologies under the widely-used [MQTT](http://mqtt.org/) protocol.

## Sponsors

<a href = "https://www.emqx.com/en?utm_source=github.com&utm_medium=referral&utm_campaign=OpenMQTTGateway-github-to-emqx-home"><img src="https://github.com/1technophile/OpenMQTTGateway/blob/development/docs/img/EMQ.png"  height="50"/></a>

## Documentation

The documentation is [here](https://docs.openmqttgateway.com)

The reference sheet, with the list of all functions, pinouts is [here](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=0)

## Upload

Upload OpenMQTTGateway directly from the [upload page](https://docs.openmqttgateway.com/upload/web-install.html) (no additional software required) or [build your own configuration](https://docs.openmqttgateway.com/upload/builds.html) with [PlatformIO](https://platformio.org/).

## Using OpenMQTTGateway ?
Support open-source development through sponsorship and gain exclusive access to our private forum. Your questions, issues, and feature requests will receive priority attention, plus you'll gain insider access to our roadmap.

[![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/theengs)

## Products powered by OpenMQTTGateway

### Theengs Bridge, Bluetooth gateway (BLE) with external antenna

[Theengs bridge](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna) is a powerfull BLE to MQTT gateway for over [100 sensors](https://decoder.theengs.io/devices/devices.html). Equipped with an Ethernet port, and external antenna, ensuring an enhanced range for your BLE sensors. It supports also WiFi connectivity.

[![Theengs bridge view](./docs/.vuepress/public/img/Theengs-Bridge-ble-gateway.png)](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna)

### Theengs Plug, Bluetooth gateway (BLE) gateway and Smart Plug

[Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption) brings the following features:
* BLE to MQTT gateway, tens of [Bluetooth devices](https://compatible.openmqttgateway.com/index.php/devices/ble-devices/) supported thanks to Theengs Decoder library. The plug uses an ESP32 acting as a BLE to Wifi gateway to scan, decode and forward the data of the nearby sensors,
* Smart plug that can be controlled remotely,
* Energy consumption monitoring,
* Device tracker,
* Presence detection (beta),
* Local connectivity first.

[![Theengs plug view](./docs/.vuepress/public/img/Theengs-Plug-OpenMQTTGateway.png)](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption)

Support the project by purchasing the [Theengs bridge](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna) or the [Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption)

## Compatible items

* [List of supported devices](https://compatible.openmqttgateway.com/index.php/devices/), door/window sensors, PIR sensors, smoke detectors, weather stations...

* [List of compatible boards (Off the shelf or DIY) is available](https://compatible.openmqttgateway.com/index.php/boards/), RF Bridge, IR, BLE gateways...

*Running on a computer*
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a Raspberry Pi, Windows or Unix PC you can now leverage [Theengs Gateway](https://theengs.github.io/gateway/).

* [List of compatible components to build your gateway](https://compatible.openmqttgateway.com/index.php/parts/), DHT, RF, IR emitters and receivers...

## Compatible controllers, saas or software

* [Home Assistant](https://docs.openmqttgateway.com/integrate/home_assistant.html)

* [OpenHAB](https://docs.openmqttgateway.com/integrate/openhab2.html)

* [NodeRed](https://docs.openmqttgateway.com/integrate/node_red.html)

* [AWS-IOT](https://docs.openmqttgateway.com/integrate/aws_iot.html)

## Contributors ✨

Thanks goes to these wonderful [people](https://github.com/1technophile/OpenMQTTGateway/graphs/contributors) who helped OpenMQTTGateway on Github and to the users contributions into the [community](https://community.openmqttgateway.com/).

## Support

For Questions or Support please don't open an issue, first go to the [docs](https://docs.openmqttgateway.com) and if you don't find your answer there, you can post your question in [the community forum](https://community.openmqttgateway.com)

## Help

If you like the project and/or used it please consider supporting it! It can be done in different ways:
* Helping other users in the [community](https://community.openmqttgateway.com)
* [Contribute](development) to the [code](https://github.com/1technophile/OpenMQTTGateway) or the [documentation](https://docs.openmqttgateway.com)
* Buy devices, boards or parts from the [compatible web site](https://compatible.openmqttgateway.com), the devices and parts linked use affiliated links.
* Donate or sponsor the project [developers](https://github.com/1technophile/OpenMQTTGateway/graphs/contributors)
* Make a video or a blog article about what you have done with [OpenMQTTGateway](https://docs.openmqttgateway.com) and share it to the [community](https://community.openmqttgateway.com)

## Media

* [Hackaday - ARDUINO LIBRARY BRINGS RTL_433 TO THE ESP32](https://hackaday.com/2023/01/13/arduino-library-brings-rtl_433-to-the-esp32)
* [CNX Software - 433 MHz is not dead! Using an ESP32 board with LoRa module to talk to 433 MHz sensors](https://www.cnx-software.com/2023/01/14/esp32-board-with-lora-433-mhz-sensors/)
* [RTL_433 PORTED TO ESP32 MICROCONTROLLERS WITH CC1101 OR SX127X TRANSCEIVER CHIPS](https://www.rtl-sdr.com/rtl_433-ported-to-esp32-microcontrollers-with-cc1101-or-sx127x-transceiver-chips/)
* [Using low-cost wireless sensors in the unlicensed bands](https://lwn.net/Articles/921497/)
* [SMART PLUG ESP32 OPENMQTTGATEWAY SERVING AS AN BLE MQTT GATEWAY AND A POWER METER](https://www.electronics-lab.com/smart-plug-esp32-openmqttgateway-serving-as-an-ble-mqtt-gateway-and-a-power-meter/)

### Theengs Plug
[![Theengs Plug video ElektroMaker](https://img.youtube.com/vi/nUwMt9p2U7o/0.jpg)](https://www.youtube.com/watch?v=nUwMt9p2U7o&t=427s)

### 433Mhz and BLE
[![433Mhz and BLE gateway video by Andreas Spiess](https://img.youtube.com/vi/_gdXR1uklaY/0.jpg)](https://www.youtube.com/watch?v=_gdXR1uklaY)

### BLE
[![BLE gateway video by Andreas Spiess](https://img.youtube.com/vi/noUROhtf0E0/0.jpg)](https://www.youtube.com/watch?v=noUROhtf0E0)

### 433Mhz
[![RTL_433 video by TECH MIND](https://img.youtube.com/vi/H-JXWbWjJYE/0.jpg)](https://www.youtube.com/watch?v=H-JXWbWjJYE)

### LORA
[![LORA video by Priceless Toolkit](https://img.youtube.com/vi/6DftaHxDawM/0.jpg)](https://www.youtube.com/watch?v=6DftaHxDawM)

# ESP32 yoRadio CYD

PlatformIO-Port von [yoRadio](https://github.com/e2002/yoradio) fuer das
ESP32-CYD aus der Spritpreis-Uhr. Das Projekt verwendet das integrierte
ILI9341-Display, den XPT2046-Touchcontroller und den Audioausgang an GPIO 26.
Mechanische Taster und Drehgeber sind deaktiviert.

## Bedienung

| Touch-Geste | Funktion |
| --- | --- |
| horizontal wischen | Lautstaerke aendern |
| vertikal wischen | Sender wechseln / Senderliste bewegen |
| kurz tippen | Play/Pause oder Auswahl bestaetigen |
| lang tippen | Player und Senderliste umschalten |

## Hardware

| Funktion | GPIO |
| --- | ---: |
| TFT SCLK / MOSI / MISO | 14 / 13 / 12 |
| TFT CS / DC / Backlight | 15 / 2 / 21 |
| Touch CLK / MOSI / MISO / CS / IRQ | 25 / 32 / 39 / 33 / 36 |
| Mono-Audio des CYD | 26 |

Es wird kein MAX98357A benoetigt. yoRadio nutzt den internen ESP32-DAC und
gibt den linken Monokanal auf GPIO 26 an die Audio-Schaltung des CYD aus.

## Build und Upload

```bash
pio run
pio run -t upload
pio device monitor
```

Der konfigurierte serielle Port ist `/dev/cu.usbserial-2110`.

Die Webdateien werden als SPIFFS-Abbild getrennt geschrieben:

```bash
pio run -t uploadfs
```

Beim ersten Start spannt yoRadio ein WLAN fuer die Einrichtung auf. Sender,
WLAN und weitere Einstellungen werden anschliessend ueber die Weboberflaeche
verwaltet.

Im AP-Modus beantwortet ein Wildcard-DNS die Captive-Portal-Pruefungen von
Android, iOS, macOS und Windows und leitet sie auf `http://192.168.4.1/`.

## Umgebungsdatei

`env.example` vor dem Build nach `.env` kopieren und `AP_SSID` sowie
`AP_PASSWORD` setzen. Passwoerter und andere geheime Werte gehoeren
ausschliesslich in `.env`; diese Datei wird nicht versioniert.

## Herkunft und Lizenz

Basis: yoRadio, Upstream-Stand `2fd3e388d528756f7db666b2261326a6ff234dc7`.
Die Software steht wie das Ausgangsprojekt unter der GNU GPL v3; siehe
`LICENSE`.

Der eingebettete Netzwerk-Unterbau wurde auf ESP32Async AsyncTCP 3.5.0
aktualisiert, weil der aeltere yoRadio-Stand beim Verbindungsaufbau im AP-Modus
abstuerzte.

## Maintainer

arensm@e-mail.de // MA

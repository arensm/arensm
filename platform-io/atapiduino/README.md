# Atapiduino

Atapiduino steuert ein ATAPI-CD/DVD-Laufwerk ueber einen Arduino Pro Mini und
drei PCF8574-I/O-Expander. Audio-CDs koennen ueber fuenf Taster gestartet,
pausiert, gestoppt sowie titelweise vor- und zurueckgeschaltet werden.

Dies ist ein modularer PlatformIO-Neubau des bisherigen Projekts
`code-snippet/atapiduino`. Der Altbestand bleibt unveraendert.

## Hardware

- Arduino Pro Mini, ATmega328P, 5 V, 16 MHz
- drei PCF8574
- ATAPI/IDE-CD- oder DVD-Laufwerk mit analogem Audioausgang
- eigenstaendige, fuer das Laufwerk geeignete Stromversorgung
- fuenf Taster gegen GND

### I2C-Expander

| PCF8574 | Adresse | Aufgabe |
| --- | ---: | --- |
| 1 | `0x20` | IDE `DD0` bis `DD7` |
| 2 | `0x21` | IDE `DD8` bis `DD15` |
| 3 | `0x22` | `nDIOR`, `nDIOW`, `nRST`, `nCS1`, `nCS0`, `DA2..DA0` |

Beim Pro Mini liegen SDA und SCL auf den hardwareseitigen I2C-Pins A4 und A5.
Die Verdrahtung des dritten PCF8574 entspricht der Bitfolge:

```text
Bit:      7     6     5    4     3    2   1   0
IDE:    nDIOR nDIOW nRST nCS1  nCS0  DA2 DA1 DA0
```

### Taster

| Funktion | Pin | Schaltung |
| --- | ---: | --- |
| Nächster Titel | D12 | Taster nach GND |
| Auswerfen/Laden | D11 | Taster nach GND |
| Stopp | D10 | Taster nach GND |
| Play/Pause | D9 | Taster nach GND |
| Vorheriger Titel | D8 | Taster nach GND |

Die Eingänge verwenden die internen Pull-up-Widerstände. Die eingebaute LED an
D13 zeigt eine erfolgreich abgeschlossene Laufwerksinitialisierung an.

## Bauen

Voraussetzung ist eine installierte PlatformIO-CLI.

```bash
pio run
```

Upload, falls der serielle Port automatisch erkannt wird:

```bash
pio run --target upload
```

Serielle Diagnose mit 9600 Baud:

```bash
pio device monitor
```

## Umgebungsdatei

`.env.example` enthaelt auf Deutsch und Englisch die Anweisung, die Datei vor
der Verwendung in `.env` umzubenennen. Das Projekt benoetigt derzeit keine
geheimen Werte; die gemeinsame Struktur ist fuer spaetere Konfigurationen
bereits eingerichtet. `.env` wird nicht eingecheckt.

## Aufbau

- `src/main.cpp`: Start, Tasterereignisse und periodisches Polling
- `src/AtapiBus.cpp`: 16-Bit-IDE-Zugriff ueber die drei PCF8574
- `src/CdPlayer.cpp`: ATAPI-Kommandos und CD-Zustand
- `include/HardwareConfig.h`: Pins, I2C-Adressen und Zeitlimits

## Unterschiede zum Altbestand

- Bus-, Player- und Tasterlogik sind getrennt.
- Alle Statuswartezeiten besitzen ein Zeitlimit.
- `DRQ` und `DRDY` werden mit eindeutigen Bitmasken geprüft.
- Taster reagieren entprellt auf die fallende Flanke statt bei jedem
  Schleifendurchlauf erneut.
- Die seriellen Statusmeldungen stehen jeweils in einer Zeile.

Der Build prueft Quellcode, Boarddefinition und Abhaengigkeiten. Funktion,
Verdrahtung und Laufwerkskompatibilitaet muessen anschliessend am realen Aufbau
geprueft werden.

## Herkunft und Lizenz

Die Neuimplementierung basiert auf dem ATAPI-Controller Release 3.1 von Carlos
Durandal und auf `matt199394/ATAPIduino-oled`. Entsprechend dem Altcode steht
der abgeleitete Quellcode unter `GPL-3.0-or-later`.

## Maintainer

arensm@e-mail.de // MA

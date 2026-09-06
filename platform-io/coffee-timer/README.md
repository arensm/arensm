# Coffee Timer

Coffee Timer erweitert eine Aroma-Plus-Kaffeemaschine um einen einstellbaren
Countdown. Ein Drehencoder stellt 0 bis 45 Sekunden ein. Ein Druck startet oder
stoppt den Countdown. Während der Timer läuft, ist das Relais aktiv; bei null
wird es sicher abgeschaltet.

Dies ist der modulare PlatformIO-Neubau von
`code-snippet/coffee-Timer`. Der Altbestand bleibt unveraendert.

## Hardware

- Arduino Uno (ATmega328P, 5 V, 16 MHz)
- zwei hintereinandergeschaltete 74HC595
- zweistellige 7-Segment-Anzeige mit aktiv-LOW-Segmentmustern
- Drehencoder mit Taster
- aktiv-LOW-Relaismodul

### Pinbelegung

| Funktion | Arduino-Pin |
| --- | ---: |
| 74HC595 Clock/SCLK | D2 |
| 74HC595 Data/SDI | D3 |
| 74HC595 Latch/LOAD | D4 |
| Encoder A | D5 |
| Encoder B | D6 |
| Encoder-Taster | D7 |
| Relais DIN, aktiv-LOW | D8 |

Encoder-Taster, A und B verwenden die internen Pull-up-Widerstaende. Alle
Komponenten benoetigen eine gemeinsame Masse. Vor der Verbindung mit der
Kaffeemaschine muessen Relaisart, Schaltspannung und sichere Netztrennung am
konkreten Aufbau geprueft werden.

## Bedienung

1. Drehencoder drehen und 0 bis 45 Sekunden einstellen.
2. Encoder druecken, um einen Wert groesser null zu starten.
3. Erneut druecken, um den laufenden Countdown abzubrechen.
4. Bei null schaltet die Firmware das Relais aus.

Der zuletzt eingestellte Wert wird mit `EEPROM.update()` verschleissarm
gespeichert und nach einem Neustart wieder angezeigt.

## Bauen

```bash
pio run
```

Upload bei automatisch erkanntem Port:

```bash
pio run --target upload
```

Serieller Monitor:

```bash
pio device monitor
```

## Umgebungsdatei

`.env.example` enthaelt auf Deutsch und Englisch die Anweisung, die Datei vor
der Verwendung in `.env` umzubenennen. Das Projekt benoetigt derzeit keine
geheimen Werte; die gemeinsame Struktur ist fuer spaetere Konfigurationen
bereits eingerichtet. `.env` wird nicht eingecheckt.

## Aufbau des Quellcodes

- `src/main.cpp`: Zusammenschaltung und Bedienablauf
- `src/CountdownTimer.cpp`: hardwareunabhaengige Countdown-Zustandslogik
- `src/InputDevices.cpp`: Entprellung von Encoder und Taster
- `src/TwoDigitDisplay.cpp`: Ausgabe ueber zwei 74HC595
- `include/HardwareConfig.h`: zentrale Pin- und Zeitkonfiguration

## Unterschiede zum Altbestand

- Anwendungslogik, Eingabe und Anzeige sind getrennte Module.
- Der Countdown holt verstrichene Intervalle nach, statt bei einer langsamen
  Schleife Zeit zu verlieren.
- Der Encoder-Taster wird ohne blockierendes `delay()` entprellt.
- EEPROM-Schreibzugriffe erfolgen nur bei geaenderten Daten.
- Die ungenutzte TM1637-Bibliothek wurde entfernt.

Ein erfolgreicher Build bestaetigt Quellcode, Board und Abhaengigkeiten. Upload,
Relaispolaritaet, Drehrichtung des Encoders, Segmentreihenfolge und Betrieb an
der Kaffeemaschine sind damit noch nicht praktisch verifiziert.

## Maintainer

arensm@e-mail.de // MA

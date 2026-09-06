# Uhrensöhne – gemeinsame Uhren-Firmware

Dieses PlatformIO-Projekt ist der gemeinsame Standard für alle Uhren der
Sammlung `uhrensoehne`. Grundlage ist das lokale Referenzprojekt
`esp8266-clock`.

## Enthaltene Uhren und Build-Umgebungen

| Umgebung | Hardware / Uhr |
| --- | --- |
| `obegransad` | ESP8266, IKEA OBEGRÄNSAD |
| `wordclock` | ESP8266, WordClock |
| `time-2-words` | ESP8266, Time 2 Words |
| `sieben-segmente` | ESP8266, Sieben-Segment-Uhr |
| `bttf1` | ESP8266, BTTF-1 |
| `custom` | ESP8266, eigene Uhr |
| `bttf3` | ESP32, BTTF-3; Standard-Build |
| `spritpreis-cyd-ili9341` | ESP32 CYD Spritpreis-Uhr, ILI9341 |
| `spritpreis-cyd-st7789` | ESP32 CYD Spritpreis-Uhr, ST7789 |

Alle Uhren starten über die gemeinsame `src/main.cpp` und implementieren die
in `src/clock_hooks.h` festgelegten `user_*`-Hooks. Der Hardwarecode liegt
gemeinsam in `src/cl_*.cpp`. Die Spritpreis-Uhr heißt
`src/cl_spritpreis_cyd.cpp`; sie verwendet dieselbe Modulschnittstelle,
verwaltet ihre spezialisierten Netzwerk-, Web-, Touch- und Displaydienste aber
weiterhin selbst.

## Build

Eine Variante gezielt bauen:

```bash
pio run -e wordclock
pio run -e bttf3
pio run -e spritpreis-cyd-ili9341
```

Alle Umgebungen prüfen:

```bash
pio run
```

Uploadbeispiel für die Spritpreis-Uhr:

```bash
pio run -e spritpreis-cyd-ili9341 -t upload
```

## Spritpreis-Uhr

Die Spritpreis-Uhr behält ihre bisherigen Funktionen einschließlich
WLAN-Einrichtung, Standortsuche, TankPuls-Abruf, Touch-Navigation und der fünf
Displayseiten. Seite 2 zeigt Diesel von exakt derselben auf Seite 1 ausgewählten
Tankstelle. Die Zuordnung geschieht über die Tankstellen-ID; ohne passenden
Dieselwert wird `--.--` angezeigt.

`spritpreis-cyd-ili9341` bleibt die bevorzugte Variante. Die ST7789-Umgebung
ist für entsprechend bestückte CYD-Chargen vorgesehen.

## Umgebungsdatei

`env.example` vor der Verwendung nach `.env` kopieren. Passwörter und andere
Geheimnisse gehören nur in `.env`; die Datei wird nicht nach GitHub gepusht.
Für die Spritpreis-Uhr sind WLAN-Daten optional, da sie weiterhin über das
Einrichtungsportal im Gerätespeicher hinterlegt werden können.

## Maintainer

arensm@e-mail.de // MA

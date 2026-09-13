# Spritpreis-Uhr für Heemol/CYD ESP32-2432S028R

Inspiration stammt von https://www.youtube.com/watch?v=_gmLFUoEDiQ

Eigenstaendiges PlatformIO-Projekt unter
`platform-io/spritpreis-uhr-cyd`, analog zu `platform-io/atapiduino`. Das
Projekt gehoert nicht zur Uhrensammlung `platform-io/uhrensoehne`.

Dieses Projekt ist für das abgebildete **Heemol/CYD V1.2 mit zwei USB-Buchsen**, 2,8-Zoll-Touchdisplay und laut Händler **ST7789** vorbereitet.

## Funktionen

- Tankstellen und Preise über die anonyme TankPuls-API
- E5, E10 oder Diesel
- zusätzliche Diesel-Seite für exakt dieselbe ausgewählte Tankstelle
- ausgewählte, günstigste und nächstgelegene Tankstelle
- Entfernung und berechnete Kosten für Hin- und Rückfahrt
- lokaler Preisverlauf seit dem Einschalten (bis zu 96 Abrufe)
- Wunschpreis-Alarm über den Lautsprecheranschluss an GPIO 26
- Ruhezeit für den Alarm
- getrennte WLAN-Einrichtung und Spritpreis-Konfiguration
- automatisches Captive Portal mit WLAN-Scan im Einrichtungs-WLAN
- Standortsuche per deutscher PLZ oder Ortsname
- 240×320-Hochformat in 270°-Richtung mit UTF-8-Schrift für deutsche Umlaute
- Touch: linke Bildschirmhälfte = zurück, rechte Hälfte = weiter
- alternative Build-Umgebung für ILI9341-Chargen

## Zusätzlich benötigte Hardware

- Heemol/CYD ESP32-2432S028R
- USB-Kabel und 5-V-Netzteil
- optional: **passiver 4-Ohm-Lautsprecher bis 1,5 W** mit passendem 2-poligen 1,25-mm-Stecker

Keinen aktiven Summer und keinen Lautsprecher mit eigener Versorgung direkt an den Speaker-Port anschließen.

## Installation mit PlatformIO

1. Den Ordner `spritpreis-uhr-cyd` in VS Code/PlatformIO öffnen.
2. Das Board per USB anschließen.
3. Für das getestete Modul wird standardmäßig `cyd-ili9341` gebaut.
4. Hochladen:

   ```bash
   pio run -e cyd-ili9341 -t upload
   ```

5. Seriellen Monitor öffnen:

   ```bash
   pio device monitor -b 115200
   ```

Nur falls eine andere Charge tatsächlich einen ST7789 besitzt, diese Variante testen:

```bash
pio run -e cyd-st7789 -t upload
```

## Umgebungsdatei

`env.example` enthaelt auf Deutsch und Englisch die Anweisung, sie vor der
Verwendung in `.env` umzubenennen. Optional koennen dort initiale WLAN-Daten
gesetzt werden. `.env` wird ignoriert und nicht nach GitHub gepusht.

Ohne `.env` bleibt der bisherige Ablauf erhalten: Das offene Einrichtungs-WLAN
fragt SSID und Passwort ab und speichert beide ausschließlich im NVS des
ESP32. Sobald dort WLAN-Daten vorhanden sind, haben sie Vorrang vor `.env`.
TankPuls wird anonym verwendet und benoetigt keinen API-Schluessel.

Die benötigten Bibliotheken lädt PlatformIO anhand der `platformio.ini`:

- LovyanGFX
- ArduinoJson
- QRCode
- U8g2

## Erste Einrichtung

1. Nach dem ersten Start erzeugt das Gerät das offene Einrichtungs-WLAN `Spritpreis-Uhr`.
2. Mit diesem WLAN verbinden. Auf dem Display steht „Verbinden Sie sich mit dem AP: Spritpreis-Uhr“. Die Anmeldeseite sollte automatisch erscheinen.
3. Falls das Mobilgerät die Seite nicht automatisch öffnet, im Browser ausdrücklich `http://192.168.4.1` eingeben. Dabei muss `http://` und nicht `https://` verwendet werden.
4. Auf dieser ersten Seite das eigene WLAN auswählen, dessen Passwort eingeben und **Verbinden** wählen. Diese AP-Seite enthält bewusst keine Spritpreis-Einstellungen.
5. Der ESP32 startet neu und verbindet sich mit dem gewählten WLAN.
6. Die fünfte Displayseite zeigt nun die normale IP-Adresse und einen QR-Code. Diese Adresse im Browser öffnen.
7. Erst auf dieser zweiten Webseite PLZ oder Ort, Kraftstoff, Radius und Alarmwert eintragen und speichern.

Die PLZ bzw. der Ortsname wird beim Speichern einmal über OpenStreetMap Nominatim in Koordinaten umgewandelt. Das Ergebnis wird lokal gespeichert; es findet keine automatische Vervollständigung und keine Ortssuche bei jedem Preisabruf statt. Die ermittelten Koordinaten werden bei Preisabfragen an TankPuls übertragen.

Soll später ein anderes WLAN verwendet werden, auf der Spritpreis-Konfigurationsseite **WLAN neu einrichten** wählen. Nur SSID und WLAN-Passwort werden dabei gelöscht; die übrige Konfiguration bleibt erhalten.

## Bevorzugte Tankstelle auswählen

Nach dem ersten erfolgreichen Datenabruf die Weboberfläche erneut öffnen. Im Feld „Bevorzugte Tankstelle“ werden die im Radius gefundenen Stationen vorgeschlagen. Die gewünschte ID auswählen und speichern.

Bleibt das Feld leer, zeigt Seite 1 ebenfalls die aktuell günstigste Station.

## Displayseiten

1. ausgewählte Tankstelle
2. Dieselpreis derselben ausgewählten Tankstelle
3. günstigste Tankstelle im Suchradius
4. nächstgelegene Tankstelle
5. lokaler Preisverlauf und Konfigurations-QR-Code

Der Dieselpreis wird separat abgerufen und ausschließlich über die eindeutige
Tankstellen-ID der auf Seite 1 angezeigten Station zugeordnet. Gibt es für genau
diese Station keinen Dieselpreis, zeigt Seite 2 `--.--` statt eines Preises von
einer anderen Tankstelle.

## Hinweise

- TankPuls erlaubt für anonyme, nichtkommerzielle Nutzung derzeit 60 Anfragen pro Minute und 10.000 pro Tag. Das Projekt fragt standardmäßig nur alle fünf Minuten ab.
- Die Ortssuche nutzt den öffentlichen Nominatim-Dienst mit identifizierendem User-Agent und speichert das vom Benutzer ausgelöste Suchergebnis lokal. Bitte keine automatisierten Serienabfragen daraus entwickeln.
- Die API unterstützt einen Radius bis maximal 25 km.
- Die Preisdaten werden auf dem Display als `TankPuls · MTS-K` gekennzeichnet.
- Der lokale Verlauf beginnt nach jedem Neustart neu. Der ESP32-Flash wird dadurch nicht ständig beschrieben.
- Die HTTPS-Verbindung prüft in dieser Version kein fest eingebautes Root-Zertifikat. Dadurch bleibt sie bei Zertifikatswechseln funktionsfähig, bietet aber keinen Schutz gegen manipulierte Antworten in einem kompromittierten Netzwerk.
- Das Board besitzt keinen erkennbaren LiPo-Ladeanschluss. Für Akkubetrieb ist ein geeignetes Lade-/Schutz- und 5-V-Wandlermodul erforderlich.

## Touch-Kalibrierung

Die Standardwerte sind `X 200–3800` und `Y 240–3800`. Reagiert nur ein Teil des Displays, lassen sich die vier Werte unten in der Weboberfläche ändern. Wird links und rechts vertauscht oder ist die Ausrichtung gedreht, muss zusätzlich `offset_rotation` in `src/main.cpp` angepasst werden.

## Verwendete Schnittstellen

| Funktion | GPIOs |
|---|---|
| LCD SPI | MISO 12, MOSI 13, CLK 14, CS 15, DC 2 |
| Hintergrundlicht | 21 |
| XPT2046 Touch | CLK 25, CS 33, MOSI 32, MISO 39, IRQ 36 |
| Lautsprecherverstärker | 26 |

Quellen: [TankPuls API](https://tankpuls.de/api.html), [Nominatim Search API](https://nominatim.org/release-docs/latest/api/Search/), [Nominatim-Nutzungsrichtlinie](https://operations.osmfoundation.org/policies/nominatim/), [ESP32-2432S028R Referenz](https://esp32pins.com/boards/esp32-cyd-2432s028r/).

## Maintainer

arensm@e-mail.de // MA

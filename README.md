# arensm projects

**Stand: 13. September 2026**

## Aktuelle Erweiterungen

- `spritpreis-uhr-cyd`: Eigenstaendiges PlatformIO-Projekt fuer das
  ESP32-CYD-Informationsdisplay, mit getrennten Umgebungen fuer ILI9341- und
  ST7789-Displays.
- `uhrensoehne`: Gemeinsame PlatformIO-Uhrensammlung nach dem Referenzaufbau
  von `esp8266-clock`.
- `esp32-yoRadio`: Als PlatformIO-Projekt fuer das ESP32-CYD integriert, mit
  Touch-Bedienung, Audioausgabe ueber den internen DAC, Weboberflaeche,
  Captive Portal und aktualisiertem AsyncTCP-Netzwerkunterbau.
- `openMQTTGateway-1.8.1`: Vollstaendiges PlatformIO-Projekt ergaenzt,
  einschließlich ESP32-/CC1101-Konfigurationen fuer Pilight und Somfy.
- `atapiduino`: Auswertung der Minute-/Sekunde-/Frame-Daten des ATAPI-
  Subchannels korrigiert.
- `coffee-timer`: Zweistellige Anzeige auf 99 begrenzt und beim Abbruch des
  Countdowns den zuvor ausgewaehlten Wert wiederhergestellt.

Dieses Verzeichnis ist der gemeinsame Einstiegspunkt fuer die Projekte von
`arensm`. Projekte werden nach ihrem primaeren Build- und Laufzeittyp
einsortiert.

## Kategorien

| Verzeichnis | Inhalt |
| --- | --- |
| `platform-io/` | Mikrocontroller-Firmware, die mit PlatformIO gebaut wird |
| `platform-io/uhrensoehne/` | Gemeinsames PlatformIO-Projekt fuer alle Uhr-Firmwarevarianten |
| `bash/` | Eigenstaendige Bash-Skripte und Bash-Projekte |
| `docs/adr/` | Uebergreifende Architecture Decision Records (ADRs) |

Weitere Kategorien werden erst angelegt, wenn ein passendes Projekt migriert
wird. Der verbindliche Aufbau ist in
[`docs/adr/0001-einheitlicher-projektaufbau.md`](docs/adr/0001-einheitlicher-projektaufbau.md)
beschrieben.

## Grundregeln

- Ein Projekt liegt genau in einer fachlich passenden Kategorie.
- Bestehende Quellen werden bei einer Migration nicht veraendert oder geloescht.
- Der neue Stand muss eigenstaendig baubar und dokumentiert sein.
- Zugangsdaten und lokale Build-Artefakte gehoeren nicht ins Repository.
- Alle Ordnernamen werden kleingeschrieben.
- Jedes Projekt enthaelt eine zweisprachige `env.example`; private Werte
  stehen ausschließlich in der ignorierten `.env`.

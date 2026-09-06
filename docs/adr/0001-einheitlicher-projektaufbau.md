# ADR-0001: Einheitlicher Aufbau aller Projekte

- Status: Angenommen
- Datum: 2026-09-06
- Entscheidungstraeger: arensm

## Kontext

Die Projekte liegen bisher in unterschiedlichen Verzeichnissen und verwenden
unterschiedliche Strukturen, Build-Verfahren und Dokumentationsformen. Das
erschwert Auffinden, Vergleichen, Bauen und Warten. Insbesondere reine Arduino-
Sketche sollen kuenftig reproduzierbar mit PlatformIO gebaut werden.

Bereits vorhandene Projekte bilden den nachvollziehbaren Altstand. Eine
Migration darf diesen Stand nicht ueberschreiben.

## Entscheidung

Alle neu geordneten Projekte liegen im Wurzelverzeichnis dieses Repositorys:

```text
./
```

Die erste Verzeichnisebene bezeichnet den primaeren Projekttyp:

```text
arensm/
|-- platform-io/
|   `-- uhrensoehne/
|-- bash/
`-- docs/
    `-- adr/
```

Weitere Kategorien werden bei Bedarf durch eine eigene ADR oder eine
Ergaenzung dieser ADR festgelegt. Unklare Sammelverzeichnisse wie `misc` oder
`sonstiges` werden nicht verwendet.

### Regeln fuer Migrationen

1. Das Quellprojekt bleibt unveraendert an seinem bisherigen Ort bestehen.
2. Das Zielprojekt wird neu in der passenden Kategorie angelegt.
3. Reine Arduino-Projekte werden als PlatformIO-Projekte neu aufgebaut.
4. Verhalten, Hardwarebelegung und externe Schnittstellen werden aus dem
   Altprojekt uebernommen und im neuen README dokumentiert.
5. Fehlerhafte oder blockierende Implementierungsdetails muessen nicht kopiert
   werden. Abweichungen mit Einfluss auf das Verhalten werden dokumentiert.
6. Jede Migration wird mindestens durch einen lokalen Build geprueft. Ein
   erfolgreicher Build ersetzt keinen Test auf der realen Hardware.

### Arduino-Projekte

Arduino bezeichnet in dieser Struktur das Framework und nicht die
Ablagekategorie. Neu aufgebaute Arduino-Firmware liegt deshalb immer unter
`platform-io/<projektname>` und verwendet in `platformio.ini`:

- eine exakt benannte PlatformIO-Board-ID,
- `framework = arduino`,
- fest deklarierte und moeglichst versionierte Bibliotheksabhaengigkeiten,
- die zum Projekt passende Geschwindigkeit fuer den seriellen Monitor.

Bei der Migration gelten zusaetzlich folgende Regeln:

1. Vorhandene `.ino`-Dateien bleiben ausschliesslich im unveraenderten
   Altprojekt erhalten. Der Neubau verwendet `src/main.cpp` mit explizitem
   `#include <Arduino.h>`.
2. Pinbelegung, elektrische Logik wie aktiv-LOW und das konkrete Board werden
   zentral dokumentiert und nicht ueber mehrere Quelldateien verteilt.
3. Hardwarezugriff, Anwendungslogik und Bedienung werden getrennt, sobald das
   Projekt mehr als eine triviale Funktion besitzt.
4. Blockierende Wartezeiten werden nur verwendet, wenn das Protokoll sie
   erfordert. Zeitablaeufe und Entprellung werden normalerweise mit `millis()`
   umgesetzt.
5. EEPROM wird mit verschleissarmen Operationen wie `EEPROM.update()` statt
   unbedingten Schreibzugriffen aktualisiert.
6. Nicht verwendete Bibliotheken werden entfernt. Verwendete Bibliotheken
   muessen in `lib_deps` stehen oder Teil des Arduino-Frameworks sein.
7. Build, Upload und Hardwaretest werden getrennt ausgewiesen. Nur tatsaechlich
   ausgefuehrte Pruefungen duerfen als erfolgreich dokumentiert werden.

### Uhrprojekte

Alle Uhr-Firmware wird innerhalb von PlatformIO in der gemeinsamen Sammlung
`platform-io/uhrensoehne/` abgelegt. Jedes Uhrprojekt bleibt darunter ein
eigenstaendig baubares PlatformIO-Projekt mit eigener Boarddefinition,
Abhaengigkeiten und Dokumentation.

Versionsangaben wie `_V5`, `-v2` oder `final` gehoeren nicht in den
Ordnernamen. Versionen werden ueber Git-Tags beziehungsweise Releases und eine
Firmware-Versionskonstante verwaltet. Ein etablierter Geraetename darf seine
bisherige Bedeutung behalten, der Ordnername wird jedoch kleingeschrieben.

### Umgebungsdateien und Geheimnisse

Jedes Projekt enthaelt eine versionierte `env.example`. Die ersten Hinweise
dieser Datei erklaeren auf Deutsch und Englisch, dass sie vor der Verwendung in
`.env` umbenannt werden muss. Die Beispieldatei enthaelt nur Platzhalter und
keine echten Zugangsdaten.

Passwoerter, Tokens, API-Schluessel und andere private Werte stehen
ausschließlich in `.env`. Die Datei `.env` darf niemals eingecheckt oder
gepusht werden. PlatformIO-Projekte lesen sie ueber ein `extra_scripts`-
Skript ein; daraus erzeugte Header und andere Zwischendateien liegen nur im
ignorierten `.pio/`-Verzeichnis.

Jede Projekt-`.gitignore` sowie die zentrale `.gitignore` enthalten mindestens:

```gitignore
.env
.pio/
.DS_Store
._*
```

Weitere typische macOS-Metadaten werden ebenfalls ignoriert. Geheimnisse, die
zur Laufzeit auf einem Mikrocontroller gebraucht werden, sind trotz dieser
Ablage im kompilierten Firmwareabbild enthalten und muessen entsprechend
geschuetzt und austauschbar bleiben.

Lokale Migrationsprotokolle werden als `docs/.MIGRATION.md` gespeichert. Jede
Projekt-`.gitignore` enthaelt `.MIGRATION.md`, damit interne Quell-/Zielpfade und
Arbeitsnotizen nicht nach GitHub uebertragen werden.

### Mindestinhalt eines Projekts

Jedes Projekt enthaelt:

- `README.md` mit Zweck, Voraussetzungen, Build, Installation und Hardware- bzw.
  Laufzeitkonfiguration,
- eine zum Werkzeug passende Ignore-Datei,
- deklarierte Abhaengigkeiten und Zielplattformen,
- Quellcode in der fuer das Werkzeug ueblichen Struktur,
- keine Zugangsdaten, Build-Artefakte oder editorabhaengigen Dateien.

PlatformIO-Projekte verwenden mindestens:

```text
projektname/
|-- platformio.ini
|-- include/
|-- src/
|-- test/
`-- README.md
```

Nicht benoetigte Verzeichnisse duerfen entfallen. Gemeinsam genutzte Logik wird
nicht vorschnell projektuebergreifend gekoppelt; eine gemeinsame Bibliothek
wird erst nach mindestens zwei konkreten Nutzungen eingefuehrt.

### Benennung

- Alle Ordnernamen werden kleingeschrieben.
- Neue Projektnamen werden in `kebab-case` geschrieben. Bei bestehenden Namen
  wird mindestens die Schreibweise in Kleinbuchstaben umgewandelt.
- ADR-Dateien verwenden `NNNN-kurzer-titel.md`.
- Jedes Projekt nennt im README einen verantwortlichen Maintainer.

## Konsequenzen

### Positiv

- Projekte lassen sich anhand ihres Pfades einordnen.
- Builds und Abhaengigkeiten werden reproduzierbar.
- Migrationen sind mit dem unveraenderten Altstand vergleichbar.
- Dokumentation und Entscheidungen sind an festen Stellen auffindbar.

### Negativ

- Alt- und Neubestand belegen voruebergehend doppelt Speicherplatz.
- Hardwareprojekte benoetigen zusaetzlich zum Build weiterhin einen Test am
  realen Geraet.
- Die Neustrukturierung kann bewusst von der Dateiaufteilung des Altprojekts
  abweichen.

## Pilot

`code-snippet/atapiduino` wird als erster Pilot neu unter
`platform-io/atapiduino` aufgebaut. Der bestehende Ordner bleibt unveraendert.

Als zweite Migration wird `code-snippet/coffee-Timer` unter dem normalisierten
Namen `platform-io/coffee-timer` neu aufgebaut. Das Projekt dient als Referenz
fuer Arduino-Uno-Firmware mit Anzeige, Drehencoder, EEPROM und Relais.

Die Uhrensammlung beginnt mit `Arduino/Projekte/WordClock_Ari_V5`. Der Neubau
liegt ohne Versionssuffix unter `platform-io/uhrensoehne/wordclock_ari`.

Die bestehende `Spritpreis-Uhr-CYD` wird als
`platform-io/uhrensoehne/spritpreis-uhr-cyd` aufgenommen. Mehrere notwendige
Displaycontroller bleiben als getrennte PlatformIO-Umgebungen im selben
Projekt erhalten.

Für die Uhrensammlung wird anschließend das lokale Referenzprojekt
`esp8266-clock` als verbindlicher technischer Standard übernommen.
`platform-io/uhrensoehne` ist selbst das
gemeinsame PlatformIO-Projekt; ein zusätzlicher Projekt-Unterordner wird nicht
verwendet. Alle Uhrenvarianten werden über eigene, kleingeschriebene Umgebungen
gebaut. Alle Varianten starten über die gemeinsame `src/main.cpp` und
implementieren die in `src/clock_hooks.h` definierte `user_*`-Schnittstelle.
Die Spritpreis-Uhr liegt als `src/cl_spritpreis_cyd.cpp` direkt neben den
anderen Dateien `cl_*.cpp`, verwaltet ihre spezialisierten Dienste selbst und
besitzt ILI9341- und ST7789-Umgebungen. Ihr eigenständiger vorheriger Neubau
bleibt als Vergleichsstand erhalten.

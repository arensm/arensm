# Retro TV V20

Kleines Retro-Monitorgehäuse für ein 4,2-Zoll-E-Paper-Display auf einem Desktop-Sockel. V20 enthält jedes Druckteil als eigene OpenSCAD-Datei, STL und PNG-Vorschau.

Die Gehäuseteile basieren auf **V11**, das korrigierte Drehgelenk auf **V13**. Der obere Drehteller ist an die Monitorauflage angepasst; der Anschlag sitzt hinten auf einem kleineren Radius. Die beiden Gelenkteile gemeinsam verwenden.

## Vorschau

![Zusammengebautes Gehäuse ohne Display-Elektronik](Baugruppe.png)

![Explosionszeichnung der acht Druckteile](Explosionszeichnung.png)

## Dateien

| Bauteil | OpenSCAD | Druckdatei | Vorschau |
| --- | --- | --- | --- |
| Monitorblende | [front.scad](front.scad) | [front.stl](front.stl) | [PNG](front.png) |
| Monitor-Rückgehäuse mit geschlossenen Kabeltaschen | [rueckgehaeuse.scad](rueckgehaeuse.scad) | [rueckgehaeuse.stl](rueckgehaeuse.stl) | [PNG](rueckgehaeuse.png) |
| Display-Haltering | [displayhalter.scad](displayhalter.scad) | [displayhalter.stl](displayhalter.stl) | [PNG](displayhalter.png) |
| Unterer Standfuß | [stand_unterteil.scad](stand_unterteil.scad) | [stand_unterteil.stl](stand_unterteil.stl) | [PNG](stand_unterteil.png) |
| Oberer Monitor-Drehteller | [monitor_drehteller.scad](monitor_drehteller.scad) | [monitor_drehteller.stl](monitor_drehteller.stl) | [PNG](monitor_drehteller.png) |
| Computer-Frontblende | [sockelfront.scad](sockelfront.scad) | [sockelfront.stl](sockelfront.stl) | [PNG](sockelfront.png) |
| Computer-Oberschale | [sockeldeckel.scad](sockeldeckel.scad) | [sockeldeckel.stl](sockeldeckel.stl) | [PNG](sockeldeckel.png) |
| Computer-Unterschale | [bodenplatte.scad](bodenplatte.scad) | [bodenplatte.stl](bodenplatte.stl) | [PNG](bodenplatte.png) |
| Gewindeeinsatz-Passprobe | [SCAD](Gewindeeinsatz_Passprobe.scad) | [STL](Gewindeeinsatz_Passprobe.stl) | [PNG](Gewindeeinsatz_Passprobe.png) |

Zusätzlich enthalten:

- [Baugruppe.scad](Baugruppe.scad): zusammengesetztes Gehäuse ohne Display-Elektronik.
- [Explosionszeichnung.scad](Explosionszeichnung.scad): räumlich getrennte Darstellung der acht Hauptteile.
- `Geometrie/`: benötigte, aus STEP abgeleitete Grundkonturen.
- [Pruefbericht.txt](Pruefbericht.txt): Ergebnisse der Geometrieprüfung.

Die bereits im Ordner vorhandenen Dateien mit `V13` beziehungsweise `v13` im Namen und `Vorschau.png` wurden erhalten. Für V20 die oben verlinkten Bauteildateien verwenden.

## In OpenSCAD bearbeiten

1. Das vollständige Paket entpacken. **Den Unterordner `Geometrie` neben den SCAD-Dateien behalten.**
2. Die gewünschte Bauteil-SCAD öffnen. Sie zeigt genau ein Teil in Drucklage.
3. Die Parameter am Dateianfang ändern. Alle Längen sind in Millimetern angegeben.
4. Mit **F5** die Vorschau prüfen und mit **F6** rendern.
5. Über **Datei → Exportieren → Als STL exportieren** eine neue Druckdatei erstellen.

Baugruppe und Explosionszeichnung verwenden die einzelnen SCAD-Dateien. Änderungen an diesen Quellen werden dort beim erneuten Laden beziehungsweise Berechnen übernommen. Vorhandene STL- und PNG-Dateien aktualisieren sich nicht automatisch.

Die importierten Gehäusekonturen sind fest. Das Ändern einzelner Parameter skaliert diese Konturen nicht automatisch. Frei einstellbar bleiben die in der jeweiligen Datei vorhandenen Einbauten und Ausschnitte. Aufeinander abgestimmte Maße des Drehgelenks müssen in beiden Dateien zusammen geändert werden.

### Wichtige Parameter

| Datei | Parameter | Bedeutung |
| --- | --- | --- |
| `bodenplatte.scad` | `halter_abstand_x`, `halter_abstand_y` | Lochraster der vier Elektronikhalter |
| `bodenplatte.scad` | `halter_versatz_x`, `halter_versatz_y` | Gemeinsame Position der Halter |
| `bodenplatte.scad` | `halter_1_korrektur` bis `halter_4_korrektur` | Individuelle Korrektur: `[X, Y, Höhe, Außendurchmesser]` |
| `bodenplatte.scad` | `halter_hoehe`, `halter_durchmesser` | Grundabmessungen der Elektronikhalter |
| `bodenplatte.scad` | `einsatz_bohrung`, `einsatz_tiefe` | Aufnahme für die Gewindeeinsätze der Elektronikhalter |
| `bodenplatte.scad` | `usb_breite`, `usb_hoehe`, `usb_radius`, `usb_mitte_x`, `usb_mitte_z` | USB-Ausschnitt in der Rückwand |
| `sockeldeckel.scad` | `gehaeuse_einsatz_d`, `gehaeuse_einsatz_tiefe` | Aufnahmen für die Gehäuseverschraubung |
| `rueckgehaeuse.scad` | `stecker_hoehe`, `stecker_mitte_y`, `stecker_aussen_x` | Größe und Lage der seitlichen Kabeltaschen |

Der USB-Ausschnitt ist ein einstellbarer Wandausschnitt; seine Standardmaße sind keine Zusage für eine bestimmte USB-C-Buchse. Das Elektronik-Lochraster ist ebenfalls an die tatsächlich verwendete Platine anzupassen.

## Display und Befestigung

| Maß | Wert |
| --- | --- |
| Platinenaußenmaß | 103 × 78,5 mm |
| Gesamtdicke mit Display | 3,42 mm |
| Displaydicke | 1,6 mm |
| Aktive Bildfläche | 84,8 × 63,6 mm |
| Untere Platinenkante bis aktive Bildfläche | 10,5 mm |
| M3-Gewindeeinsatz: Länge | 5,7 mm |
| M3-Gewindeeinsatz: Außendurchmesser | 4,28 mm |
| Standard-Pilotbohrung | Ø 4,0 mm, 6,0 mm tief |

Die asymmetrische Lage der aktiven Bildfläche ist in der Monitorblende berücksichtigt. Die geschlossenen Kabeltaschen des Rückgehäuses behalten 2 mm Außenwand über den Taschen.

Die zentrale Gelenkachse verwendet **M4**. Zwei Befestigungen verbinden den oberen Drehteller mit dem Monitor; vier Befestigungen verbinden den unteren Standfuß mit dem Computerdeckel. Der Computerdeckel wird über vier Schrauben von unten befestigt.

## Drucken und montieren

1. Zuerst die Gewindeeinsatz-Passprobe drucken. Sie enthält Bohrungen mit Ø **3,9 / 4,0 / 4,1 mm** zum Abstimmen auf Drucker, Material und Einsatz.
2. Die acht Hauptteile im Slicer öffnen. Die gelieferten STL-Dateien liegen bereits auf Z = 0 in der vorgesehenen Drucklage. Überhänge, Brücken und gegebenenfalls nötige Stützen in der Schichtvorschau prüfen; es wurde kein konkretes Druckprofil validiert.
3. Gewindeeinsätze in die vorgesehenen Aufnahmen einsetzen. Befestigungs- und Kabelfreiraum vor dem endgültigen Zusammenbau prüfen.
4. Unteren Standfuß von der Innenseite des Computerdeckels befestigen. Oberen Drehteller mit dem Monitorboden verbinden und das Gelenk mit der zentralen M4-Verbindung zusammenfügen. Die Verbindung so einstellen, dass das Gelenk beweglich bleibt.
5. Kabel durch die bogenförmigen Öffnungen führen. Bewegungsreserve lassen und beide Drehrichtungen prüfen.
6. Display in die Monitoraufnahme einsetzen, Haltering und Rückgehäuse montieren. Kabel dürfen dabei nicht zwischen den Gehäuseteilen eingeklemmt werden.
7. Elektronik auf den Bodenhaltern befestigen, Computer-Frontblende einsetzen und die beiden Schalen mit den vier Schrauben von unten verbinden.

Schraubenlängen und Klemmung anhand der tatsächlich verwendeten Schraubenköpfe, Unterlegscheiben und Einsätze kontrollieren. Die Explosionszeichnung zeigt die Zuordnung der Teile; sie enthält keine Schrauben oder Elektronik.

## Prüfung und Grenzen

- Alle neun Druck-STLs einschließlich Passprobe sind geschlossen, haben konsistente Normalen und liegen mit ihrer Unterseite auf Z = 0.
- Abmessungen und Volumen wurden gegen die bisherigen Referenzen geprüft.
- Acht STL-Dateien wurden neu aus SCAD exportiert. Das Rückgehäuse-STL wurde neu aus der passenden V11-STEP-Geometrie erzeugt; seine separate SCAD wurde als CSG und PNG geprüft. Der vollständige OpenSCAD-STL-Export dieses Teils wurde wegen der langen Rechenzeit abgebrochen.
- Das V13-Gelenk wurde geometrisch bei 0° und ±30° auf freie Bewegung geprüft. Bei ±34° blockieren die Endanschläge; das genaue Kontaktmaß innerhalb dieses Bereichs wurde nicht bestimmt.
- Eine reale Druck-, Stecker- und Montagepassprobe des vollständigen V20-Pakets steht aus.
- Vorherige Versionsdateien wurden bei der Erstellung von V20 nicht verändert.

# squeezelite-ALCD

`squeezelite-ALCD` ist ein Remix aus zwei Squeezelite-Projekten:

- [ralph-irving/squeezelite](https://github.com/ralph-irving/squeezelite) – aktuelle
  Squeezelite-Basis, Audiowiedergabe, Codecs, SlimProto und LIRC-Unterstützung.
- [fpasteau/squeezelite](https://github.com/fpasteau/squeezelite) – Grundlage für
  die zusätzliche, vom Server gesteuerte Textanzeige aus dem früheren
  Squeezeslave-GUI-/Display-Zweig.

Ziel dieses Projekts ist ein aktuelles Squeezelite mit Unterstützung für ein
lokales oder entferntes LCD, das über LCDd aus dem
[LCDproc-Projekt](https://www.lcdproc.org/) angesteuert wird. Der aktuelle
Squeezelite-Unterbau bleibt dabei so weit wie möglich unverändert.

## Was wurde geändert?

Ausgehend vom aktuellen Code von `ralph-irving/squeezelite` wurden nur die für
die Textanzeige erforderlichen Teile portiert und an den neuen Quellstand
angepasst:

- optionales Build-Feature `INTERACTIVE` ergänzt;
- neue LCDd-Anbindung in `interactive.c` und `interactive.h`;
- Anmeldung beim Lyrion Media Server als textfähiger Squeezeslave-Client, wenn
  die LCD-Funktion aktiviert ist;
- Übermittlung der Displaybreite per SlimProto-`SETD`;
- Verarbeitung der vom Server gesendeten SlimProto-`vfdc`-Displaydaten;
- Ausgabe als zwei Textzeilen über LCDd;
- optionale zweizeilige Terminalanzeige mit `-T` über ncurses;
- getrennte Zeichencodierung für beide Ausgaben: deutsche Umlaute und `ß`
  werden im Terminal als UTF-8 und für LCDd als ISO-8859-1 an die
  `hd44780_euro`-Zeichentabelle übergeben;
- direkte Tastatursteuerung aus einem lokalen oder SSH-Terminal, ohne LIRC;
- Zurücksetzen von Anzeige und virtuellem HD44780-DDRAM-Zeiger bei jedem
  vollständigen Displaypaket, damit beide Zeilen zuverlässig aktualisiert werden;
- Verbindung zu LCDd auf dem lokalen Rechner oder auf einem entfernten Host;
- Unterstützung von IPv4, IPv6 und Hostnamen für den LCDd-Server;
- automatische Übernahme der von LCDd gemeldeten Displaybreite;
- Kompatibilitätsmodus für ältere LCDd-Versionen;
- Schutz vor zu kurzen oder fehlerhaften `vfdc`-Paketen;
- Begrenzung der Displaybreite auf 11 bis 99 Zeichen;
- Bereinigung von Steuerzeichen, Zeilenumbrüchen und LCDd-Sonderzeichen vor der
  Ausgabe;
- regelmäßiges Leeren der LCDd-Antwortdaten, damit sich der Socket-Puffer bei
  längerer Laufzeit nicht füllt;
- Fehler beim Verbindungsaufbau zu LCDd beenden die Audiowiedergabe nicht.
- ein mit `-n` gesetzter Playername bleibt verbindlich und wird nicht mehr
  durch einen zuvor in LMS gespeicherten Namen überschrieben; er wird nach
  jedem Verbindungsaufbau aktiv an LMS gesendet;
- zusätzliche LIRC-Kommandos für Navigation sowie eine zustandsabhängige
  kombinierte Wiedergabe-/Pause-Funktion.

Nicht übernommen wurden veraltete Änderungen an Audioausgabe, Decodern,
Codecs, GPIO oder Versionsnummern. Von der früheren Curses-Oberfläche wurde nur
die kompakte Textanzeige neu umgesetzt; deren Tastatursteuerung wurde nicht
portiert.

## Infrarot-Fernbedienung

Der aktuelle Originalcode von `ralph-irving/squeezelite` enthält bereits eine
LIRC-basierte IR-Fernbedienung. Deshalb wurde der alte IR-Code aus dem
Display-Zweig nicht übernommen. Die aktuelle Squeezelite-Basis wurde lediglich
um die Kommandos `up`, `down`, `left`, `right` und `playpause` erweitert, damit
kurzer und langer Tastendruck in der Image-Konfiguration getrennt abgebildet
werden können.

Die IR-Unterstützung wird beim Kompilieren mit `-DIR` aktiviert und zur Laufzeit
mit `-i` eingeschaltet:

```bash
./squeezelite -i
```

Optional kann eine eigene LIRC-Konfiguration angegeben werden:

```bash
./squeezelite -i /pfad/zur/lircrc
```

Das Raspberry-Pi-Buildroot-Image enthält LIRC, den GPIO-IR-Empfänger auf BCM
GPIO 4 (physischer Pin 7) und eine Standarddefinition für die weiße Apple-
Fernbedienung A1156. Kurzes Drücken von Plus/Minus navigiert, langes Drücken
ändert die Lautstärke. Die Mitteltaste startet die Auswahl; Menü schaltet
Wiedergabe/Pause und ein langer Menüdruck den Player ein oder aus.
`lircd.conf` und `lircrc` liegen auf der beschreibbaren FAT-Bootpartition.

Zusätzlich ist ein MAX98357A-I2S-Verstärker aktiviert. Dessen `SD_MODE`-Eingang
wird über BCM GPIO 17 (physischer Pin 11) vom Audiotreiber gesteuert, damit beim
Öffnen des I2S-Ausgangs kein lautes Knacken entsteht.

## Kompilieren

Es gelten grundsätzlich die Abhängigkeiten und Build-Hinweise des aktuellen
Squeezelite-Projekts. Die LCDd-Anbindung verwendet direkt eine TCP-Verbindung
und benötigt keine zusätzliche LCDproc-Clientbibliothek. Das Feature
`INTERACTIVE` linkt ncurses für die optionale Terminalanzeige.

Nur LCD-Unterstützung:

```bash
make OPTS="-DINTERACTIVE"
```

LCD und die vorhandene LIRC-Unterstützung unter Linux:

```bash
make OPTS="-DINTERACTIVE -DIR"
```

Weitere benötigte Squeezelite-Optionen können wie gewohnt zu `OPTS` ergänzt
werden.

## LCDd verwenden

LCDd muss laufen und auf seinem Standard-TCP-Port `13666` erreichbar sein.

Lokaler LCDd-Server:

```bash
./squeezelite -y
```

Entfernter LCDd-Server:

```bash
./squeezelite -y -E lcd.example.lan
```

LCDd und ein bestimmter Lyrion Media Server:

```bash
./squeezelite -s lms.example.lan -y -E lcd.example.lan
```

LCDd zusammen mit LIRC:

```bash
./squeezelite -y -i
```

## Neue Kommandozeilenoptionen

| Option | Bedeutung |
| --- | --- |
| `-y` | LCDd-Ausgabe aktivieren |
| `-x` | LCDd-Ausgabe mit Prioritätssyntax für LCDd älter als 0.5.4 aktivieren |
| `-T` | zweizeilige Textanzeige im aktuellen Terminal aktivieren |
| `-E <host>` | LCDd-Host festlegen; Standard ist `127.0.0.1` |
| `-w <zeichen>` | Displaybreite zwischen 11 und 99 Zeichen festlegen |

`-E` legt nur den Zielhost fest. Die LCDd-Ausgabe muss zusätzlich mit `-y` oder
`-x` aktiviert werden. `-T` arbeitet unabhängig von LCDd; beide Ausgaben können
gleichzeitig aktiv sein. Meldet LCDd beim Verbindungsaufbau eine gültige
Breite, hat dieser Wert Vorrang vor `-w`.

Bei `-T` wird die Tastatur direkt aus dem aktuellen Terminal gelesen. Das gilt
auch bei einem Start in einer SSH-Sitzung und benötigt keinen LIRC-Daemon:

| Taste | Funktion |
| --- | --- |
| Pfeiltasten | Navigation in der LMS-Anzeige |
| Enter | Auswahl/rechts |
| Leertaste oder `P` | Pause |
| `p` | Wiedergabe |
| `+`/`=` und `-` | Lauter und leiser |
| `m` | Stumm |
| `h` | Home |
| `0` bis `9` | Ziffern/Voreinstellungen |
| `q` | Squeezelite beenden |

Auf dem Raspberry Pi 2 wurde folgende Kombination mit einer interaktiven
SSH-Sitzung und dem MAX98357A erfolgreich geprüft:

```bash
./squeezelite -T -s lms.lan -n squeezelite-ALCD \
  -o plughw:CARD=MAX98357A,DEV=0 -f /tmp/squeezelite-terminal.log
```

## Technischer Ablauf

Bei aktivierter LCD-Funktion meldet sich Squeezelite gegenüber dem Server mit
der Geräte-ID des textfähigen Squeezeslave-Clients. Anschließend wird die
Displaybreite mit einem `SETD`-Paket übertragen. Der Server sendet daraufhin die
Displayinhalte als `vfdc`-Pakete. `squeezelite-ALCD` dekodiert die darin
enthaltenen Zeichen- und Cursorbefehle und aktualisiert zwei LCDd-Textwidgets.
Da die LMS-SqueezeSlave-Klasse eingehende `SETD`-Namenspakete nicht verarbeitet,
wird ein ausdrücklich mit `-n` gesetzter Name zusätzlich über die LMS-CLI auf
TCP-Port 9090 gespeichert. Die CLI muss dafür aktiviert und erreichbar sein.

Ohne `-y`, `-x` oder `-T` verhält sich das Programm wie das unveränderte
aktuelle Squeezelite und verwendet weiterhin die normale
SqueezePlay-Geräte-ID.

## Prüfstand

Der LCD-Port wurde wie folgt geprüft:

- ein vollständiges 32-Bit-ARMv7-Buildroot-Image mit glibc wurde erfolgreich
  gebaut;
- `interactive.c` wurde zusätzlich mit `-Wall -Wextra -Werror` geprüft;
- ein Parser-Test für Löschen, Zeilenwechsel, Paketgrenzen sowie Host- und
  Breitenvalidierung wurde erfolgreich ausgeführt;
- `git diff --check` meldet keine Whitespace-Fehler;
- die zusätzlichen LIRC-Kommandos wurden mit der Apple-A1156-Fernbedienung am
  Raspberry Pi 2 geprüft.

LCDd mit zwei Zeilen und deutschen Umlauten, Titelanzeige, `-T` samt
SSH-Tastatursteuerung, Playername, Audio über den MAX98357A sowie die
Apple-A1156-Fernbedienung wurden auf einem Raspberry Pi 2 live geprüft. Das
Buildroot-Image 0.8 ergänzt den schwarzen AudioBox-Startbildschirm mit
Fortschrittsbalken und IP-Anzeige sowie eine Notfallkonsole auf `tty2`. Diese
0.8-Startdarstellung ist im Build verifiziert und muss nach dem Flashen noch
auf der Zielhardware geprüft werden.

## Herkunft und Lizenz

Dieses Projekt basiert auf:

- [ralph-irving/squeezelite](https://github.com/ralph-irving/squeezelite)
- [fpasteau/squeezelite](https://github.com/fpasteau/squeezelite)

Die ursprünglichen Copyright- und Lizenzhinweise bleiben erhalten. Es gilt die
im Projekt enthaltene [GPL-Lizenz](LICENSE.txt). Hinweise zu eingebundenen
Bibliotheken und deren eigenen Lizenzen befinden sich ebenfalls in
`LICENSE.txt` und in den jeweiligen Quelldateien.

`squeezelite-ALCD` ist ein unabhängiger Remix und keine offizielle Variante der
beiden Ursprungsprojekte.

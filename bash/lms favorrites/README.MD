# LMS-Favoriten aus Radio Browser

`LMS-fav.sh` sucht Radiosender über die API von
[Radio Browser](https://www.radio-browser.info/) und erzeugt daraus eine
Favoritendatei für Logitech Media Server beziehungsweise Lyrion Music Server.

Es werden ausschließlich Sender übernommen, deren Codec von der API als
`MP3` oder `OGG` angegeben wird. Links auf `.m3u`- und `.m3u8`-Dateien werden
übersprungen.

## Voraussetzungen

Das Skript benötigt:

- Bash
- `curl`
- `jq`

Unter Debian, Ubuntu oder Raspberry Pi OS können die benötigten Pakete so
installiert werden:

```bash
sudo apt update
sudo apt install curl jq
```

## Verwendung

Das Skript zunächst ausführbar machen:

```bash
chmod +x LMS-fav.sh
```

Anschließend den gewünschten Sendernamen als erstes Argument übergeben:

```bash
./LMS-fav.sh wdr
```

Ohne Argument wird ebenfalls nach `wdr` gesucht. Die Ausgabe heißt in diesem
Beispiel:

```text
favorites-wdr.opml
```

Ein eigener Dateiname kann als zweites Argument angegeben werden:

```bash
./LMS-fav.sh wdr favorites.opml
```

Suchbegriffe mit Leerzeichen müssen in Anführungszeichen stehen:

```bash
./LMS-fav.sh "radio bob" favorites-radio-bob.opml
```

Eine Übersicht aller Aufrufmöglichkeiten zeigt:

```bash
./LMS-fav.sh --help
```

## Anzahl und Startposition der Ergebnisse

In der Voreinstellung fragt das Skript zehn Ergebnisse ab Position zehn ab.
Das entspricht `offset=10` und `limit=10` aus der ursprünglichen API-Abfrage.

Mit den Umgebungsvariablen `OFFSET` und `LIMIT` lässt sich dies ändern. Für die
ersten 100 Treffer:

```bash
OFFSET=0 LIMIT=100 ./LMS-fav.sh wdr favorites.opml
```

Die Ergebnisse werden nach der Klickzahl absteigend sortiert. Als defekt
markierte Sender werden von Radio Browser nicht geliefert.

## Erzeugtes Format

Die Ausgabedatei ist eine XML-Datei im OPML-Format. Jeder unterstützte Sender
wird als LMS-Audiofavorit eingetragen:

```xml
<outline URL="http://example.org/stream.mp3" text="Beispielsender" type="audio" />
```

Sonderzeichen in Sendernamen und URLs werden automatisch XML-konform maskiert.
Einträge ohne Namen oder Stream-URL werden ausgelassen.

## In LMS verwenden

Vor dem Ersetzen einer vorhandenen `favorites.opml` sollte immer eine
Sicherungskopie angelegt werden. Danach kann die erzeugte Datei als
`favorites.opml` in das Voreinstellungsverzeichnis des LMS kopiert werden.

Der genaue Speicherort hängt von Installation und Betriebssystem ab. Bei einer
typischen Linux-Installation liegt die Datei häufig unter:

```text
/var/lib/squeezeboxserver/prefs/favorites.opml
```

Bei Docker-Installationen ist stattdessen das eingebundene `prefs`-Verzeichnis
des Containers zu verwenden. LMS muss gegebenenfalls neu gestartet werden,
damit die geänderte Favoritendatei eingelesen wird.

> **Achtung:** Das Kopieren als `favorites.opml` ersetzt die vorhandene
> Favoritenliste. Vorher unbedingt eine Sicherung erstellen.

## Mehrere OPML-Dateien zusammenführen

Mit `LMS-merge.sh` lassen sich mehrere zuvor erzeugte Favoritendateien zu einer
großen OPML-Datei zusammenführen. Das Skript benötigt Bash und Python 3.

Beide Skripte ausführbar machen:

```bash
chmod +x LMS-fav.sh LMS-merge.sh
```

Danach zuerst die Ausgabedatei und anschließend mindestens zwei Eingabedateien
angeben:

```bash
./LMS-merge.sh favorites-gesamt.opml favorites-wdr.opml favorites-ndr.opml
```

Beliebig viele Eingabedateien sind möglich:

```bash
./LMS-merge.sh favorites-gesamt.opml \
  favorites-wdr.opml \
  favorites-ndr.opml \
  favorites-swr.opml
```

Auch ein Platzhalter kann verwendet werden. Dabei darf die Ausgabedatei nicht
bereits auf den Platzhalter passen, weil sie sonst zugleich als Eingabedatei
übergeben würde:

```bash
./LMS-merge.sh gesamt.opml favorites-*.opml
```

Das Skript prüft alle Eingabedateien auf gültiges XML und eine vorhandene
OPML-`body`-Sektion. Favoriten mit derselben Stream-URL werden nur einmal in
die Gesamtliste übernommen. Die Zieldatei wird erst nach erfolgreicher
Verarbeitung aller Eingabedateien ersetzt.

VDR custom – 2026-11-13

Basis: das auf root@vdr bereits eingesetzte lapicidae-Image, per Digest fixiert:
ghcr.io/lapicidae/vdr-server@sha256:65da77dcdbd5ba4bfa8500dac6299f17b335ce14987ba5fbd58091921a6613d1

Änderungen:
- svdrposd 1.0.0-33 wird beim Imagebau aus dem mitgelieferten PKGBUILD gebaut.
  Die Quellen werden durch dessen SHA-256-Prüfsumme kontrolliert.
- dvbapi, epgsearch, live, streamdev-server und vnsiserver sind im Basisimage
  enthalten. Der Build prüft alle sechs Pakete und schreibt die Versionen nach
  /etc/vdr-custom-packages.txt.
- Keine rekursiven Rechteänderungen an /srv/vdr (Aufnahmen) oder Timeshift.
- Keine globale Suche nach Dateien bei Änderungen von PUID/PGID.
- Andere initiale Rechteanpassungen, insbesondere für Konfiguration und Geräte,
  bleiben erhalten. Aufnahmen und Timeshift müssen vorher für UID/GID 1000
  auf Host/NAS passend freigegeben sein. Das Image repariert NAS-Rechte nicht.
- Keine Internetprobe, Paketdatenbankaktualisierung, Paketinstallation oder
  benutzerdefinierten PKGBUILD-Bauten beim Start. PLUGINS aktiviert ausschließlich
  bereits eingebaute Plugins; fehlende Plugins führen zu einem klaren Fehler.
- Der optionale Onlineabgleich der epgsearch-Mailvorlage entfällt.
- Die s6-Abhängigkeiten bleiben erhalten.

Bauen (x86_64-Linux mit Docker):
  docker build --progress=plain -t vdr-server-custom:2026-11-13 .

Das Basisimage bleibt bewusst unverändert. Dies ist kein minimales Image und
kein vollständiger Arch-Neubau. Updates erfolgen durch einen neuen geprüften
Basis-Digest und einen neuen Build, nicht im Containerstart. Buildwerkzeuge des
Basisimages bleiben enthalten. Der Pluginquellcode muss beim Build erreichbar
sein; der normale Start benötigt dafür keinen Internetzugang.

Produktivstatus (2026-11-13):
Das Image wurde nach Zustimmung auf root@vdr aktiviert und dauerhaft in
/data/docker.compose/VDR/docker-compose.yml eingetragen.
Die vorherige Compose-Datei liegt auf dem Server unter:
/data/docker.compose/VDR/docker-compose.yml.before-custom-20261113

Normale Verwaltung:
  cd /data/docker.compose/VDR
  docker compose up -d

Die zusätzlich angelegte compose.custom.yaml ist nicht mehr erforderlich;
sie verweist ebenfalls auf dasselbe angepasste Image.

Rückkehr zum ursprünglichen Image (unterbricht Aufnahmen/Streams):
  cd /data/docker.compose/VDR
  docker compose -f docker-compose.yml.before-custom-20261113 up -d --no-deps vdr-server
Danach muss für eine dauerhafte Rückkehr die gesicherte Compose-Datei wieder
als docker-compose.yml hergestellt werden. Der Rollback startet wieder mit den
ursprünglichen Initialisierungsskripten einschließlich ihrer Rechteänderungen.

Produktivprüfung:
- rund 10 Sekunden vom Containerstart bis zum VDR-Aufruf
- alle sechs Plugins aktiviert, zwei DVB-Geräte initialisiert
- Healthcheck erfolgreich
- Live HTTP 307 (Weiterleitung), Streamdev HTTP 200
- VNSI und SVDRP TCP erreichbar, keine Containerneustarts
- keine Aufnahmeberechtigungsfehler oder Paketbauten beim Start
Bild/Ton und eine tatsächliche Aufnahme sind nicht unabhängig geprüft.

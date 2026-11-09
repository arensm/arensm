# PogoPlug E02 Player — V3.3

[English](#english) · [Deutsch](#deutsch)

## English

Installation and first setup for **[2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip](./2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip)**.

### New in V3.3

V3.3 extends the **cog / Setup** menu. Its image is named `2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip`; the installation instructions below refer to this V3.3 image.

| Feature | Function |
|---|---|
| **LMS server** | Enter a hostname or IP address, optionally with a port. Leave empty for automatic discovery. Saved as `LMS_SERVER` in `/boot/squeezelite.conf`. |
| **Audio device selection** | Choose an output from `squeezelite -l`. The list reloads when Setup opens and includes device names and descriptions. `default` remains available and is the initial setting. Saved as `AUDIO_DEVICE` in `/boot/squeezelite.conf`. |
| **Public SSH keys** | View and edit `/root/.ssh/authorized_keys`. The field replaces the file when saved. Enter one public RSA, Ed25519 or ECDSA key per line, without options; keep your own access key. At least one valid key must remain. Never enter private keys. |
| **Reboot / Power off** | Restart or shut down the PogoPlug after confirmation. To start it again after poweroff, disconnect and reconnect power. |

Press **Save** to apply settings. Changing the LMS server or audio device briefly restarts a running Squeezelite; the other audio options in the configuration file are retained. SSH directory/file permissions are maintained as `700` / `600`.

All new labels are available in **English and German**. Hostname/player naming, the U-Boot version display, LED behavior and the rule that every reboot starts Squeezelite are retained. The image carries the same authorized public keys as V3.2 until you edit them.

The web interface has no login. Anyone who can reach it over the network can change these settings, including SSH access; use it within a trusted local network.

### Requirements

- A **PogoPlug E02** with an existing bootloader configured to boot Linux from USB.
- A USB stick with at least **100 MB usable capacity**. Larger sticks work; the image initially uses only 100 MB.
- An Ethernet connection to a network with DHCP.
- A USB sound card or USB DAC and speakers/amplifier.
- A Lyrion/Logitech Media Server (LMS) for Squeezelite, or an AirPlay source for classic AirPlay.

### 1. Extract and check the image

Download the ZIP file and open a terminal in its download directory. Extract only the disk image:

```sh
unzip 2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip 'pogo-e02-v3.3-ext2-100MB.img'
```

The extracted image is exactly **100,000,000 bytes**. Its SHA256 is:

```text
7e6db69124e1046f0ba0aae1fa734bc11822d04a5609343dce309c42d4bbd11c
```

Check it on Linux:

```sh
sha256sum pogo-e02-v3.3-ext2-100MB.img
```

Or on macOS:

```sh
shasum -a 256 pogo-e02-v3.3-ext2-100MB.img
```

### 2. Write the USB stick

**Writing the image erases all data on the selected USB drive. Identify it by size and model before continuing.** Write the extracted `.img` file to the whole drive. The image already contains the partition table and ext2 filesystem.

#### Linux

List the drives:

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS
```

Replace `/dev/sdX` with your USB drive. Unmount every mounted partition on that drive; the following example unmounts its first partition:

```sh
sudo umount /dev/sdX1
```

Then write the image:

```sh
sudo dd if=pogo-e02-v3.3-ext2-100MB.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

Wait for completion, then safely eject the stick.

#### macOS

Find the USB drive:

```sh
diskutil list
```

Replace `N` with the correct disk number in all commands below:

```sh
diskutil unmountDisk /dev/diskN
sudo dd if=pogo-e02-v3.3-ext2-100MB.img of=/dev/rdiskN bs=4m
sync
diskutil eject /dev/diskN
```

During `dd`, press **Ctrl+T** to display progress.

### 3. Start the PogoPlug

1. With the PogoPlug powered off, connect the USB stick and USB sound card/DAC.
2. Connect Ethernet and the audio output to your amplifier or speakers.
3. Leave additional USB storage devices disconnected for the first boot.
4. Connect power and allow the system to start.
5. Find the PogoPlug's DHCP address in your router; its default hostname is `pogo-e02`.
6. Open **`http://<PogoPlug-IP>/`** in a browser on the same network.

### 4. Web interface

- **DE / EN:** switch the interface language using the button next to the cog. The browser remembers your selection.
- **Setup:** set the hostname, Squeezelite name and AirPlay name, then save. These settings survive a reboot.
- **Squeezelite → Turn on:** select the player in LMS and start playback. Its initial name is **PogoPlug E02**.
- **AirPlay → Turn on:** select **PogoPlug E02 AirPlay**, or your configured name, on your AirPlay source.

Only one player service is selected at a time. Turning on the other player switches services and hands over the USB audio device. The active button shows **On** and is disabled.

**Every reboot starts Squeezelite; AirPlay starts when you select it in the web interface.**

### 5. Server, audio and SSH setup

This image is configured with:

| Setting | Default |
|---|---|
| LMS server | `lms.lan` |
| Squeezelite audio device | `default` |
| AirPlay audio device | `default` |

Make sure `lms.lan` resolves to your LMS, or change `LMS_SERVER` in `/boot/squeezelite.conf`.

Squeezelite uses `AUDIO_DEVICE="default"` in `/boot/squeezelite.conf`; keep this setting for the default audio output. AirPlay also uses `output_device = "default";` in `/etc/shairport-sync.conf`. The web audio-device selector changes Squeezelite only. If a separate AirPlay output is required, configure it in that file and restart AirPlay.

For console setup, use a **3.3 V TTL serial adapter**, **115200 baud, 8N1, no flow control**. Log in as `root`; the initial console password is empty. The IP address can also be checked there:

```sh
ip addr show eth0
```

SSH uses public-key authentication only. **Replace the existing entry in `/root/.ssh/authorized_keys` with your own public key** when setting up your device. Keep `/root/.ssh` at mode `700` and `authorized_keys` at `600`.

### Front LED

| State | LED |
|---|---|
| Linux / player starting | Green blinking: 250 ms on, 250 ms off |
| Squeezelite running | Solid green |
| AirPlay running | Orange flash every 3 seconds: 250 ms on, 2750 ms off; green off |
| No player running, or system shut down while power remains connected | Solid orange |

The LED indicates the active player service; green also remains on when Squeezelite is idle. The indication before Linux starts depends on the existing bootloader.

Before removing power or the boot stick, shut the system down from the console:

```sh
poweroff
```

---

## Deutsch

Installation und erste Einrichtung für **[2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip](./2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip)**.

### Neu in V3.3

V3.3 erweitert das Menü **Zahnrad / Einstellungen**. Das Image heißt `2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip`; die nachfolgende Installationsanleitung bezieht sich auf dieses V3.3-Image.

| Funktion | Beschreibung |
|---|---|
| **LMS-Server** | Hostname oder IP-Adresse eintragen, optional mit Port. Ein leerer Eintrag aktiviert die automatische Suche. Speicherung als `LMS_SERVER` in `/boot/squeezelite.conf`. |
| **Audio-Geräteauswahl** | Ausgabe aus `squeezelite -l` auswählen. Beim Öffnen der Einstellungen wird die Liste mit Gerätenamen und Beschreibungen neu eingelesen. `default` bleibt verfügbar und ist voreingestellt. Speicherung als `AUDIO_DEVICE` in `/boot/squeezelite.conf`. |
| **Öffentliche SSH-Schlüssel** | `/root/.ssh/authorized_keys` anzeigen und bearbeiten. Das Feld ersetzt beim Speichern den Dateiinhalt. Ein öffentlicher RSA-, Ed25519- oder ECDSA-Schlüssel pro Zeile, ohne Optionen; den eigenen Zugangsschlüssel beibehalten. Mindestens ein gültiger Schlüssel muss verbleiben. Keine privaten Schlüssel eingeben. |
| **Neu starten / Ausschalten** | Den PogoPlug nach einer Bestätigung neu starten oder herunterfahren. Nach dem Ausschalten zum Einschalten die Stromversorgung kurz trennen und wieder verbinden. |

Mit **Speichern** werden die Einstellungen übernommen. Änderungen am LMS-Server oder Audio-Gerät starten einen laufenden Squeezelite kurz neu; die übrigen Audio-Optionen in der Konfigurationsdatei bleiben erhalten. Die Rechte für SSH-Verzeichnis und Schlüsseldatei werden auf `700` / `600` gehalten.

Alle neuen Beschriftungen sind auf **Deutsch und Englisch** vorhanden. Hostname und Playernamen, U-Boot-Versionsanzeige, LED-Verhalten und der automatische Start von Squeezelite nach jedem Neustart bleiben erhalten. Das Image enthält bis zur eigenen Bearbeitung dieselben autorisierten öffentlichen Schlüssel wie V3.2.

Die Weboberfläche hat keine Anmeldung. Wer sie im Netzwerk erreichen kann, kann diese Einstellungen einschließlich der SSH-Zugänge ändern; sie ist für ein vertrauenswürdiges lokales Netzwerk vorgesehen.

### Voraussetzungen

- Ein **PogoPlug E02** mit vorhandenem Bootloader, der für den Linux-Start von USB eingerichtet ist.
- Ein USB-Stick mit mindestens **100 MB nutzbarer Kapazität**. Größere Sticks funktionieren; das Image nutzt zunächst nur 100 MB.
- Eine Ethernet-Verbindung zu einem Netzwerk mit DHCP.
- Eine USB-Soundkarte oder ein USB-DAC sowie Lautsprecher/Verstärker.
- Ein Lyrion/Logitech Media Server (LMS) für Squeezelite oder eine AirPlay-Quelle für klassisches AirPlay.

### 1. Image entpacken und prüfen

Die ZIP-Datei herunterladen und im Downloadverzeichnis ein Terminal öffnen. Nur das Laufwerksimage entpacken:

```sh
unzip 2026.11.09-pogo-e02-v3.3-ext2-100MB.img.zip 'pogo-e02-v3.3-ext2-100MB.img'
```

Das entpackte Image ist exakt **100.000.000 Byte** groß. Seine SHA256-Prüfsumme lautet:

```text
7e6db69124e1046f0ba0aae1fa734bc11822d04a5609343dce309c42d4bbd11c
```

Unter Linux prüfen:

```sh
sha256sum pogo-e02-v3.3-ext2-100MB.img
```

Oder unter macOS:

```sh
shasum -a 256 pogo-e02-v3.3-ext2-100MB.img
```

### 2. USB-Stick schreiben

**Das Schreiben löscht alle Daten auf dem gewählten USB-Laufwerk. Vorher anhand von Größe und Modell das richtige Laufwerk bestimmen.** Die entpackte `.img`-Datei auf das gesamte Laufwerk schreiben. Partitionstabelle und ext2-Dateisystem sind bereits im Image enthalten.

#### Linux

Laufwerke anzeigen:

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS
```

`/dev/sdX` durch das USB-Laufwerk ersetzen. Alle eingehängten Partitionen dieses Laufwerks aushängen; das folgende Beispiel hängt die erste Partition aus:

```sh
sudo umount /dev/sdX1
```

Danach das Image schreiben:

```sh
sudo dd if=pogo-e02-v3.3-ext2-100MB.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

Das Ende des Schreibvorgangs abwarten und den Stick anschließend sicher auswerfen.

#### macOS

USB-Laufwerk ermitteln:

```sh
diskutil list
```

`N` in allen folgenden Befehlen durch die richtige Laufwerksnummer ersetzen:

```sh
diskutil unmountDisk /dev/diskN
sudo dd if=pogo-e02-v3.3-ext2-100MB.img of=/dev/rdiskN bs=4m
sync
diskutil eject /dev/diskN
```

Während `dd` lässt sich der Fortschritt mit **Ctrl+T** anzeigen.

### 3. PogoPlug starten

1. Bei ausgeschaltetem PogoPlug den USB-Stick und die USB-Soundkarte/den USB-DAC anschließen.
2. Ethernet und die Audioausgabe mit Verstärker oder Lautsprechern verbinden.
3. Weitere USB-Speicher beim ersten Start abziehen.
4. Stromversorgung anschließen und den Systemstart abwarten.
5. Die per DHCP vergebene IP-Adresse im Router nachsehen; der voreingestellte Hostname lautet `pogo-e02`.
6. Im Browser desselben Netzwerks **`http://<PogoPlug-IP>/`** öffnen.

### 4. Weboberfläche

- **DE / EN:** Die Sprache über den Schalter neben dem Zahnrad wechseln. Der Browser merkt sich die Auswahl.
- **Zahnrad / Einstellungen:** Hostname sowie Squeezelite- und AirPlay-Name eintragen und speichern. Diese Einstellungen bleiben nach einem Neustart erhalten.
- **Squeezelite → Einschalten:** Den Player im LMS auswählen und die Wiedergabe starten. Sein voreingestellter Name lautet **PogoPlug E02**.
- **AirPlay → Einschalten:** Auf der AirPlay-Quelle **PogoPlug E02 AirPlay** oder den selbst vergebenen Namen auswählen.

Es ist jeweils ein Playerdienst ausgewählt. Das Einschalten des anderen Players wechselt den Dienst und übergibt die USB-Audioausgabe. Der aktive Button zeigt **Eingeschaltet** und ist deaktiviert.

**Nach jedem Geräteneustart startet Squeezelite; AirPlay wird über die Weboberfläche eingeschaltet.**

### 5. Server, Audio und SSH einrichten

Dieses Image verwendet folgende Vorgaben:

| Einstellung | Vorgabe |
|---|---|
| LMS-Server | `lms.lan` |
| Squeezelite-Audiogerät | `default` |
| AirPlay-Audiogerät | `default` |

`lms.lan` muss auf den eigenen LMS zeigen. Alternativ `LMS_SERVER` in `/boot/squeezelite.conf` anpassen.

Squeezelite verwendet `AUDIO_DEVICE="default"` in `/boot/squeezelite.conf`; diese Einstellung für die Standard-Audioausgabe beibehalten. AirPlay verwendet ebenfalls `output_device = "default";` in `/etc/shairport-sync.conf`. Die Audio-Geräteauswahl im Webinterface ändert nur Squeezelite. Eine abweichende AirPlay-Ausgabe bei Bedarf in dieser Datei eintragen und AirPlay neu starten.

Für die Einrichtung an der Konsole einen **3,3-V-TTL-Serielladapter** mit **115200 Baud, 8N1, ohne Flusskontrolle** verwenden. Anmeldung als `root`; das anfängliche Konsolenpasswort ist leer. Die IP-Adresse lässt sich dort ebenfalls anzeigen:

```sh
ip addr show eth0
```

SSH erlaubt ausschließlich die Anmeldung per Schlüssel. **Den vorhandenen Eintrag in `/root/.ssh/authorized_keys` bei der Einrichtung durch den eigenen öffentlichen Schlüssel ersetzen.** `/root/.ssh` benötigt die Rechte `700`, `authorized_keys` die Rechte `600`.

### Front-LED

| Zustand | LED |
|---|---|
| Linux / Player startet | Grün blinkend: 250 ms an, 250 ms aus |
| Squeezelite läuft | Grün dauerhaft |
| AirPlay läuft | Orange blinkt alle 3 Sekunden: 250 ms an, 2750 ms aus; Grün aus |
| Kein Player läuft oder System heruntergefahren, Stromversorgung noch angeschlossen | Orange dauerhaft |

Die LED zeigt den aktiven Playerdienst an; Grün bleibt auch bei pausierter Squeezelite-Wiedergabe an. Die Anzeige vor dem Linux-Start hängt vom vorhandenen Bootloader ab.

Vor dem Trennen der Stromversorgung oder Abziehen des Bootsticks das System an der Konsole herunterfahren:

```sh
poweroff
```

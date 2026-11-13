# VDR Server Custom

A custom image based on [lapicidae/vdr-server](https://github.com/lapicidae/vdr-server), with preinstalled plugins and a faster startup path. It keeps VDR 2.8.2 and the existing configuration layout while moving plugin compilation out of container startup.

The image is built locally as `vdr-server-custom:2026-11-13`. This tag is not published to a container registry.

## What changes

- Builds and installs **svdrposd 1.0.0-33 during the image build**. Its source archive is checked against the SHA-256 checksum in [`plugin/PKGBUILD`](plugin/PKGBUILD).
- Uses the **epgsearch, live, streamdev-server and vnsiserver** packages already installed in the base image.
- Removes recursive ownership and permission changes for `/srv/vdr`, which contains the recording directory, and `/vdr/timeshift`.
- Replaces the global filesystem search previously used when changing the VDR user or group ID.
- Disables startup internet probes, package database refreshes, runtime package installation, and custom PKGBUILD builds.
- Removes the optional online migration check for the epgsearch email template.
- Preserves the s6 service dependency graph and other upstream initialization steps.

The image still adjusts permissions for configuration, cache, logos, logs and DVB devices. It retains the base image's build tools and is not a minimal runtime image. The `PLUGINS` variable selects installed plugins; it no longer downloads missing ones.

## Requirements

- An x86_64 Linux Docker host and Docker Compose v2.
- Internet access during the build to retrieve the base image, Dockerfile frontend and plugin sources.
- DVB devices on the host for direct tuner reception, or an independently configured alternative input.
- Recording and timeshift directories that the configured VDR UID/GID can already access.

Defaults are `PUID=1000`, `PGID=1000` and `TZ=Europe/Berlin`. For NFS or SMB storage, configure the appropriate server permissions, ACLs or mount options. This image deliberately does not repair recording or timeshift permissions at startup.

Normal startup does not need internet access for package management. TV sources or other services may still require network access.

## Build the image

Run these commands on the Docker host from this directory, which contains the Dockerfile:

```bash
docker build --progress=plain -t vdr-server-custom:2026-11-13 .
```

The original server build used `--network=host`. Add that option if the build needs the Linux host's network configuration:

```bash
docker build --network=host --progress=plain \
  -t vdr-server-custom:2026-11-13 .
```

The Dockerfile pins the base image to:

```text
ghcr.io/lapicidae/vdr-server@sha256:65da77dcdbd5ba4bfa8500dac6299f17b335ce14987ba5fbd58091921a6613d1
```

VDR itself is not rebuilt. The build compiles svdrposd against the base image's VDR API, installs its package, verifies the five required plugin packages, and copies in the startup changes. Installed versions are recorded in `/etc/vdr-custom-packages.txt`.

Inspect that manifest without starting VDR:

```bash
docker run --rm --network none --entrypoint /bin/cat \
  vdr-server-custom:2026-11-13 /etc/vdr-custom-packages.txt
```

Build the image on the deployment host, or transfer it to another x86_64 Docker host:

```bash
# On the build host:
docker save vdr-server-custom:2026-11-13 | gzip > vdr-server-custom-2026-11-13.tar.gz

# After transferring the archive to the deployment host:
gunzip -c vdr-server-custom-2026-11-13.tar.gz | docker load
```

## Use an existing Compose setup

Keep the existing VDR volumes, ports, devices and other settings. Recreating the container interrupts active recordings and streams; choose an appropriate maintenance window.

### Option 1: Change the main Compose file

From the directory containing your existing `docker-compose.yml`, make a backup using a filename that does not already exist:

```bash
cp -p docker-compose.yml docker-compose.yml.before-custom
```

Change the image of the existing `vdr-server` service and set the pull policy for the locally built image:

```yaml
services:
  vdr-server:
    image: vdr-server-custom:2026-11-13
    pull_policy: never
    # Keep your existing environment, volumes, ports, devices and other settings.
```

This is a fragment to merge into the existing service, not a complete Compose file. Validate and deploy it:

```bash
docker compose -f docker-compose.yml config --quiet
docker compose -f docker-compose.yml up -d --no-deps vdr-server
```

### Option 2: Use the supplied override

The supplied [`compose.override.yaml`](compose.override.yaml) changes only the image and pull policy. Its service name must match the existing service, which is `vdr-server` in this setup.

From the directory containing the original Compose file, replace `/path/to/vdr-server-custom` below with this project's location:

```bash
docker compose -f docker-compose.yml \
  -f /path/to/vdr-server-custom/compose.override.yaml config --quiet

docker compose -f docker-compose.yml \
  -f /path/to/vdr-server-custom/compose.override.yaml \
  up -d --no-deps vdr-server
```

Use both files for subsequent deployments with this option. The explicit `-f` arguments also avoid relying on automatic override-file discovery.

## Configuration

Default plugin selection:

```yaml
environment:
  PUID: "1000"
  PGID: "1000"
  TZ: Europe/Berlin
  PLUGINS: "svdrposd epgsearch live streamdev-server vnsiserver"
```

| Container path | Purpose |
| --- | --- |
| `/vdr/system` | VDR startup configuration and plugin configuration fragments |
| `/vdr/config` | VDR settings, channel list and plugin data |
| `/vdr/recordings` | Recordings; permissions managed on the host or NAS |
| `/vdr/cache` | EPG and other cache data |
| `/vdr/timeshift` | Optional timeshift storage; permissions managed on the host or NAS |
| `/vdr/channellogos` | Optional channel logos |
| `/vdr/log` | Optional file logging with `LOG2FILE=true` |

The legacy `/vdr/pkgbuild` mount no longer triggers package builds. Additional plugins must be installed in the Dockerfile and included in a new image build. Setting `PLUGINS` alone will not install them.

The Live plugin is kept enabled unless `DISABLE_WEBINTERFACE=true` is set. Existing upstream options, such as `LOGO_COPY=false` to preserve your own channel logos, remain available where their startup code is retained.

## Verify the deployment

For the existing container named `VDR`:

```bash
docker logs --timestamps --tail 100 VDR
docker exec VDR vdrctl list --enabled
docker exec VDR /usr/local/bin/healthy
docker inspect VDR --format '{{.Config.Image}} {{.State.Status}}'
```

Check the actual services and clients:

- Live web interface: `http://HOST:8008/`.
- Streamdev HTTP interface: `http://HOST:3000/`.
- Kodi/VNSI: the configured host on TCP port `34890`.
- Verify picture, sound, timeshift and a short recording.

Do not expect internet checks, package database downloads, plugin compilation, or recording-directory permission changes in the startup log.

## Updates

Updates are built into a new image rather than installed during startup:

1. Review a newer upstream base image and its VDR API version.
2. Update the pinned `BASE_IMAGE` digest or pass a replacement with `--build-arg BASE_IMAGE=...`.
3. Check `plugin/PKGBUILD`; its current svdrposd package requires **VDR API 13**.
4. Review the startup patches against the new upstream scripts. The patch script contains layout checks and may intentionally fail when upstream changes.
5. Build with a new local tag, verify the plugins, then update the Compose image reference.
6. Keep the previous image and Compose configuration until playback and recordings have been verified.

A fixed base digest and checked plugin source make the inputs explicit, but do not guarantee a byte-for-byte identical rebuilt image. Do not replace the digest with a moving `latest` reference without reviewing compatibility.

## Rollback

If you used the main-file method, restore the backup created above and explicitly select it as the main Compose file:

```bash
cp -p docker-compose.yml.before-custom docker-compose.yml
docker compose -f docker-compose.yml config --quiet
docker compose -f docker-compose.yml up -d --no-deps vdr-server
```

If you used only the override method, deploy with the original file alone:

```bash
docker compose -f docker-compose.yml up -d --no-deps vdr-server
```

Both methods recreate the container and interrupt active use. The original image also restores its original startup behavior, including recursive permission changes and runtime plugin installation. Keep all data volumes when rolling back.

On the original deployment host, the pre-change backup was named `docker-compose.yml.before-custom-20261113`; use that filename instead of the generic backup name above when restoring that deployment.

## Recorded validation

The initial x86_64 build and deployment on November 13, 2026 verified:

- All five required plugins were installed and recognized by VDR.
- svdrposd had no missing shared-library dependencies.
- Offline initialization reached the VDR invocation in approximately 5–6 seconds with a fresh test configuration.
- Initialization worked with a read-only test recording mount; the test file's ownership and permissions remained unchanged.
- Requesting an unavailable plugin failed without attempting installation.
- Production startup reached the VDR invocation in approximately 10 seconds.
- The VDR healthcheck passed, both DVB devices initialized, and the expected service ports responded.

These timings measure startup to the VDR invocation, not guaranteed time to a playable picture. The isolated test had no DVB devices, so VDR subsequently exited because no primary device was available. Picture, sound and an actual recording were not independently verified during deployment.

## Project files

| File | Purpose |
| --- | --- |
| `Dockerfile` | Pinned base, plugin build and runtime modifications |
| `plugin/PKGBUILD` | svdrposd source, checksum and Arch package recipe |
| `patch-runtime.sh` | Changes to inherited startup tasks |
| `rootfs/` | Replacement UID/GID and plugin initialization scripts |
| `compose.override.yaml` | Optional image override for an existing service |
| `LICENSE.upstream` | Included upstream license text |

## Upstream and licensing

Based on [lapicidae/vdr-server](https://github.com/lapicidae/vdr-server). The svdrposd recipe originates from [VDR4Arch](https://github.com/VDR4Arch/vdr4arch), and its sources come from [vdr-plugin-svdrposd](https://github.com/vdr-projects/vdr-plugin-svdrposd).

See [`LICENSE.upstream`](LICENSE.upstream) and the respective upstream projects for licensing. Bundled packages retain their individual licenses.

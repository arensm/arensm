# syntax=docker/dockerfile:1
ARG BASE_IMAGE=ghcr.io/lapicidae/vdr-server@sha256:65da77dcdbd5ba4bfa8500dac6299f17b335ce14987ba5fbd58091921a6613d1
FROM ${BASE_IMAGE}
LABEL org.opencontainers.image.title="VDR custom: prebuilt plugins, fast startup"
ENV PLUGINS="svdrposd epgsearch live streamdev-server vnsiserver"     PUID=1000 PGID=1000 TZ=Europe/Berlin
COPY plugin/PKGBUILD /tmp/vdr-plugin-build/PKGBUILD
RUN chown -R builduser:users /tmp/vdr-plugin-build  && cd /tmp/vdr-plugin-build  && sudo -u builduser makepkg --clean --noconfirm  && pacman -U --noconfirm /tmp/vdr-plugin-build/vdr-svdrposd-*.pkg.tar.zst  && cp /etc/vdr/conf.avail/50-svdrposd.conf /defaults/system/conf.avail/  && pacman -Q vdr vdr-svdrposd vdr-epgsearch vdr-live vdr-streamdev-server vdr-vnsiserver > /etc/vdr-custom-packages.txt  && rm -rf /tmp/vdr-plugin-build
COPY patch-runtime.sh /tmp/patch-runtime.sh
RUN bash /tmp/patch-runtime.sh && rm /tmp/patch-runtime.sh
COPY rootfs/ /
RUN chmod 755 /etc/s6-overlay/scripts/vdr-plugins_up     /etc/s6-overlay/scripts/init-user_custom  && bash -n /etc/s6-overlay/scripts/vdr-plugins_up  && bash -n /etc/s6-overlay/scripts/init-user_custom

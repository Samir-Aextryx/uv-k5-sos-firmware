#!/bin/sh
# Baut die SOS-Edition (F4HWN + Schnellwahl + Samir-Logo). Aufruf in Git Bash:
#   ./build-sos.sh            -> compiled-firmware/f4hwn.sos.packed.bin           (empfohlen)
#   ./build-sos.sh spektrum   -> compiled-firmware/f4hwn.sos-spektrum.packed.bin  (mit Spektrum, ohne CHIRP)
# Voraussetzung: Docker-Image einmal bauen mit  docker build -t uvk5-build .
cd "$(dirname "$0")" || exit 1

COMMON="ENABLE_FMRADIO=1 ENABLE_AIRCOPY=0 ENABLE_FEAT_F4HWN_SCREENSHOT=0 ENABLE_FEAT_F4HWN_GAME=0 \
ENABLE_FEAT_F4HWN_GMRS_FRS_MURS=0 ENABLE_NOAA=0 ENABLE_FEAT_F4HWN_RESCUE_OPS=0 \
ENABLE_QUICKMENU=1 ENABLE_SAMIR_LOGO=1"

if [ "$1" = "spektrum" ]; then
    TG=sos-spektrum
    OPTS="ENABLE_SPECTRUM=1 ENABLE_VOX=0 ENABLE_FEAT_F4HWN_PMR=0 EDITION_STRING=SPEKTRUM \
ENABLE_UART=0 ENABLE_FEAT_F4HWN_SPECTRUM=0 ENABLE_AUDIO_BAR=0 ENABLE_FEAT_F4HWN_RESUME_STATE=0 \
ENABLE_FEAT_F4HWN_CTR=0 ENABLE_FEAT_F4HWN_SLEEP=0 ENABLE_FEAT_F4HWN_RX_TX_TIMER=0 \
ENABLE_FEAT_F4HWN_NARROWER=0 ENABLE_COPY_CHAN_TO_VFO=0 ENABLE_FEAT_F4HWN_INV=0 ENABLE_FEAT_F4HWN_CA=0 \
ENABLE_FASTER_CHANNEL_SCAN=0 ENABLE_SQUELCH_MORE_SENSITIVE=0 ENABLE_NO_CODE_SCAN_TIMEOUT=0 \
ENABLE_KEEP_MEM_NAME=0"
else
    TG=sos
    # abgeschaltet (einfache Bedienung, Platz im 60-KB-Speicher): VOX,
    # Schlafmodus, TX/RX-Timer, Mikrofon-Pegel, NARROWER, Kanal->VFO kopieren,
    # CA, Invertieren, Kontrast, Fortsetzen nach dem Einschalten, PMR-Sperrmodus
    OPTS="ENABLE_SPECTRUM=0 ENABLE_VOX=0 ENABLE_FEAT_F4HWN_PMR=0 EDITION_STRING=SOS ENABLE_FEAT_F4HWN_SLEEP=0 ENABLE_FEAT_F4HWN_RX_TX_TIMER=0 ENABLE_AUDIO_BAR=0 ENABLE_FEAT_F4HWN_NARROWER=0 ENABLE_COPY_CHAN_TO_VFO=0 ENABLE_FEAT_F4HWN_CA=0 ENABLE_FEAT_F4HWN_INV=0 ENABLE_FEAT_F4HWN_RESUME_STATE=0 ENABLE_FEAT_F4HWN_CTR=0"
fi

MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W 2>/dev/null || pwd):/src" uvk5-build /bin/bash -c "\
  rm -rf /tmp/b && cp -r /src /tmp/b && cd /tmp/b && make clean >/dev/null 2>&1; \
  make -s $COMMON $OPTS $EXTRA TARGET=f4hwn.$TG \
  && mkdir -p /src/compiled-firmware && cp f4hwn.$TG* /src/compiled-firmware/ && arm-none-eabi-size f4hwn.$TG"

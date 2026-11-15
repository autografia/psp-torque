#!/bin/sh
# Compila PSP Torque OBD con el toolchain pspdev en WSL, sin Docker.
#
# Requisitos: una distro WSL con el toolchain en /usr/local/pspdev
# (imagen pspdev/pspdev de Docker Hub, extraida en /mnt/d/pspdev-root).
#
# Uso (desde Windows):
#   wsl -d Debian -e sh /mnt/d/psp_torqueOBD/tools/build_wsl.sh
set -e
cd "$(dirname "$0")/.."

export PATH=/usr/local/pspdev/bin:$PATH

echo "== limpieza =="
make clean >/dev/null 2>&1 || true

echo "== compilacion =="
make 2>&1 | tee /tmp/psp_build.log | grep -E "error|warning|Error" || true

if grep -qE "error:|Error [0-9]" /tmp/psp_build.log; then
    echo "ERROR: la compilacion ha fallado"
    exit 1
fi

echo "== copia a dist/ =="
mkdir -p dist/PSPTorqueOBD
cp -f EBOOT.PBP dist/PSPTorqueOBD/
cp -f host.txt dist/PSPTorqueOBD/
cp -f RUN.txt   dist/PSPTorqueOBD/

echo "== SHA-256 =="
sha256sum EBOOT.PBP | cut -d' ' -f1

echo "== limpio =="
make clean >/dev/null 2>&1 || true
echo "OK: EBOOT.PBP en dist/PSPTorqueOBD/"

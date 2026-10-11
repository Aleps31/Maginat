#!/bin/bash
set -euo pipefail
mountpoint -q /mnt/lfs || { echo "Monta prima /mnt/lfs"; exit 1; }
cp -a /root/magi-overlay/. /mnt/lfs/
[ -d /mnt/lfs/home/utente ] && chown -R 1000:999 /mnt/lfs/home/utente
echo "Personalizzazioni di Maginat applicate"

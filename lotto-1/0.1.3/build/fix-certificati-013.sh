#!/bin/bash
set -e
LFS=/mnt/lfs
OVL=/root/magi-overlay
[ "$(id -u)" -eq 0 ] || { echo "Esegui come root"; exit 1; }
[ ! -d /sources ] || { echo "Sei dentro il chroot: esci con exit"; exit 1; }
mountpoint -q $LFS || { echo "Monta prima $LFS"; exit 1; }
[ -s $LFS/etc/ssl/certs/ca-certificates.crt ] || { echo "manca il file dei certificati"; exit 1; }
IP=$(getent ahostsv4 flathub.org | awk 'NR==1{print $1}')
echo "== prima =="
chroot $LFS /usr/bin/curl -sI --resolve flathub.org:443:$IP https://flathub.org | head -1 || true
echo "esito curl: ${PIPESTATUS[0]}"
echo "== divido il file in certificati singoli e creo i collegamenti =="
chroot $LFS /usr/bin/env LC_ALL=C /bin/bash -c '
set -e
cd /etc/ssl/certs
rm -f maginat-ca-*.pem
awk "/BEGIN CERTIFICATE/{n++} n>0{print > sprintf(\"maginat-ca-%03d.pem\", n)}" ca-certificates.crt
openssl rehash /etc/ssl/certs 2>&1 | grep -v "ca-certificates.crt" || true
echo "file maginat-ca: $(ls maginat-ca-*.pem | wc -l)"
echo "collegamenti: $(find . -maxdepth 1 -type l -name "*.[0-9]" | wc -l)"
'
echo "== dopo =="
chroot $LFS /usr/bin/curl -sI --resolve flathub.org:443:$IP https://flathub.org | head -1 || true
echo "esito curl: ${PIPESTATUS[0]}"
echo "== copia nell'overlay =="
mkdir -p $OVL/etc/ssl/certs
cp -a $LFS/etc/ssl/certs/. $OVL/etc/ssl/certs/
ls $OVL/etc/ssl/certs | wc -l
echo FATTO

#!/bin/bash
set -euo pipefail
LFS=/mnt/lfs
OVL=/root/magi-overlay
WORK=/root/magi-build-luks
REL=usr/share/maginat/initrd-luks.img
[ "$(id -u)" -eq 0 ] || { echo "ERRORE: serve root (usa: sudo -i)"; exit 1; }
[ ! -d /sources ] || { echo "ERRORE: sei dentro la chroot: esci con exit"; exit 1; }
mountpoint -q $LFS || mount -o loop /root/lfs.img $LFS
for m in dev proc sys run; do
  if mountpoint -q $LFS/$m; then echo "ERRORE: smonta $LFS/$m con /root/lfs-esci.sh"; exit 1; fi
done
[ -d "$OVL" ] || { echo "ERRORE: manca $OVL"; exit 1; }
command -v cpio >/dev/null || { echo "ERRORE: manca cpio (apt install cpio)"; exit 1; }
APPLETS="sh mount umount mkdir sleep echo cat switch_root loadkmap ls cttyhack setsid tr"
BB=""
for c in $(which -a busybox 2>/dev/null) /bin/busybox /usr/bin/busybox /usr/lib/initramfs-tools/bin/busybox; do
  [ -x "$c" ] || continue
  if ! file -L "$c" | grep -q 'statically linked'; then echo "scarto $c: non statico"; continue; fi
  MANCA=""
  ELENCO=$("$c" --list 2>/dev/null)
  for a in $APPLETS; do grep -qx "$a" <<< "$ELENCO" || MANCA="$MANCA $a"; done
  if [ -n "$MANCA" ]; then echo "scarto $c: mancano$MANCA"; continue; fi
  BB=$c
  break
done
[ -n "$BB" ] || { echo "ERRORE: nessun busybox statico completo: installa con: apt install busybox-static"; exit 1; }
echo "uso busybox: $BB"
printf 'x\n' | "$BB" sh -c 'IFS= read -rs p; [ "$p" = x ]' || { echo "ERRORE: questo busybox non supporta read -s"; exit 1; }
[ -x $LFS/usr/sbin/cryptsetup ] || { echo "ERRORE: manca cryptsetup nell'immagine"; exit 1; }
rm -rf "$WORK"
R=$WORK/ird
mkdir -p $R/bin $R/usr/lib $R/usr/sbin $R/dev $R/proc $R/sys $R/run $R/newroot $R/etc/kbd $R/lib64 $R/tmp
ln -s usr/lib $R/lib
ln -s usr/sbin $R/sbin
cp "$BB" $R/bin/busybox
for a in $APPLETS; do ln -s busybox $R/bin/$a; done
cp $LFS/usr/sbin/cryptsetup $R/usr/sbin/cryptsetup
chmod 755 $R/usr/sbin/cryptsetup
LIBS=$(chroot $LFS /usr/bin/ldd /usr/sbin/cryptsetup | sed -n 's|.*=> \(/[^ ]*\) .*|\1|p; s|^[[:space:]]*\(/lib64/[^ ]*\) .*|\1|p')
[ -n "$LIBS" ] || { echo "ERRORE: ldd non ha dato librerie"; exit 1; }
for l in $LIBS; do
  [ -e "$LFS$l" ] || { echo "ERRORE: manca $LFS$l"; exit 1; }
  cp -L "$LFS$l" "$R/usr/lib/$(basename "$l")"
done
[ -f $R/usr/lib/ld-linux-x86-64.so.2 ] || { echo "ERRORE: manca ld-linux"; exit 1; }
ln -s ../usr/lib/ld-linux-x86-64.so.2 $R/lib64/ld-linux-x86-64.so.2
for k in us uk it de-latin1 fr es; do
  chroot $LFS /bin/bash -c "loadkeys -b $k" > $R/etc/kbd/$k.bkmap 2>/dev/null || true
  if [ "$(stat -c %s $R/etc/kbd/$k.bkmap)" -lt 500 ]; then echo "ATTENZIONE: tastiera $k non generata"; rm -f $R/etc/kbd/$k.bkmap; fi
done
mknod -m 600 $R/dev/console c 5 1
mknod -m 666 $R/dev/null c 1 3
cat > $R/init << 'INITEOF'
#!/bin/sh
export PATH=/bin:/usr/sbin:/sbin
export LD_LIBRARY_PATH=/usr/lib
export DM_DISABLE_UDEV=1
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
exec </dev/console >/dev/console 2>&1
LUKS=""
KBD=us
for a in $(cat /proc/cmdline); do
  case "$a" in
    maginat.luks=*) LUKS=${a#*=};;
    maginat.kbd=*) KBD=${a#*=};;
  esac
done
if [ -f /etc/kbd/$KBD.bkmap ]; then loadkmap < /etc/kbd/$KBD.bkmap || echo "Maginat: keyboard layout not loaded"; fi
emergenza() {
  echo "Maginat: $1"
  echo "Maginat: emergency shell"
  exec setsid cttyhack /bin/sh
}
[ -n "$LUKS" ] || emergenza "missing maginat.luks parameter"
trova() {
  for d in /dev/sd* /dev/vd* /dev/xvd* /dev/nvme*n*p* /dev/mmcblk*p*; do
    [ -b "$d" ] || continue
    cryptsetup isLuks "$d" 2>/dev/null || continue
    if [ "$(cryptsetup luksUUID "$d" 2>/dev/null)" = "$LUKS" ]; then echo "$d"; return 0; fi
  done
  return 1
}
DEV=""
n=0
while [ -z "$DEV" ] && [ $n -lt 40 ]; do
  DEV=$(trova) || { n=$((n+1)); sleep 1; }
done
[ -n "$DEV" ] || emergenza "encrypted disk not found"
echo "Maginat: encrypted disk $DEV"
prova_pw() { printf '%s' "$1" | cryptsetup open -d - "$DEV" maginat-root 2>/dev/null; }
while true; do
  printf 'Maginat: disk password or recovery key: '
  IFS= read -rs PW
  echo
  if prova_pw "$PW"; then break; fi
  PW2=$(printf '%s' "$PW" | tr -d ' -' | tr 'A-Z' 'a-z')
  if [ "$PW2" != "$PW" ] && prova_pw "$PW2"; then break; fi
  echo "Maginat: wrong password, try again"
done
PW=""
PW2=""
mount -o ro /dev/mapper/maginat-root /newroot || emergenza "cannot mount the system"
umount /run 2>/dev/null
mkdir -p /newroot/dev /newroot/proc /newroot/sys
mount --move /dev /newroot/dev
mount --move /proc /newroot/proc
mount --move /sys /newroot/sys
exec switch_root /newroot /sbin/init
INITEOF
chmod 755 $R/init
"$BB" sh -n $R/init || { echo "ERRORE: sintassi init"; exit 1; }
( cd $R && find . | cpio -o -H newc -R 0:0 2>/dev/null | gzip -9 > $WORK/initrd-luks.img )
ls -l $WORK/initrd-luks.img
for d in $OVL $LFS; do
  mkdir -p $d/usr/share/maginat
  cp $WORK/initrd-luks.img $d/$REL
  cmp $WORK/initrd-luks.img $d/$REL
done
echo "contenuto:"
zcat $WORK/initrd-luks.img | cpio -t 2>/dev/null | grep -E 'cryptsetup|bkmap|lib.*\.so|init$' 
echo "FATTO: initrd-luks.img in overlay e immagine"

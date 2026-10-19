#!/bin/bash
set -euo pipefail
LFS=/mnt/lfs
WORK=/root/magi-build
OUT=/root/maginat-0.1.1.iso

[ "$(id -u)" -eq 0 ] || { echo "Esegui come root"; exit 1; }
mountpoint -q $LFS || { echo "Monta prima $LFS"; exit 1; }
for m in dev proc sys run; do
  if mountpoint -q $LFS/$m; then echo "Smonta $LFS/$m con /root/lfs-esci.sh"; exit 1; fi
done
BUSYBOX=$(command -v busybox)
file "$BUSYBOX" | grep -q 'statically linked' || { echo "busybox non statico"; exit 1; }
command -v cpio >/dev/null || { echo "Manca cpio: apt install cpio"; exit 1; }

rm -rf "$WORK"
mkdir -p "$WORK"/initramfs "$WORK"/iso/boot/grub "$WORK"/iso/magi

/root/magi-applica.sh
echo "== 1/4 squashfs (alcuni minuti) =="
mksquashfs $LFS "$WORK/iso/magi/magi.squashfs" -comp xz -b 1M -Xbcj x86 -noappend -wildcards -e sources lost+found "tmp/*" root/.bash_history "usr/lib/libLLVM*.a" "usr/lib/libclang*.a" "usr/lib/libclang*.so*" "usr/include/llvm" "usr/include/llvm-c" "usr/include/clang" "usr/include/clang-c" "usr/lib/cmake/llvm" "usr/lib/cmake/clang" "usr/lib/clang" "usr/bin/clang*"

echo "== 2/4 initramfs =="
cd "$WORK/initramfs"
mkdir -p bin sbin dev proc sys iso ro rw newroot
cp "$BUSYBOX" bin/busybox
mkdir -p lib/firmware; cp -a $LFS/usr/lib/firmware/i915 lib/firmware/ 2>/dev/null || true
for a in sh mount umount mkdir sleep echo ls cat switch_root losetup; do ln -s busybox bin/$a; done
cat > init << 'INITEOF'
#!/bin/sh
export PATH=/bin:/sbin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
echo "Maginat: ricerca del supporto di avvio..."
found=""
n=0
while [ -z "$found" ] && [ $n -lt 15 ]; do
  for d in /dev/sr* /dev/sd* /dev/vd* /dev/nvme*; do
    [ -b "$d" ] || continue
    if mount -t iso9660 -o ro "$d" /iso 2>/dev/null; then
      if [ -f /iso/magi/magi.squashfs ]; then found="$d"; break; fi
      umount /iso
    fi
  done
  [ -n "$found" ] || { n=$((n+1)); sleep 1; }
done
if [ -z "$found" ]; then
  echo "Maginat: supporto di avvio non trovato. Shell di emergenza."
  exec /bin/sh
fi
echo "Maginat: trovato su $found"
umount /iso
L=$(losetup -f)
losetup -r $L $found
mount -t iso9660 -o ro $L /iso
mount -t squashfs -o ro,loop /iso/magi/magi.squashfs /ro
mount -t tmpfs tmpfs /rw
UP=/rw/upper; WK=/rw/work
case " $(cat /proc/cmdline) " in *" maginat.persist "*)
  echo "Maginat: ricerca della persistenza..."
  mkdir -p /p
  for d in /dev/sd?[0-9] /dev/vd?[0-9] /dev/nvme?n?p?; do
    [ -b "$d" ] || continue
    mount -t ext4 -o ro "$d" /p 2>/dev/null || continue
    if [ -f /p/.maginat-persist ] && mount -o remount,rw /p; then UP=/p/upper; WK=/p/work; echo "Maginat: persistenza su $d"; break; fi
    umount /p
  done;;
esac
mkdir -p $UP $WK
mount -t overlay overlay -o lowerdir=/ro,upperdir=$UP,workdir=$WK /newroot
echo "$found" > /newroot/etc/maginat-bootdev 2>/dev/null
k=${found#/dev/}
case "$k" in
  sd*|vd*) k=${k%%[0-9]*}; P="$k|$k[0-9]*";;
  *) P="$k";;
esac
mkdir -p /newroot/etc/udev/rules.d
echo "KERNEL==\"$P\", ENV{UDISKS_IGNORE}=\"1\"" > /newroot/etc/udev/rules.d/98-maginat-bootdisk.rules 2>/dev/null
mkdir -p /newroot/dev /newroot/proc /newroot/sys
mount --move /dev /newroot/dev
mount --move /proc /newroot/proc
mount --move /sys /newroot/sys
exec switch_root /newroot /sbin/init
INITEOF
chmod +x init
find . | cpio -o -H newc 2>/dev/null | gzip -9 > "$WORK/iso/boot/initrd.img"

echo "== 3/4 kernel e GRUB =="
cp $LFS/boot/vmlinuz-6.16.1-magi-0.1 "$WORK/iso/boot/vmlinuz"
cp /root/microcode.img "$WORK/iso/boot/microcode.img"
mkdir -p "$WORK/iso/boot/grub/themes/magi" "$WORK/iso/boot/grub/fonts"
cp "$LFS"/usr/share/maginat/grub/background.jpg "$LFS"/usr/share/maginat/grub/theme.txt "$LFS"/usr/share/maginat/grub/select_*.png "$WORK/iso/boot/grub/themes/magi/"
if [ -f /usr/share/grub/unicode.pf2 ]; then cp /usr/share/grub/unicode.pf2 "$WORK/iso/boot/grub/fonts/unicode.pf2"; fi
cp /root/grub-lingua.cfg "$WORK/iso/boot/grub/grub.cfg"; cp /root/grub-principale.cfg "$WORK/iso/boot/grub/principale.cfg"

echo "== 4/4 ISO =="
grub-mkrescue -o "$OUT" "$WORK/iso" 2>&1 | tail -n 5
ls -lh "$OUT"

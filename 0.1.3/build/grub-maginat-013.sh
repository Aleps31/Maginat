#!/bin/bash
SB=/home/alessandropisanu/Scaricati/Maginat
DISCO=$SB/qemu/disco-dual.img
IMGG=/mnt/lfs/usr/share/maginat/secureboot/grubx64-maginat.efi
OVLG=/root/magi-overlay/usr/share/maginat/secureboot/grubx64-maginat.efi

case "$1" in
costruisci)
  if [ ! -f /sources/grub-mem.tar ] || [ ! -f /usr/lib/grub/x86_64-efi/normal.mod ]; then
    echo "ERRORE: questa fase va lanciata DENTRO la chroot (prompt: (lfs chroot) root:/#)"
    exit 1
  fi
  cd /sources || exit 1
  printf 'sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md\ngrub,4,Free Software Foundation,grub,2.12,https://www.gnu.org/software/grub/\n' > grub-sbat.csv
  printf 'search --no-floppy --file --set=root /EFI/maginat/grub.cfg\nset prefix=($root)/EFI/maginat\nconfigfile ($root)/EFI/maginat/grub.cfg\n' > grub-early.cfg
  L=""
  for m in part_gpt part_msdos fat ext2 btrfs xfs ntfs normal configfile search search_fs_uuid search_fs_file search_label chain linux boot echo sleep test true loadenv efifwsetup all_video gfxterm gfxmenu gfxterm_background font png jpeg gettext terminal minicmd ls cat reboot halt memdisk tar cryptodisk luks luks2 gcry_rijndael gcry_sha256 gcry_sha512 gcry_sha1 pbkdf2 diskfilter lvm; do
    if [ -f /usr/lib/grub/x86_64-efi/$m.mod ]; then L="$L $m"; else echo "manca il modulo: $m"; fi
  done
  rm -f /sources/grubx64-maginat.efi
  # shellcheck disable=SC2086
  grub-mkimage -O x86_64-efi -d /usr/lib/grub/x86_64-efi -p /EFI/maginat -c /sources/grub-early.cfg -m /sources/grub-mem.tar --sbat /sources/grub-sbat.csv -o /sources/grubx64-maginat.efi $L || exit 1
  ls -l /sources/grubx64-maginat.efi
  objdump -h /sources/grubx64-maginat.efi | grep -i sbat
  echo "FATTO: ora esci dalla chroot con: exit"
  ;;
firma)
  if [ "$(id -u)" -ne 0 ]; then echo "ERRORE: serve root (usa: sudo -i)"; exit 1; fi
  if [ -d /sources ]; then echo "ERRORE: sei dentro la chroot: esci con exit"; exit 1; fi
  if pgrep -f disco-dual.img >/dev/null; then echo "ERRORE: QEMU e ancora aperto: chiudilo"; exit 1; fi
  mountpoint -q /mnt/lfs || mount -o loop /root/lfs.img /mnt/lfs
  [ -f /mnt/lfs/sources/grubx64-maginat.efi ] || { echo "ERRORE: manca /mnt/lfs/sources/grubx64-maginat.efi (fase costruisci non fatta)"; exit 1; }
  [ -d /root/magi-overlay ] || { echo "ERRORE: manca /root/magi-overlay"; exit 1; }
  trap 'rm -rf "$SB/chiavi-secureboot" "$SB/chiavi-aggiornamenti" "$SB/chiave-moduli-6.18.58.pem"' EXIT
  export GPG_TTY=$(tty)
  GPGF=$SB/v0.1.3/chiavi-maginat.tar.gz.gpg
  [ -f "$GPGF" ] || GPGF=$SB/chiavi-maginat.tar.gz.gpg
  [ -f "$GPGF" ] || { echo "ERRORE: manca chiavi-maginat.tar.gz.gpg"; exit 1; }
  gpg --pinentry-mode loopback -d "$GPGF" | tar -xzf - -C "$SB" || { echo "ERRORE: chiavi non estratte"; exit 1; }
  K=$SB/chiavi-secureboot
  sbsign --key $K/maginat-sb.key --cert $K/maginat-sb.crt --output $IMGG /mnt/lfs/sources/grubx64-maginat.efi || exit 1
  sbverify --cert $K/maginat-sb.crt $IMGG || exit 1
  rm -rf "$SB/chiavi-secureboot" "$SB/chiavi-aggiornamenti" "$SB/chiave-moduli-6.18.58.pem"
  cp $IMGG $OVLG && cmp $IMGG $OVLG && echo "overlay-ok"
  L=$(losetup -f --show -P "$DISCO") || exit 1
  sleep 2
  mkdir -p /mnt/dual-esp
  if mount ${L}p1 /mnt/dual-esp; then
    cp $IMGG /mnt/dual-esp/EFI/maginat/grubx64.efi && cp $IMGG /mnt/dual-esp/EFI/BOOT/grubx64.efi && echo "disco-ok"
    sync
    umount /mnt/dual-esp
  fi
  losetup -d "$L"
  losetup -a
  ls "$SB"
  ;;
*)
  echo "uso: grub-maginat-013.sh costruisci (nella chroot) | firma (nel Mint come root)"
  exit 1
  ;;
esac

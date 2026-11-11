#!/bin/bash
D=${D:-/home/alessandropisanu/Scaricati/Maginat/v0.1.3/sorgenti-013}
IMG=${IMG:-/mnt/lfs}
mkdir -p "$D"
scarica() {
  local dir=$1 pat=$2 base lista h n trovati=0
  for base in http://archive.ubuntu.com/ubuntu/pool/main http://security.ubuntu.com/ubuntu/pool/main; do
    lista=$(wget -q -O- "$base/$dir/" | grep -o 'href="[^"?/][^"]*"' | sed 's/^href="//; s/"$//')
    [ -n "$lista" ] || continue
    while IFS= read -r h; do
      n=$(printf '%s' "$h" | sed 's/%2[bB]/+/g')
      if printf '%s' "$n" | grep -qE "$pat"; then
        wget -q -c -O "$D/$n" "$base/$dir/$h" && { echo "scaricato: $n"; trovati=$((trovati+1)); }
      fi
    done <<< "$lista"
    [ "$trovati" -gt 0 ] && break
  done
  [ "$trovati" -gt 0 ] || echo "NIENTE per $dir ($pat)"
}
scarica g/grub2 '^grub2_2\.12(-1ubuntu7\.3\.(dsc|debian\.tar\.[a-z0-9]+)|\.orig\.tar\.[a-z0-9]+)$'
scarica g/grub2-signed '^grub2-signed_1\.202\.5\.(dsc|tar\.[a-z0-9]+)$'
scarica s/shim-signed '^shim-signed_1\.58\.(dsc|tar\.[a-z0-9]+)$'
scarica s/shim '^shim_15\.8(-0ubuntu[0-9]+\.(dsc|debian\.tar\.[a-z0-9]+)|\.orig\.tar\.[a-z0-9]+)$'
cp "$IMG/sources/hplip-3.25.8.tar.gz" "$D/" && echo "copiato: hplip-3.25.8.tar.gz"
scarica b/busybox '^busybox_1\.36\.1(-6ubuntu3\.1\.(dsc|debian\.tar\.[a-z0-9]+)|\.orig\.tar\.[a-z0-9]+)$'
for f in linux-6.18.62.tar.xz openssl-3.5.9.tar.gz curl-8.23.0.tar.xz openssh-10.6p2.tar.gz libpng-1.6.59.tar.xz xz-5.8.4.tar.xz git-2.47.3.tar.xz zlib-1.3.2.tar.xz Python-3.13.18.tar.xz ntfs-3g_ntfsprogs-2022.10.3.tgz tumbler-4.20.2.tar.bz2 ffmpegthumbnailer-2.2.3.tar.gz efivar-39.tar.gz efibootmgr-18.tar.gz cryptsetup-2.8.1.tar.xz librsvg-2.40.21.tar.xz b43-fwcutter-019.tar.bz2 b43-fwcutter-019.tar.bz2.asc; do
  if [ -f "$IMG/sources/$f" ]; then cp "$IMG/sources/$f" "$D/" && echo "copiato: $f"; else echo "MANCA: $f"; fi
done
cp "$IMG/boot/config-6.18.62" "$D/config-6.18.62-maginat-0.1.3" && echo "copiato: config del kernel"
if [ -f "$D/config-6.18.58-maginat-0.1.3" ]; then
  mkdir -p "$D.vecchi" && mv "$D/config-6.18.58-maginat-0.1.3" "$D.vecchi/" && echo "spostata la vecchia config 6.18.58 in $D.vecchi"
fi
chown -R alessandropisanu: "$D" "$D.vecchi" 2>/dev/null
echo
ls -l "$D"

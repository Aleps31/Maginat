#!/bin/bash
[ -d /sources ] || { echo "ERRORE: lancialo dentro la chroot (bash /root/lfs-entra.sh)"; exit 1; }
cd /sources || exit 1
V=librsvg-2.40.21
[ -f $V.tar.xz ] || { echo "ERRORE: manca /sources/$V.tar.xz"; exit 1; }
rm -rf $V
tar -xf $V.tar.xz || exit 1
cd $V || exit 1
echo "== configure =="
CFLAGS="-O2 -std=gnu17 -Wno-error -Wno-deprecated-declarations -I/usr/include/libxml2 -include libxml/parser.h -include libxml/tree.h -include libxml/entities.h -include libxml/SAX2.h -include libxml/xmlIO.h -include libxml/xmlerror.h" ./configure --prefix=/usr --disable-static --disable-introspection --disable-vala --disable-pixbuf-loader --disable-gtk-doc > /root/rsvg-configure.log 2>&1 || { echo "ERRORE configure, ultime righe:"; tail -25 /root/rsvg-configure.log; exit 1; }
echo "== make (qualche minuto) =="
make -j"$(nproc)" > /root/rsvg-make.log 2>&1 || { echo "ERRORE make, ultime righe:"; tail -40 /root/rsvg-make.log; exit 1; }
echo "== install =="
make install > /root/rsvg-install.log 2>&1 || { echo "ERRORE install, ultime righe:"; tail -25 /root/rsvg-install.log; exit 1; }
echo "== verifica =="
LC_ALL=C rsvg-convert --version | head -1
T=$(mktemp -d)
cat > "$T/t.svg" << 'EOS'
<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128"><rect width="128" height="128" rx="24" fill="#e8590c"/><circle cx="64" cy="64" r="36" fill="#fff"/></svg>
EOS
rsvg-convert -w 48 -h 48 -o "$T/t.png" "$T/t.svg" || { echo "ERRORE: conversione di prova"; exit 1; }
if [ "$(head -c 4 "$T/t.png" | tail -c 3)" = "PNG" ]; then echo "OK: conversione SVG -> PNG (48x48, $(stat -c %s "$T/t.png") byte)"; else echo "ERRORE: il file prodotto non e un PNG"; exit 1; fi
rm -rf "$T"
echo "caricatori gdk-pixbuf (svg non deve esserci):"
ls /usr/lib/gdk-pixbuf-2.0/2.10.0/loaders/ | tr '\n' ' '
echo
printf 'righe svg nella cache dei caricatori: '
grep -ci svg /usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache || true
echo "FATTO"

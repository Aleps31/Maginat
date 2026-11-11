#!/bin/bash
set -e
cd /sources
rm -rf b43-fwcutter-019
tar -xjf b43-fwcutter-019.tar.bz2
cd b43-fwcutter-019
echo "== make =="
make CFLAGS="-O2 -std=gnu17 -Wno-error" > /tmp/fwcutter-make.log 2>&1 || { echo "ERRORE make:"; tail -20 /tmp/fwcutter-make.log; exit 1; }
echo "== install =="
install -m755 b43-fwcutter /usr/bin/b43-fwcutter
[ -f b43-fwcutter.1 ] && install -Dm644 b43-fwcutter.1 /usr/share/man/man1/b43-fwcutter.1
mkdir -p /usr/share/licenses/b43-fwcutter
for f in COPYING LICENSE README; do [ -f "$f" ] && install -m644 "$f" /usr/share/licenses/b43-fwcutter/; done
echo "== verifica =="
ls -l /usr/bin/b43-fwcutter
b43-fwcutter --version 2>&1 | head -2 || true
ls /usr/share/licenses/b43-fwcutter
head -12 /usr/share/licenses/b43-fwcutter/COPYING 2>/dev/null
echo FATTO

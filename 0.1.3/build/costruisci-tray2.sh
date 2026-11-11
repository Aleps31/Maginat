set -e
[ -f "${SRC:-/usr/share/maginat/src}/maginat-widget.c" ] || { echo "manca maginat-widget.c, esegui prima installa-widget.sh"; exit 1; }
SRC=${SRC:-/usr/share/maginat/src}
OUT=${OUT:-/usr/bin/maginat-tray}
B=$(mktemp -d)
cd "$B"
for par in battery:batt bluetooth:bt volume:vol lum:lum; do
  nome=${par%%:*}
  pre=${par##*:}
  f="$SRC/maginat-$nome-tray.c"
  [ -f "$f" ] || { echo "manca $f"; exit 1; }
  sed -e "s/^int main(/int ${pre}_main(/" -e '/^[[:space:]]*gtk_main();[[:space:]]*$/d' "$f" > "$pre.c"
  [ "$(grep -c "^int ${pre}_main(" "$pre.c")" = 1 ] || { echo "main non trovato in $nome"; exit 1; }
  ! grep -q "gtk_main();" "$pre.c" || { echo "gtk_main ancora presente in $nome"; exit 1; }
done
cat > maginat-tray-main.c <<'CEOF'
/* maginat-tray: avvia in un solo processo le icone di volume, luminosita, Bluetooth e batteria
 * Licenza: GPL-2.0-or-later
 */
#include <gtk/gtk.h>
int vol_main(int argc, char **argv);
int lum_main(int argc, char **argv);
int bt_main(int argc, char **argv);
int batt_main(int argc, char **argv);
int wid_main(int argc, char **argv);
int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    vol_main(argc, argv);
    lum_main(argc, argv);
    bt_main(argc, argv);
    batt_main(argc, argv);
    wid_main(argc, argv);
    gtk_main();
    return 0;
}
CEOF
for u in batt bt vol lum; do
  gcc -w -O2 -I"$SRC" -c "$u.c" -o "$u.o" $(pkg-config --cflags gtk+-3.0 gio-2.0)
done
gcc -w -O2 -I"$SRC" -c "$SRC/maginat-widget.c" -o wid.o $(pkg-config --cflags gtk+-3.0 gio-2.0)
gcc -w -O2 -c maginat-tray-main.c -o main.o $(pkg-config --cflags gtk+-3.0)
gcc -s -o "$OUT" main.o vol.o lum.o bt.o batt.o wid.o $(pkg-config --libs gtk+-3.0 gio-2.0)
cp maginat-tray-main.c "$SRC/maginat-tray-main.c" 2>/dev/null || true
echo "creato $OUT ($(du -k "$OUT" | cut -f1) KB)"
rm -rf "$B"

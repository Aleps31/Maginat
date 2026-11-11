#!/bin/sh
B="$1"
H="$B/exports/share/icons/hicolor"
[ -d "$H" ] || exit 0
command -v rsvg-convert >/dev/null 2>&1 || exit 0
L="$H/.maginat-png.lista"
N="$L.nuovo"
SIZES="16 24 32 48 64 96 128 256"
: > "$N"
if [ -f "$L" ]; then
  while IFS= read -r rel; do
    [ -n "$rel" ] || continue
    nome=${rel##*/}
    nome=${nome%.png}
    if [ -e "$H/scalable/apps/$nome.svg" ]; then
      echo "$rel" >> "$N"
    else
      rm -f "$H/$rel"
    fi
  done < "$L"
fi
for svg in "$H"/scalable/apps/*.svg; do
  [ -e "$svg" ] || continue
  nome=${svg##*/}
  nome=${nome%.svg}
  for s in $SIZES; do
    rel="${s}x${s}/apps/$nome.png"
    out="$H/$rel"
    if [ -e "$out" ] || [ -L "$out" ]; then
      if grep -qxF "$rel" "$N" 2>/dev/null; then
        [ "$svg" -nt "$out" ] || continue
      else
        continue
      fi
    fi
    mkdir -p "$H/${s}x${s}/apps" 2>/dev/null || continue
    if timeout 20 rsvg-convert -a -w "$s" -h "$s" -o "$out.tmp" "$svg" 2>/dev/null && [ -s "$out.tmp" ]; then
      mv -f "$out.tmp" "$out"
      grep -qxF "$rel" "$N" 2>/dev/null || echo "$rel" >> "$N"
    else
      rm -f "$out.tmp"
    fi
  done
done
mv -f "$N" "$L"
cp /usr/share/icons/hicolor/index.theme "$H/" 2>/dev/null
gtk-update-icon-cache --ignore-theme-index --force "$H" 2>/dev/null
exit 0

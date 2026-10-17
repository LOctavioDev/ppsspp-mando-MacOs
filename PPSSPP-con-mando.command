#!/bin/zsh
# Abre PPSSPP junto con el puente mando -> teclado.
# El SDL 2.26.5 que trae PPSSPP no detecta mandos en macOS 27, así que el puente
# lee el mando directamente y envía las teclas que PPSSPP ya tiene asignadas.
DIR=${0:A:h}   # carpeta real del script (también si se abre desde un enlace)
[ -x "$DIR/puente" ] || "$DIR/compilar.sh" || exit 1
"$DIR/puente" &
PUENTE=$!
trap 'kill $PUENTE 2>/dev/null' EXIT
/Applications/PPSSPPSDL.app/Contents/MacOS/PPSSPPSDL "$@"

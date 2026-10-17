#!/bin/zsh
# Compila el puente. Prueba primero el SDK por defecto y, si el compilador está
# desfasado respecto a él (pasa tras actualizar macOS), prueba los SDK anteriores.
cd ${0:A:h}
FLAGS=(-O2 puente.c -framework IOKit -framework CoreFoundation -framework ApplicationServices -o puente)
clang $FLAGS 2>/dev/null && { echo "Compilado"; exit 0; }
for sdk in $(ls -d /Library/Developer/CommandLineTools/SDKs/MacOSX[0-9]*.sdk | sort -rV); do
  clang -isysroot $sdk $FLAGS 2>/dev/null && { echo "Compilado con $(basename $sdk)"; exit 0; }
done
echo "No se pudo compilar. Instala las herramientas con: xcode-select --install"
exit 1

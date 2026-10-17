#!/bin/zsh
# Crea "PPSSPP con mando.app" en ~/Applications: abre el puente y PPSSPP sin
# pasar por Terminal, así que funciona desde Spotlight, el Dock o Launchpad.
DIR=${0:A:h}
APP="$HOME/Applications/PPSSPP con mando.app"
"$DIR/compilar.sh" || exit 1

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp /Applications/PPSSPPSDL.app/Contents/Resources/ppsspp.icns "$APP/Contents/Resources/" 2>/dev/null

cat > "$APP/Contents/MacOS/lanzar" <<SCRIPT
#!/bin/zsh
"$DIR/puente" &
PUENTE=\$!
trap 'kill \$PUENTE 2>/dev/null' EXIT
/Applications/PPSSPPSDL.app/Contents/MacOS/PPSSPPSDL
SCRIPT
chmod +x "$APP/Contents/MacOS/lanzar"

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>PPSSPP con mando</string>
  <key>CFBundleIdentifier</key><string>dev.loctavio.ppsspp-mando</string>
  <key>CFBundleExecutable</key><string>lanzar</string>
  <key>CFBundleIconFile</key><string>ppsspp</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleVersion</key><string>1.0</string>
  <key>LSUIElement</key><true/>
</dict>
</plist>
PLIST

codesign --force -s - "$APP" 2>/dev/null   # firma local: macOS recuerda el permiso de Accesibilidad
touch "$APP"
echo "Creada: $APP"

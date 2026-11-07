#!/bin/sh
# Usage: ./build.sh "/path/to/3D Weather Globe Deluxe Edition.app" [output.dmg]
# Applies the 2.0.7-U2B version to the app's Info.plist, re-signs it ad hoc, and builds the DMG.
set -e
APP="${1:?path to 3D Weather Globe Deluxe Edition.app}"; OUT="${2:-3D Weather Globe Deluxe.dmg}"
cp Info.plist "$APP/Contents/Info.plist"
codesign --force --deep --sign - "$APP"
pip install dmgbuild
dmgbuild -s settings.py -D app="$APP"  "3D Weather Globe Deluxe" "$OUT"

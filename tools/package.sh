#!/bin/sh
# Assembles build/3D Weather Globe Deluxe Edition.app (and the .saver and
# .mdimporter) from the original 2.0.7 bundles in game/ with the translated
# binaries from build/bin, then signs them ad hoc.
#
#   tools/package.sh            after `make`
set -e
cd "$(dirname "$0")/.."

APP_NAME="3D Weather Globe Deluxe Edition"
SRC="game"
OUT="build"
BIN="build/bin"

# Clones (APFS): the app's 7.5 GB of resources take no room twice; only the
# replaced files do.
copy_bundle() {
	rm -rf "$OUT/$1"
	ditto --clone "$SRC/$1" "$OUT/$1"
}

# The binary of a framework inside a bundle.
put_framework() {
	fw="$1/Contents/Frameworks/$2.framework/Versions/A/$2"
	[ -f "$fw" ] || { echo "missing $fw" >&2; exit 1; }
	cp "$BIN/$2" "$fw"
}

APP="$OUT/$APP_NAME.app"
copy_bundle "$APP_NAME.app"
cp "$BIN/WeatherGlobe" "$APP/Contents/MacOS/$APP_NAME"
for fw in LFoundation FishHook SQLiteLib WebCamsDB_C AppModel RenderLib ScreenSaverLib SMKControls SMKiMovieExport; do
	put_framework "$APP" "$fw"
done
cp "$BIN/URLCreator" "$APP/Contents/Resources/BASE/URLCreator"
# MainMenu.nib is the one pre-10.3 (typedstream) nib; 64-bit AppKit only reads
# keyed nibs, so it's re-archived as keyedobjects.nib.
NIB="$APP/Contents/Resources/English.lproj/MainMenu.nib"
build/tools/nibconv "$NIB/objects.nib" "$NIB/keyedobjects.nib" 2>/dev/null
rm -f "$NIB/objects.nib"
# Numeric table column identifiers break NSComboBox decoding now.
find "$APP" -name keyedobjects.nib -print0 | xargs -0 python3 -I tools/nibfix.py
QL="$APP/Contents/Library/QuickLook/AtlasRoute.qlgenerator/Contents/MacOS/AtlasRoute"
cp "$BIN/AtlasRoute" "$QL"

SAVER="$OUT/$APP_NAME.saver"
copy_bundle "$APP_NAME.saver"
cp "$BIN/GlobeScreenSaver" "$SAVER/Contents/MacOS/$(defaults read "$PWD/$SAVER/Contents/Info" CFBundleExecutable)"

MDI="$OUT/3DWeatherGlobe.mdimporter"
copy_bundle "3DWeatherGlobe.mdimporter"
cp "$BIN/GlobeImporter" "$MDI/Contents/MacOS/3DWeatherGlobe"

# Every binary is fat arm64 + x86_64 now; nothing i386 or PPC is left.
# (The Dashboard widget, 3D Weather.wdgt, is not packaged: macOS no longer has
# Dashboard.)
old=$(find "$APP" "$SAVER" "$MDI" -type f -size +10k | while IFS= read -r f; do
	if file -b "$f" | grep -q '^Mach-O' && lipo -archs "$f" | grep -qE 'i386|ppc'; then
		echo "$f"
	fi
done)
[ -z "$old" ] || { echo "old binaries left:" >&2; echo "$old" >&2; exit 1; }

# game/ lost the execute bits; cp kept them off.
find "$APP" "$SAVER" "$MDI" -type f -size +10k | while IFS= read -r f; do
	file -b "$f" | grep -q '^Mach-O' && chmod 755 "$f"
done

# The 2012 interface assumes light controls (white panes, black text; a plain
# NSTextField's background is black in dark mode): keep the app light.
plutil -replace NSRequiresAquaSystemAppearance -bool YES "$APP/Contents/Info.plist"

codesign --force --deep --sign - "$APP"
codesign --force --deep --sign - "$SAVER"
codesign --force --deep --sign - "$MDI"
echo "packaged $APP"

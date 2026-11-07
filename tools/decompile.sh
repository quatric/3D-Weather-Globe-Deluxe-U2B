#!/bin/sh
# Decompiles the thin i386 slices in analysis/ with headless Ghidra into
# analysis/decomp/<binary>/. PrepareObjC.java first applies the PIC thunk
# fixups and types every method from the ObjC metadata in
# analysis/objc-<name>.json (made by tools/objc_dump.py). Ghidra's settings
# and caches go under analysis/ghidra-home so nothing lands on the internal
# disk. FixPicCalls.java runs first, before analysis.
set -e
cd "$(dirname "$0")/.."
GHIDRA=${GHIDRA:-/Applications/ghidra_12.1.2_PUBLIC}
export JAVA_HOME=${JAVA_HOME:-/opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home}
HOME_DIR=$PWD/analysis/ghidra-home
mkdir -p "$HOME_DIR/tmp" analysis/ghidra-proj analysis/decomp
export JAVA_TOOL_OPTIONS="-Duser.home=$HOME_DIR -Djava.io.tmpdir=$HOME_DIR/tmp"

for name in ${@:-lfoundation fishhook sqlitelib webcamsdb_c appmodel renderlib screensaverlib smkcontrols smkimovieexport wg saver mdimporter qlroute urlcreator}; do
	python3 -I tools/objc_dump.py "analysis/$name-i386" "analysis/objc-$name.json"
	rm -rf "analysis/decomp/$name-i386"
	"$GHIDRA/support/analyzeHeadless" analysis/ghidra-proj WeatherGlobe \
		-import "analysis/$name-i386" -overwrite \
		-scriptPath tools/ghidra \
		-preScript FixPicCalls.java \
		-postScript PrepareObjC.java "$PWD/analysis/objc-$name.json" \
		-postScript ExportDecomp.java "$PWD/analysis/decomp" \
		-max-cpu 8 2>&1 | grep -E "ERROR|Exception|typed|patched|Import succeeded" || true
done

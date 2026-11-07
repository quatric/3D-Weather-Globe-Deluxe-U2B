#!/bin/sh
# put.sh <src/Target/File.m>: replaces a translated file's body with stdin,
# keeping its "Original module" first line and its own header import.
f="$1"
name=$(basename "$f" .m)
first=$(sed -n '1p' "$f")
if [ "$first" = "#import \"$name.h\"" ]; then
	{ echo "$first"; echo; cat; } > "$f.new" && mv "$f.new" "$f"
else
	{ echo "$first"; echo "#import \"$name.h\""; echo; cat; } > "$f.new" && mv "$f.new" "$f"
fi

#!/usr/bin/env python3
# nibfix.py keyedobjects.nib…
#
# Gives the table columns whose identifier is a number (NSComboBox's own
# column in EntryDetailsHeaderView.nib: 0) the number as a string. AppKit now
# sends the identifier -isEqualToString: when it decodes a combo box.
import plistlib
import sys

for path in sys.argv[1:]:
    with open(path, 'rb') as f:
        archive = plistlib.load(f)
    objects = archive['$objects']

    def class_name(o):
        if isinstance(o, dict) and '$class' in o:
            return objects[o['$class'].data]['$classname']

    changed = False
    for o in objects:
        if class_name(o) != 'NSTableColumn' or 'NSIdentifier' not in o:
            continue
        value = objects[o['NSIdentifier'].data]
        if isinstance(value, (int, float)) and not isinstance(value, bool):
            objects.append(str(int(value)) if float(value).is_integer() else str(value))
            o['NSIdentifier'] = plistlib.UID(len(objects) - 1)
            changed = True
    if changed:
        with open(path, 'wb') as f:
            plistlib.dump(archive, f, fmt=plistlib.FMT_BINARY)
        print('fixed', path)

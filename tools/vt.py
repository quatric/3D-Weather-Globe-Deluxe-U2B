#!/usr/bin/env python3
# vt.py <module> <class>...: a class's bases and vtable slots with their i386 offsets,
# skipping the LF::LObject slots it does not override.
import json, sys
mod = sys.argv[1]
classes = {c['name']: c for c in json.load(open(f'analysis/cxx-{mod}.json'))}
for name in sys.argv[2:]:
    c = classes.get(name)
    if not c:
        print(f'{name}: not found'); continue
    print(name, '<-', ', '.join(f"{b['name']}@{b['offset']:#x}" for b in c.get('bases', [])))
    for i, v in enumerate(c['vtable']):
        if v.startswith('LF::LObject::') and i < 0x70 // 4:
            continue
        print(f'  {i*4:#5x} {v}')

#!/usr/bin/env python3
"""Shows what is known of a class of a library for writing its declaration:
the worksheet's notes (size, bases, vtable, fields used) and the class's
symbols in the x86_64 build, which carry the parameter types.

usage: class_skel.py <appmodel|renderlib|...> <Class> [worksheet dir]

Note: when a listing starts with a "secondary vtable" line (classes with
more than one base), a vtable call at i386 offset N in the decompiles is the
listing's entry at +N+4; otherwise it is the entry at +N.
"""
import glob
import os
import re
import sys

lib, cls = sys.argv[1], sys.argv[2]
wsdir = sys.argv[3] if len(sys.argv) > 3 else {'appmodel': 'src/AppModel', 'renderlib': 'src/RenderLib',
                                                'screensaverlib': 'src/ScreenSaverLib'}.get(lib, 'src')
for f in sorted(glob.glob(os.path.join(wsdir, cls + '*.cpp'))):
    if not re.match(re.escape(cls) + r'(__LF)?\.cpp$', os.path.basename(f)):
        continue
    out = []
    for line in open(f, errors='replace'):
        if line.startswith('/* UNTRANSLATED'):
            break
        if line.startswith('//') and ('_classImp_' in line):
            break
        if line.startswith('//') and 'Worksheet generated' not in line and 'Translating a function' not in line:
            if re.search(r'LF::LObject::|LF::LClass::', line):
                continue
            out.append(line.rstrip())
    print('\n'.join(out))
syms = 'build/%s64-syms.txt' % lib
seen = set()
for line in open(syms):
    p = line.rstrip('\n').split(' ', 2)
    if len(p) < 3:
        continue
    name = p[2]
    if name.startswith(cls + '::') and '_classImp_' not in name and '_sClassImp' not in name:
        if name in seen:
            continue
        seen.add(name)
        print('%s %s' % (p[1], name))

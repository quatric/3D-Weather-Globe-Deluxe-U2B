#!/usr/bin/env python3
"""Prints the untranslated methods of a source file compactly, for
translating: the ivar offset map, then each method's declaration followed by
its decompiled body without local declarations and Ghidra boilerplate.

usage: show.py <src/Target/File.m>
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peek import sections

BINARY = {'SMKGL': 'smkgl', 'SMKControls': 'smkcontrols', 'WeatherGlobe': 'wg',
          'GlobeScreenSaver': 'saver'}


def cfstrings(name):
    """label -> literal, for the cfstr_ labels PrepareObjC.java made"""
    path = 'analysis/%s-i386' % name
    data = open(path, 'rb').read()
    secs = sections(path)

    def off(addr):
        for x in secs:
            if x['addr'] <= addr < x['addr'] + x['size']:
                return x['offset'] + addr - x['addr']
    out = {}
    ambiguous = set()
    for x in secs:
        if x['sect'] != '__cfstring':
            continue
        for a in range(x['addr'], x['addr'] + x['size'], 16):
            o = off(a)
            p, n = struct.unpack('<II', data[o + 8:o + 16])
            so = off(p)
            if so is None:
                continue
            lit = data[so:so + n].decode('latin-1')
            label = re.sub(r'[^A-Za-z0-9_]', '_', lit) or 'empty'
            key = 'cfstr_' + label[:48]
            if key in out and out[key] != lit:
                ambiguous.add(key)
            out[key] = lit
    for key in ambiguous:
        out[key] = None
    return out

text = open(sys.argv[1]).read()
target = os.path.basename(os.path.dirname(os.path.abspath(sys.argv[1])))
lits = cfstrings(BINARY[target])


def literal(m):
    lit = lits.get(m.group(1))
    if lit is None:
        return m.group(0)
    return '@' + json.dumps(lit)

for m in re.finditer(r'/\* i386 ivar offsets: .*?\*/|@implementation .*', text):
    print(m.group(0))
pat = re.compile(r'/\* --- i386 decompilation \(Ghidra\) ---\n(.*?)\*/\n([-+] \(.*?)\n\{\n    WG_UNTRANSLATED\(\);', re.S)
decl_re = re.compile(r'^\s+(?:undefined\d*|u?int|uint|u?short|byte|char|bool|BOOL|id|SEL|float|double|longdouble|'
                     r'u?longlong|undefined\d* \*|[A-Za-z_][\w ]*\*+)\s*\*?\s*[\w\[\]]+(?: \[\d+\])?;$')
for m in pat.finditer(text):
    body, decl = m.group(1), m.group(2)
    lines = []
    for l in body.splitlines():
        if not l.strip() or 'WARNING:' in l or decl_re.match(l):
            continue
        if re.match(r'^/\* [-+]\[.*\] @ ', l):
            continue
        lines.append(re.sub(r'&?(cfstr_\w+)', literal, l))
    print('\n' + decl)
    print('\n'.join(lines[1:] if lines and '(' in lines[0] else lines))

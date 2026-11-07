#!/usr/bin/env python3
"""rd64.py <name> <symbol|hexaddr> [count] [i|f|d|q]: values at a data
address of analysis/<name>-x86_64 (32-bit ints, floats, doubles or 64-bit
words, annotated with the symbol they point to)."""
import struct, subprocess, sys
sys.path.insert(0, 'tools')
from dis64 import sections
name, where = sys.argv[1], sys.argv[2]
n = int(sys.argv[3]) if len(sys.argv) > 3 else 4
kind = sys.argv[4] if len(sys.argv) > 4 else 'i'
path = 'analysis/%s-x86_64' % name
syms, addrs = {}, {}
for l in subprocess.run(['nm', path], capture_output=True, text=True).stdout.splitlines():
    p = l.split(None, 2)
    if len(p) == 3:
        addrs[p[2]] = int(p[0], 16); syms.setdefault(int(p[0], 16), p[2])
a = addrs[where] if where in addrs else int(where, 16)
data = open(path, 'rb').read()
for s in sections(path):
    if s['addr'] <= a < s['addr'] + s['size']:
        if s['name'] in ('__bss', '__common'):
            print('zero-filled (%s)' % s['name']); sys.exit()
        o = s['offset'] + a - s['addr']
        fmt, size = {'i': ('<i', 4), 'f': ('<f', 4), 'd': ('<d', 8), 'q': ('<Q', 8)}[kind]
        for i in range(n):
            v = struct.unpack(fmt, data[o + i * size:o + (i + 1) * size])[0]
            print(hex(a + i * size), v, syms.get(v, '') if kind == 'q' else '')

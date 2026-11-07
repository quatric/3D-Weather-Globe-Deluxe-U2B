#!/usr/bin/env python3
"""Resolves i386 __IMPORT,__pointers entries (PTR_xxx in the decompiles) to
the symbol they point at.

usage: ptr.py <binary> <hexaddr>...
"""
import re
import subprocess
import sys

binary = sys.argv[1]
sects = subprocess.run(['otool', '-l', binary], capture_output=True, text=True).stdout
m = re.search(r'sectname __pointers\n\s+segname __IMPORT\n\s+addr (0x[0-9a-f]+)\n\s+size (0x[0-9a-f]+)\n\s+offset (\d+)', sects)
base, size, off = int(m.group(1), 16), int(m.group(2), 16), int(m.group(3))
data = open(binary, 'rb').read()
syms = {}
for line in subprocess.run(['nm', '-n', binary], capture_output=True, text=True).stdout.splitlines():
    p = line.split(None, 2)
    if len(p) == 3:
        syms.setdefault(int(p[0], 16), p[2])
for a in sys.argv[2:]:
    a = int(a, 16)
    v = int.from_bytes(data[off + a - base:off + a - base + 4], 'little')
    name = syms.get(v, '?')
    dem = subprocess.run(['c++filt', name], capture_output=True, text=True).stdout.strip()
    print('%08x -> %08x %s' % (a, v, dem))

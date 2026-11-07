#!/usr/bin/env python3
# rd.py <binary> <addr> [nwords]: i386 words at addr; each word that points into a
# section is annotated with the C string there (if any) and its symbol.
import struct, subprocess, sys
f, addr = sys.argv[1], int(sys.argv[2], 16)
n = int(sys.argv[3]) if len(sys.argv) > 3 else 8
data = open(f, 'rb').read()
out = subprocess.run(['otool', '-arch', 'i386', '-l', f], capture_output=True, text=True).stdout
secs, cur = [], {}
for l in out.splitlines():
    p = l.strip().split()
    if not p: continue
    if p[0] in ('sectname', 'segname', 'addr', 'size', 'offset'): cur[p[0]] = p[1]
    if p[0] == 'offset' and 'addr' in cur: secs.append(dict(cur)); cur = {}
syms = {}
for l in subprocess.run(['nm', '-arch', 'i386', f], capture_output=True, text=True).stdout.splitlines():
    p = l.split(None, 2)
    if len(p) == 3 and p[0].strip(): syms[int(p[0], 16)] = p[2]
def sec(a):
    for s in secs:
        ad, sz = int(s['addr'], 16), int(s['size'], 16)
        if ad <= a < ad + sz: return s
def rd(a, k):
    s = sec(a)
    if not s or s['sectname'] in ('__bss', '__common'): return None
    o = int(s['offset']) + a - int(s['addr'], 16)
    return data[o:o + k]
def cstr(a):
    b = rd(a, 200)
    if not b: return None
    e = b.find(b'\0')
    t = b[:e if e >= 0 else 200]
    return t.decode('latin1') if t and all(32 <= c < 127 or c in (9, 10) for c in t) else None
for i in range(n):
    a = addr + 4 * i
    b = rd(a, 4)
    if b is None: print(f'{a:08x}: (no data)'); continue
    w = struct.unpack('<I', b)[0]
    s = sec(w)
    note = ''
    if s:
        note = f" -> {s['sectname']}"
        if w in syms: note += ' ' + syms[w]
        c = cstr(w)
        if c: note += f' "{c}"'
    f_ = struct.unpack('<f', b)[0]
    print(f'{a:08x}: {w:08x} {f_:g}{note}')

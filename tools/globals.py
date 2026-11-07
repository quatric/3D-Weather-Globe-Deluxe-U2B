#!/usr/bin/env python3
"""Lists the original binary's _k globals with what they hold: the CFString
a pointer refers to, or the raw words and their float values.

usage: globals.py <wg|smkgl|smkcontrols> [other]
With "other", lists the remaining data symbols that point at CFStrings.
"""
import struct
import subprocess
import sys

from peek import sections

name = sys.argv[1]
path = 'analysis/%s-i386' % name
data = open(path, 'rb').read()
secs = sections(path)


def sect(addr):
    for s in secs:
        if s['addr'] <= addr < s['addr'] + s['size']:
            return s
    return None


def word(addr):
    s = sect(addr)
    o = s['offset'] + addr - s['addr']
    return struct.unpack('<I', data[o:o + 4])[0]


def cfstring(addr):
    s = sect(addr)
    if s is None or s['sect'] != '__cfstring':
        return None
    ptr, n = word(addr + 8), word(addr + 12)
    t = sect(ptr)
    o = t['offset'] + ptr - t['addr']
    raw = data[o:o + (n * 2 if t['sect'] == '__ustring' else n)]
    return raw.decode('utf-16-le' if t['sect'] == '__ustring' else 'mac_roman')


syms = []
for line in subprocess.run(['nm', '-n', path], capture_output=True, text=True).stdout.splitlines():
    parts = line.split()
    if len(parts) == 3 and parts[1] in 'dDsSbB':
        syms.append((int(parts[0], 16), parts[2]))
other = len(sys.argv) > 2 and sys.argv[2] == 'other'
for i, (addr, sym) in enumerate(syms):
    if other:
        s = sect(addr)
        if sym.startswith('_k') or s['sect'] in ('__bss', '__common', '__cfstring'):
            continue
        lit = cfstring(word(addr))
        if lit is not None:
            print('%s\tstr\t%r' % (sym, lit))
        continue
    if not sym.startswith('_k'):
        continue
    size = (syms[i + 1][0] if i + 1 < len(syms) else addr + 4) - addr
    s = sect(addr)
    if s['sect'] in ('__bss', '__common'):
        print('%s\tbss\t%d' % (sym, size))
        continue
    w = word(addr)
    lit = cfstring(w)
    if lit is not None:
        print('%s\tstr\t%r' % (sym, lit))
    else:
        words = [word(addr + k) for k in range(0, min(size, 64), 4)]
        print('%s\traw%d\t%s\t%s' % (sym, size, ' '.join('%08x' % x for x in words),
              ' '.join('%g' % struct.unpack('<f', struct.pack('<I', x))[0] for x in words)))

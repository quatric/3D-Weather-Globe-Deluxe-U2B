#!/usr/bin/env python3
"""Shows what a symbol or address in one of the original i386 binaries holds:
4-byte words, each decoded as a CFString or C string when it points at one,
and as an int and float.

usage: peek.py <wg|smkgl|smkcontrols|saver|mdimporter> <symbol|0xaddress> [words]
"""
import struct
import subprocess
import sys


def sections(path):
    out = subprocess.run(['otool', '-l', path], capture_output=True, text=True).stdout
    secs, cur = [], None
    for line in out.splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == 'sectname':
            cur = {'sect': parts[1]}
            secs.append(cur)
        elif cur is not None and parts[0] in ('segname', 'addr', 'size', 'offset'):
            cur[parts[0]] = parts[1] if parts[0] == 'segname' else int(parts[1], 0)
    return secs


def main():
    name, what = sys.argv[1], sys.argv[2]
    words = int(sys.argv[3]) if len(sys.argv) > 3 else 4
    path = 'analysis/%s-i386' % name
    data = open(path, 'rb').read()
    secs = sections(path)

    def find(addr):
        for s in secs:
            if s['addr'] <= addr < s['addr'] + s['size']:
                return s
        return None

    def read(addr, n):
        s = find(addr)
        if s is None or s['sect'] in ('__bss', '__common'):
            return None
        o = s['offset'] + addr - s['addr']
        return data[o:o + n]

    def cstring(addr):
        b = read(addr, 512)
        return b.split(b'\0')[0].decode('latin-1') if b else None

    def describe(v):
        s = find(v)
        if s is None:
            return ''
        if s['sect'] == '__cfstring':
            w = read(v, 16)
            return '@"%s"' % cstring(struct.unpack('<I', w[8:12])[0])
        if s['sect'] in ('__cstring', '__const') and s['segname'] == '__TEXT':
            c = cstring(v)
            if c and c.isprintable():
                return '"%s"' % c
        return '-> %s,%s' % (s['segname'], s['sect'])

    if what.startswith('0x'):
        addr = int(what, 16)
    else:
        nm = subprocess.run(['nm', path], capture_output=True, text=True).stdout
        hits = [l.split()[0] for l in nm.splitlines() if l.split()[-1] in (what, '_' + what)]
        if not hits:
            sys.exit('no symbol %s' % what)
        addr = int(hits[0], 16)
    s = find(addr)
    print('%#x in %s,%s' % (addr, s['segname'], s['sect']) if s else '%#x unmapped' % addr)
    if s and s['sect'] in ('__bss', '__common'):
        print('zero-initialised')
        return
    for i in range(words):
        b = read(addr + 4 * i, 4)
        if not b or len(b) < 4:
            break
        v = struct.unpack('<I', b)[0]
        f = struct.unpack('<f', b)[0]
        print('  +%-3x %08x  int %-11d float %-12g %s' % (4 * i, v, struct.unpack('<i', b)[0], f, describe(v)))


if __name__ == '__main__':
    main()

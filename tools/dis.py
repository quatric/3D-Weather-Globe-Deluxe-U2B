#!/usr/bin/env python3
"""Disassembles one function of an original i386 binary with its PIC-relative
operands resolved: selector and class references, CFStrings, float and double
literals, and imported symbols.

usage: dis.py <wg|smkgl|smkcontrols|saver|mdimporter> '<-[Class method:]>|_function'
"""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peek import sections  # noqa: E402


def main():
    name, func = sys.argv[1], sys.argv[2]
    path = 'analysis/%s-i386' % name
    cache = 'build/%s-i386.s' % name
    if not os.path.exists(cache) or os.path.getmtime(cache) < os.path.getmtime(path):
        with open(cache, 'w') as f:
            subprocess.run(['otool', '-tV', path], stdout=f, check=True)
    data = open(path, 'rb').read()
    secs = sections(path)

    # imported symbol pointers
    imports = {}
    out = subprocess.run(['otool', '-Iv', path], capture_output=True, text=True).stdout
    for line in out.splitlines():
        m = re.match(r'\s*0x([0-9a-f]+)\s+\d+\s+(\S+)', line)
        if m:
            imports[int(m.group(1), 16)] = m.group(2)

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
        b = read(addr, 256)
        return b.split(b'\0')[0].decode('latin-1') if b else '?'

    def word(addr):
        b = read(addr, 4)
        return struct.unpack('<I', b)[0] if b and len(b) == 4 else None

    def describe(addr, is_load):
        s = find(addr)
        if s is None:
            return None
        sect = s['sect']
        if addr in imports:
            return imports[addr]
        if sect in ('__message_refs', '__selrefs'):
            return '@selector(%s)' % cstring(word(addr))
        if sect in ('__cls_refs', '__classrefs'):
            return 'class %s' % cstring(word(addr))
        if sect == '__cfstring':
            w = read(addr, 16)
            return '@"%s"' % cstring(struct.unpack('<I', w[8:12])[0])
        if sect == '__literal4':
            return '%gf' % struct.unpack('<f', read(addr, 4))[0]
        if sect == '__literal8':
            return '%r' % struct.unpack('<d', read(addr, 8))[0]
        if sect == '__cstring':
            return '"%s"' % cstring(addr)
        if sect == '__const' and s['segname'] == '__TEXT':
            fl = struct.unpack('<4f', read(addr, 16))
            return '__const %s' % ', '.join('%g' % v for v in fl)
        return '%s,%s %#x' % (s['segname'], sect, addr)

    lines = open(cache).read().split('\n')
    start = None
    for i, l in enumerate(lines):
        if l == func + ':' or l == '_' + func.lstrip('_') + ':' and func.startswith('_'):
            start = i
            break
    if start is None:
        sys.exit('no function %s' % func)
    body = []
    for l in lines[start + 1:]:
        if re.match(r'^[-+_A-Za-z].*:$', l):
            break
        body.append(l)
    ebx = None
    for i, l in enumerate(body):
        if 'get_pc_thunk.bx' in l and i + 1 < len(body):
            ebx = int(body[i + 1].split('\t')[0], 16)
            break
    for l in body:
        parts = l.split('\t')
        note = ''
        m = re.search(r'(-?0x[0-9a-f]+)\(%ebx\)', l)
        if m and ebx is not None:
            d = describe((ebx + int(m.group(1), 16)) & 0xffffffff, 'lea' not in l)
            if d:
                note = '  ; ' + d
        m = re.search(r'\$0x([0-9a-f]+)', l)
        if m and not note:
            v = int(m.group(1), 16)
            d = describe(v, False) if v > 0x10000 else None
            if d:
                note = '  ; ' + d
            elif 0x3a000000 <= v <= 0x4f000000:
                note = '  ; %gf' % struct.unpack('<f', struct.pack('<I', v))[0]
        print('\t'.join(parts[1:]) + note if len(parts) > 1 else l)


if __name__ == '__main__':
    main()

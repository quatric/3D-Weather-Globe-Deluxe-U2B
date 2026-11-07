#!/usr/bin/env python3
"""Reads the C++ structure of a thin i386 or x86_64 Mach-O into JSON: every
class that has a vtable, typeinfo or methods, with its bases (from RTTI), its
virtual methods in vtable order and its other methods, all demangled.

usage: cxx_dump.py <thin-binary> <out.json>
"""
import json
import struct
import subprocess
import sys

LC_SEGMENT, LC_SEGMENT_64, LC_SYMTAB, LC_DYSYMTAB = 0x1, 0x19, 0x2, 0xb


class MachO:
    def __init__(self, path):
        self.data = open(path, 'rb').read()
        magic, cputype = struct.unpack_from('<II', self.data)
        self.is64 = magic == 0xfeedfacf
        if magic not in (0xfeedface, 0xfeedfacf):
            sys.exit('%s: not a thin little-endian Mach-O' % path)
        self.ptr = 8 if self.is64 else 4
        ncmds = struct.unpack_from('<I', self.data, 16)[0]
        off = 32 if self.is64 else 28
        self.sections = []
        self.extrel = {}
        symoff = nsyms = stroff = 0
        for _ in range(ncmds):
            cmd, size = struct.unpack_from('<II', self.data, off)
            if cmd in (LC_SEGMENT, LC_SEGMENT_64):
                if self.is64:
                    nsects = struct.unpack_from('<I', self.data, off + 64)[0]
                    s = off + 72
                    for _ in range(nsects):
                        sect, seg = [x.rstrip(b'\0').decode() for x in struct.unpack_from('<16s16s', self.data, s)]
                        addr, sz, fo = struct.unpack_from('<QQI', self.data, s + 32)
                        flags = struct.unpack_from('<I', self.data, s + 64)[0]
                        self.sections.append((seg, sect, addr, sz, fo, flags))
                        s += 80
                else:
                    nsects = struct.unpack_from('<I', self.data, off + 48)[0]
                    s = off + 56
                    for _ in range(nsects):
                        sect, seg = [x.rstrip(b'\0').decode() for x in struct.unpack_from('<16s16s', self.data, s)]
                        addr, sz, fo = struct.unpack_from('<III', self.data, s + 32)
                        flags = struct.unpack_from('<I', self.data, s + 56)[0]
                        self.sections.append((seg, sect, addr, sz, fo, flags))
                        s += 68
            elif cmd == LC_SYMTAB:
                symoff, nsyms, stroff = struct.unpack_from('<III', self.data, off + 8)
            elif cmd == LC_DYSYMTAB:
                v = struct.unpack_from('<18I', self.data, off + 8)
                extreloff, nextrel = v[14], v[15]
                for i in range(nextrel):
                    a, info = struct.unpack_from('<iI', self.data, extreloff + 8 * i)
                    self.extrel[a] = info & 0xffffff
            off += size
        self.syms = []
        n = 16 if self.is64 else 12
        for i in range(nsyms):
            if self.is64:
                strx, typ, sect, desc, val = struct.unpack_from('<IBBHQ', self.data, symoff + n * i)
            else:
                strx, typ, sect, desc, val = struct.unpack_from('<IBBhI', self.data, symoff + n * i)
            end = self.data.index(b'\0', stroff + strx)
            self.syms.append((self.data[stroff + strx:end].decode('latin-1'), typ, sect, val))
        self.by_addr = {}
        for name, typ, sect, val in self.syms:
            if typ & 0xe0 == 0 and typ & 0x0e == 0x0e and name:
                self.by_addr.setdefault(val, name)

    def file_offset(self, addr):
        for seg, sect, a, sz, fo, flags in self.sections:
            if a <= addr < a + sz and flags & 0xff != 1:   # not zerofill
                return fo + addr - a
        return None

    def read_ptr(self, addr):
        """(value, external symbol or None)"""
        if addr in self.extrel:
            return 0, self.syms[self.extrel[addr]][0]
        fo = self.file_offset(addr)
        if fo is None:
            return None, None
        return struct.unpack_from('<Q' if self.is64 else '<I', self.data, fo)[0], None

    def cstring(self, addr):
        fo = self.file_offset(addr)
        if fo is None:
            return None
        return self.data[fo:self.data.index(b'\0', fo)].decode('latin-1')


def demangle(names):
    out = subprocess.run(['c++filt', '-_'], input='\n'.join(names), capture_output=True,
                         text=True).stdout.split('\n')
    return dict(zip(names, out))


def class_of(dem):
    """'A::B::method(args)' -> 'A::B' (None for free functions)"""
    depth = 0
    paren = None
    for i, c in enumerate(dem):
        if c == '<':
            depth += 1
        elif c == '>':
            depth -= 1
        elif c == '(' and depth == 0:
            paren = i
            break
    head = dem if paren is None else dem[:paren]
    depth = 0
    cut = None
    for i in range(len(head) - 1):
        c = head[i]
        if c == '<':
            depth += 1
        elif c == '>':
            depth -= 1
        elif head[i:i + 2] == '::' and depth == 0:
            cut = i
    return head[:cut] if cut is not None else None


def main():
    m = MachO(sys.argv[1])
    names = sorted({s[0] for s in m.syms if s[0].startswith('__Z')})
    dem = demangle(names)
    classes = {}

    def cls(name):
        return classes.setdefault(name, {'name': name, 'bases': [], 'vtable': [],
                                         'methods': [], 'statics': []})

    def name_of(sym):
        if sym not in dem and sym.startswith('__Z'):
            dem.update(demangle([sym]))
        return dem.get(sym, sym[1:] if sym.startswith('_') else sym)

    def base_of(sym):
        return name_of(sym).replace('typeinfo for ', '')

    defined = {}
    for name, typ, sect, val in m.syms:
        if typ & 0xe0 or typ & 0x0e != 0x0e or not name.startswith('__Z'):
            continue
        defined[name] = val

    for name, val in sorted(defined.items(), key=lambda x: x[1]):
        d = dem[name]
        if d.startswith('typeinfo for '):
            c = cls(d[len('typeinfo for '):])
            kind = m.read_ptr(val)[1] or ''
            if 'si_class_type_info' in kind:
                base = m.read_ptr(val + 2 * m.ptr)
                bname = base[1] or m.by_addr.get(base[0])
                if bname:
                    c['bases'].append({'name': base_of(bname), 'offset': 0})
            elif 'vmi_class_type_info' in kind:
                fo = m.file_offset(val + 2 * m.ptr)
                flags, count = struct.unpack_from('<II', m.data, fo)
                for i in range(count):
                    a = val + 2 * m.ptr + 8 + i * 2 * m.ptr
                    base = m.read_ptr(a)
                    bname = base[1] or m.by_addr.get(base[0])
                    off = m.read_ptr(a + m.ptr)[0] or 0
                    c['bases'].append({'name': base_of(bname) if bname else None,
                                       'offset': off >> 8, 'flags': off & 0xff})
            c['typeinfo'] = hex(val)
        elif d.startswith('vtable for '):
            c = cls(d[len('vtable for '):])
            c['vtable_addr'] = hex(val)
            # offset-to-top, typeinfo, then entries until the next symbol
            a = val + 2 * m.ptr
            entries = []
            while True:
                v, ext = m.read_ptr(a)
                if v is None or (a != val + 2 * m.ptr and a in m.by_addr):
                    break
                if ext:
                    entries.append(name_of(ext))
                elif v == 0:
                    entries.append(None)
                elif v in m.by_addr:
                    s = m.by_addr[v]
                    if s.startswith('__ZTI'):        # secondary vtable header
                        entries.append('-- secondary vtable --')
                    else:
                        entries.append(name_of(s))
                else:
                    break
                a += m.ptr
            while entries and entries[-1] is None:
                entries.pop()
            c['vtable'] = entries
        elif d.startswith(('typeinfo name for ', 'VTT for ', 'construction vtable')):
            continue
        else:
            owner = class_of(d)
            if owner is None:
                continue
            sec = next((s for s in m.sections if s[2] <= val < s[2] + s[3]), None)
            entry = {'addr': hex(val), 'symbol': name[1:], 'decl': d}
            if sec and sec[1] in ('__text', '__textcoal_nt'):
                cls(owner)['methods'].append(entry)
            else:
                cls(owner)['statics'].append(entry)

    json.dump(sorted(classes.values(), key=lambda c: c['name']), open(sys.argv[2], 'w'), indent=1)
    print('%s: %d classes, %d with vtables, %d methods' % (
        sys.argv[1], len(classes), sum(1 for c in classes.values() if c['vtable']),
        sum(len(c['methods']) for c in classes.values())))


if __name__ == '__main__':
    main()

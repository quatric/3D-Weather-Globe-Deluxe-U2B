#!/usr/bin/env python3
"""Prints a draft C++ declaration for a class, from analysis/cxx-<binary>.json
and Ghidra's signatures: bases, virtual methods in vtable order, the other
methods, i386 size and the field offsets its code uses. Return types are
Ghidra's guesses (undefined4 is int-or-pointer), so the draft needs a pass by
hand.

usage: draft_header.py <binary-name> <Class>        e.g. lfoundation LF::LString
"""
import json
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(__file__), '..')

RET = {'undefined4': 'int', 'undefined1': 'bool', 'undefined': 'bool', 'uint': 'unsigned int',
       'uchar': 'unsigned char', 'undefined8': 'long long', 'ulonglong': 'unsigned long long',
       'longlong': 'long long', 'ushort': 'unsigned short', 'undefined2': 'short'}


def main():
    name, cls = sys.argv[1:3]
    analysis = os.path.join(ROOT, 'analysis')
    classes = {c['name']: c for c in json.load(open(os.path.join(analysis, 'cxx-%s.json' % name)))}
    decomp = os.path.join(analysis, 'decomp', '%s-i386' % name)
    sigs = {}
    for line in open(os.path.join(decomp, 'index.tsv')):
        addr, size, fname, file = line.rstrip('\n').split('\t')
        sigs[int(addr, 16)] = file
    c = classes[cls]
    short = cls.split('::')[-1]

    def ret_type(m):
        file = sigs.get(int(m['addr'], 16))
        if not file:
            return '?'
        text = open(os.path.join(decomp, file), errors='replace').read()
        lines = [l for l in text.split('\n') if l.strip() and not l.startswith(('/*', '   '))]
        sig = re.match(r'(.*?)\(', ' '.join(lines[:3]))
        if not sig:
            return '?'
        head = ' '.join(sig.group(1).split())
        head = re.sub(r'__thiscall|__cdecl', '', head).strip()
        rt = head[:head.rfind(' ')] if ' ' in head else 'void'
        rt = rt.strip()
        return RET.get(rt, rt)

    vt = c.get('vtable') or []
    virt = {}
    for v in vt:
        if v and v.startswith(cls + '::'):
            virt[v] = True
    bases = ', '.join('public ' + b['name'] for b in c['bases'] if b['name'])
    print('class %s%s {' % (short, ' : ' + bases if bases else ''))
    print('public:')
    seen = set()
    by_decl = {}
    for m in c['methods']:
        by_decl.setdefault(m['decl'], m)
    # virtuals first, in vtable order (including inherited slots, as comments)
    for i, v in enumerate(vt):
        if not v or v.startswith('--'):
            continue
        if v.startswith(cls + '::') and v in by_decl:
            m = by_decl[v]
            seen.add(v)
            print('\tvirtual %s %s;\t// [%d]' % (ret_type(m), v[len(cls) + 2:], i))
        elif v.startswith(cls + '::'):
            print('\tvirtual %s;\t// [%d] (not defined here)' % (v[len(cls) + 2:], i))
    for d, m in by_decl.items():
        if d in seen:
            continue
        rest = d[len(cls) + 2:] if d.startswith(cls + '::') else d
        if '::' in rest.split('(')[0]:
            continue      # nested class
        if rest.startswith(('~', short + '(')):
            print('\t%s;' % rest)
        else:
            print('\t%s %s;' % (ret_type(m), rest))
    print('};')


if __name__ == '__main__':
    main()

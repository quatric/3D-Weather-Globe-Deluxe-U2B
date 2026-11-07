#!/usr/bin/env python3
"""Shows functions of an x86_64 analysis binary, where the call arguments that
the i386 decompiles drop are visible in registers. Each reference to an
LConstantString (_sFoo) or to a pointer to one (_kFoo) is annotated with the
string's text, recovered from the static initializers.

usage: dis64.py <appmodel|renderlib|...> <substring of mangled or demangled name>...
       dis64.py <binary> --strings      (lists every constant string)
"""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def load(name):
    path = 'analysis/%s-x86_64' % name
    cache = 'build/%s-x86_64.s' % name
    if not os.path.exists(cache) or os.path.getmtime(cache) < os.path.getmtime(path):
        os.makedirs('build', exist_ok=True)
        with open(cache, 'w') as f:
            subprocess.run(['otool', '-tV', path], stdout=f, check=True)
    return path, open(cache).read().splitlines()


def sections(path):
    out = subprocess.run(['otool', '-l', path], capture_output=True, text=True).stdout
    secs, cur = [], {}
    for line in out.splitlines():
        p = line.strip().split()
        if not p:
            continue
        if p[0] in ('sectname', 'segname', 'addr', 'size', 'offset'):
            cur[p[0]] = p[1]
        if p[0] == 'offset' and 'addr' in cur:
            secs.append({'name': cur['sectname'], 'addr': int(cur['addr'], 16),
                         'size': int(cur['size'], 16), 'offset': int(cur['offset'])})
            cur = {}
    return secs


def strings(path, lines):
    """symbol -> text for LConstantStrings and the _k pointers to them"""
    texts = {}
    lit = sym = None
    for line in lines:
        m = re.search(r'literal pool for: "(.*)"$', line)
        if m:
            lit = m.group(1)
            continue
        m = re.search(r'leaq\t(_\w+)\(%rip\), %rdi', line)
        if m:
            sym = m.group(1)
            continue
        if 'LConstantStringC' in line and 'callq' in line:
            if lit is not None and sym:
                texts[sym] = lit
            lit = sym = None
    # data pointers to the strings
    data = open(path, 'rb').read()
    secs = sections(path)
    syms = {}
    addrs = {}
    for line in subprocess.run(['nm', path], capture_output=True, text=True).stdout.splitlines():
        p = line.split(None, 2)
        if len(p) == 3:
            addrs[p[2]] = int(p[0], 16)
            syms.setdefault(int(p[0], 16), p[2])
    for name, addr in addrs.items():
        if name in texts:
            continue
        for s in secs:
            if s['name'] in ('__data', '__const') and s['addr'] <= addr < s['addr'] + s['size']:
                o = s['offset'] + addr - s['addr']
                target = syms.get(struct.unpack('<Q', data[o:o + 8])[0])
                if target in texts:
                    texts[name] = '-> ' + texts[target]
    return texts


def main():
    name = sys.argv[1]
    path, lines = load(name)
    texts = strings(path, lines)
    if sys.argv[2] == '--strings':
        for k in sorted(texts):
            print('%s = %s' % (k, texts[k]))
        return
    pats = sys.argv[2:]
    labels = [l[:-1] for l in lines if l.endswith(':') and not l.startswith('0')]
    dems = subprocess.run(['c++filt'], input='\n'.join(labels), capture_output=True,
                          text=True).stdout.splitlines()
    demangled = dict(zip(labels, dems))
    show = False
    for line in lines:
        if line.endswith(':') and not line.startswith('0'):
            label = line[:-1]
            dem = demangled.get(label, label)
            show = any(p in label or p in dem for p in pats)
            if show:
                print('\n## ' + dem)
            continue
        if not show:
            continue
        line = re.sub(r'^[0-9a-f]+\t', '', line)
        for m in re.findall(r'(_\w+)\(%rip\)', line):
            if m in texts:
                line += '   "%s"' % texts[m]
        line = re.sub(r'\s+## (literal pool symbol address|symbol stub for): ', '  ; ', line)
        print(line)


if __name__ == '__main__':
    main()

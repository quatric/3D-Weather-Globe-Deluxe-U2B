#!/usr/bin/env python3
"""Class declaration skeletons for RenderLib classes: hdr.py Class...

Bases and the class's own virtuals (in vtable order) come from the class map;
the other methods from the worksheets' UNTRANSLATED blocks; return types are
guessed from the x86_64 decompile (/*?*/ marks a guess to check). The fields
are left to the translator: the i386 offsets used are listed as a comment."""
import re, sys, glob, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLASSMAP = os.path.join(ROOT, 'analysis/rl_classmap.txt')
X64 = os.path.join(ROOT, 'analysis/decomp/renderlib-x86_64')

def classmap():
    out, cur = {}, None
    for line in open(CLASSMAP):
        if not line.startswith(' '):
            name = line.split(',')[0].strip()
            cur = out.setdefault(name, {'head': line.strip(), 'virt': [], 'used': ''})
        elif cur is not None:
            s = line.strip()
            if s.startswith('this+offset used:'):
                cur['used'] = s
            elif re.match(r'0x[0-9a-f]+ ', s):
                off, sig = s.split(' ', 1)
                cur['virt'].append((int(off, 16), sig))
    return out

def worksheet_funcs(cls):
    sigs = []
    for f in glob.glob(os.path.join(ROOT, 'src/RenderLib/*.cpp')):
        for m in re.finditer(r'^/\* UNTRANSLATED (%s::[^@]*?) @' % re.escape(cls), open(f, errors='replace').read(), re.M):
            s = m.group(1).strip()
            if s not in sigs:
                sigs.append(s)
    return sigs

x64index = {}
for line in open(os.path.join(X64, 'index.tsv')):
    p = line.rstrip('\n').split('\t')
    if len(p) == 4:
        x64index.setdefault(p[2], []).append(p[3])

def ret_type(cls, meth):
    for fn in x64index.get('%s::%s' % (cls, meth), []):
        t = re.sub(r'/\*.*?\*/', '', open(os.path.join(X64, fn), errors='replace').read(), flags=re.S)
        t = re.sub(r'\s+', ' ', t)
        m = re.search(r'(?:^|[;{}] ?)\s*([^;{}()]*?)\s*(__thiscall\s+)?%s::%s\s*\(([^)]*)\)' % (re.escape(cls), re.escape(meth)), t)
        if m:
            static = not m.group(2) and 'this' not in m.group(3)
            return conv(m.group(1).strip()), static
    return '/*?*/ void', False

def conv(r):
    r = r.replace('LF::', '')
    if r in ('void',): return 'void'
    if r in ('float', 'longdouble'): return 'float'
    if r == 'double': return 'double'
    if r in ('bool', 'undefined1', 'uchar', 'char', 'byte'): return '/*?*/ bool'
    if r in ('int', 'uint', 'undefined4', 'long', 'ulong', 'undefined8'): return '/*?*/ int'
    if r.endswith('*'): return '/*?*/ ' + r
    return '/*?*/ ' + r

def params(sig):
    inner = sig[sig.index('(') + 1:sig.rindex(')')]
    inner = inner.replace('LF::', '')
    return '' if inner == 'void' else inner

SKIP = re.compile(r'::(getClass|operator new|getOwnerLObject|_classImp_|~)')

def main():
    cm = classmap()
    for cls in sys.argv[1:]:
        c = cm.get(cls)
        if not c:
            print('// %s: not in the class map' % cls)
            continue
        head = c['head']
        m = re.search(r'bases: (.*)$', head)
        bases = []
        if m:
            for b in m.group(1).split(', '):
                bases.append(b.split(' @')[0].replace('LF::', ''))
        print('// %s' % head)
        print('class %s%s {' % (cls, (' : ' + ', '.join('public ' + b for b in bases)) if bases else ''))
        print('\tLF_DECLARE_CLASS(%s)' % cls)
        print('public:')
        print('\t%s();' % cls)
        virt_names = set()
        for off, sig in c['virt']:
            meth = sig.split('(')[0]
            virt_names.add(meth)
            rt, _ = ret_type(cls, meth)
            const = ' const' if sig.endswith(' const') else ''
            print('\tvirtual %s %s(%s)%s;\t// %#x' % (rt, meth, params(sig), const, off))
        print('\tvirtual %s *getOwnerLObject() { return this; }' % cls)
        for sig in worksheet_funcs(cls):
            if SKIP.search(sig):
                continue
            meth = sig[len(cls) + 2:].split('(')[0]
            if meth == cls or meth in virt_names or '(' not in sig:
                continue
            rt, static = ret_type(cls, meth)
            const = ' const' if sig.endswith(' const') else ''
            print('\t%s%s %s(%s)%s;' % ('static ' if static else '', rt, meth, params(sig[len(cls) + 2:]), const))
        print()
        if c['used']:
            print('\t// %s' % c['used'])
        print('};')
        print()

main()

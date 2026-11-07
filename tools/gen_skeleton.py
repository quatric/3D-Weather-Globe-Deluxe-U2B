#!/usr/bin/env python3
"""Generates the Objective-C source skeleton of one binary from its metadata
(analysis/objc-<name>.json) and Ghidra output (analysis/decomp/<name>-i386).

Every class and category gets a header and an implementation file, laid out
like the original source modules where the binary records their paths. Each
method starts as a stub that calls WG_UNTRANSLATED() and returns zero, with
Ghidra's decompilation above it in a comment. Translating a method means
replacing that comment and stub with the real body; the generator never
touches a file that already exists, so translated files are safe.

usage: gen_skeleton.py <name> <Target> <outdir> [<dependency-target> ...]
"""
import json
import os
import re
import subprocess
import sys

KNOWN_STRUCTS = {
    '_NSRect': 'NSRect', '_NSPoint': 'NSPoint', '_NSSize': 'NSSize',
    '_NSRange': 'NSRange', '_NSZone': 'NSZone', 'CGRect': 'CGRect',
    'CGPoint': 'CGPoint', 'CGSize': 'CGSize', 'CGAffineTransform': 'CGAffineTransform',
    '_NSAffineTransformStruct': 'NSAffineTransformStruct', 'NSEdgeInsets': 'NSEdgeInsets',
    'FSRef': 'FSRef', 'FSSpec': 'FSSpec', 'AEDesc': 'AEDesc', 'Rect': 'Rect',
    'Point': 'Point', 'ProcessSerialNumber': 'ProcessSerialNumber',
    'AudioStreamBasicDescription': 'AudioStreamBasicDescription',
    '_CGLContextObject': 'struct _CGLContextObject',
    '_CGLPixelFormatObject': 'struct _CGLPixelFormatObject',
    'OpaqueEventRef': 'struct OpaqueEventRef',
    '__CFString': 'struct __CFString', '__CFArray': 'struct __CFArray',
    '__CFDictionary': 'struct __CFDictionary', '__CFData': 'struct __CFData',
    '__CFURL': 'struct __CFURL', '__CFRunLoop': 'struct __CFRunLoop',
    '__CFRunLoopSource': 'struct __CFRunLoopSource', '__CFAllocator': 'struct __CFAllocator',
    '__CFXMLNode': 'struct __CFXMLNode', '__CFTree': 'struct __CFTree',
    '__CFNumber': 'struct __CFNumber', '__CFBundle': 'struct __CFBundle',
    '__SCDynamicStore': 'struct __SCDynamicStore', '__SCNetworkReachability': 'struct __SCNetworkReachability',
    'sqlite3': 'sqlite3', 'sqlite3_stmt': 'sqlite3_stmt', 'z_stream_s': 'z_stream',
    '__sFILE': 'FILE', 'objc_class': 'struct objc_class', 'objc_object': 'struct objc_object',
    'objc_selector': 'struct objc_selector', '_opaque_pthread_mutex_t': 'pthread_mutex_t',
    '_opaque_pthread_t': 'struct _opaque_pthread_t', '_opaque_pthread_cond_t': 'pthread_cond_t',
    'timeval': 'struct timeval', 'tm': 'struct tm',
}

# AppKit/Foundation methods whose float/int really are 32 bits wide
FLOAT_SELECTORS = {'floatValue', 'setFloatValue:', 'floatForKey:', 'setFloat:forKey:',
                   'encodeFloat:forKey:', 'decodeFloatForKey:', 'numberWithFloat:',
                   'initWithFloat:'}
INT_SELECTORS = {'intValue', 'setIntValue:', 'integerForKey:', 'numberWithInt:',
                 'initWithInt:', 'encodeInt:forKey:', 'decodeIntForKey:',
                 'encodeInt32:forKey:', 'decodeInt32ForKey:'}


class Types:
    """Turns @encode strings into C type spellings, collecting the
    definitions of the structures the binary declares itself."""

    def __init__(self, prefix):
        self.prefix = prefix
        self.structs = {}   # name -> definition text
        self.order = []
        self.anon = 0
        self.anons = {}

    def decode(self, s, method=False, sel=''):
        t, _ = self._one(s, 0, method, sel)
        return t

    def method_types(self, s, sel):
        out = []
        i = 0
        while i < len(s):
            t, i = self._one(s, i, True, sel)
            out.append(t)
            while i < len(s) and (s[i].isdigit() or s[i] == '-'):
                i += 1
        return out

    def _one(self, s, i, method, sel):
        while s[i] in 'rnNoORV':
            i += 1
        c = s[i]
        i += 1
        simple = {'c': 'BOOL', 'C': 'unsigned char', 's': 'short', 'S': 'unsigned short',
                  'q': 'long long', 'Q': 'unsigned long long', 'd': 'double', 'B': 'bool',
                  'v': 'void', '*': 'char *', '#': 'Class', ':': 'SEL', '?': 'void *'}
        if c in simple:
            return simple[c], i
        if c in 'il':
            return ('NSInteger' if method and sel not in INT_SELECTORS else 'int'), i
        if c in 'IL':
            return ('NSUInteger' if method and sel not in INT_SELECTORS else 'unsigned int'), i
        if c == 'f':
            return ('CGFloat' if method and sel not in FLOAT_SELECTORS else 'float'), i
        if c == '@':
            if i < len(s) and s[i] == '"':
                e = s.index('"', i + 1)
                name = s[i + 1:e]
                return (name + ' *' if name else 'id'), e + 1
            if i < len(s) and s[i] == '?':
                return 'id /* block */', i + 1
            return 'id', i
        if c == '^':
            if s[i] == '?':
                return 'void *', i + 1
            t, i = self._one(s, i, False, sel)
            t = re.sub(r'\[\d+\]', '', t, count=1)   # pointer to array -> element pointer
            return (t + '*' if t.endswith('*') else t + ' *'), i
        if c == 'b':
            j = i
            while j < len(s) and s[j].isdigit():
                j += 1
            return 'unsigned int:' + s[i:j], j
        if c == '[':
            j = i
            while s[j].isdigit():
                j += 1
            n = s[i:j]
            t, j = self._one(s, j, False, sel)
            # the outer dimension comes first in C
            m = re.match(r'^(.*?)((?:\[\d+\])*)$', t)
            return '%s[%s]%s' % (m.group(1), n, m.group(2)), j + 1
        if c in '{(':
            close = '}' if c == '{' else ')'
            j = i
            while s[j] not in '=' + close:
                j += 1
            name = s[i:j]
            if s[j] == close:
                return self._name(name, c), j + 1
            # body
            j += 1
            fields = []
            idx = 0
            while s[j] != close:
                fname = None
                if s[j] == '"':
                    e = s.index('"', j + 1)
                    fname = s[j + 1:e]
                    j = e + 1
                t, j = self._one(s, j, False, sel)
                fields.append((t, fname or 'f%d' % idx))
                idx += 1
            j += 1
            spelled = self._name(name, c)
            if name in KNOWN_STRUCTS:
                return spelled, j
            if name in ('', '?'):
                return self._anon(c, fields), j
            if not fields:
                return spelled, j    # opaque
            if name not in self.structs or ('f0' in self.structs[name] and
                                            not all(f.startswith('f') for _, f in fields)):
                if name not in self.structs:
                    self.order.append(name)
                self.structs[name] = self._define(name, c, fields)
            return spelled, j
        raise ValueError('bad type %r in %r' % (c, s))

    def _name(self, name, c):
        if name in KNOWN_STRUCTS:
            return KNOWN_STRUCTS[name]
        if name in ('', '?'):
            return 'void'
        return ('struct ' if c == '{' else 'union ') + name

    @staticmethod
    def _field(t, f):
        if t.startswith('unsigned int:'):
            return 'unsigned int %s:%s;' % (f, t.split(':')[1])
        m = re.match(r'^(.*?)((?:\[\d+\])+)$', t)
        if m:
            return '%s %s%s;' % (m.group(1), f, m.group(2))
        return '%s %s;' % (t, f) if not t.endswith('*') else '%s%s;' % (t, f)

    def _anon(self, c, fields):
        # anonymous structures get a stable name so that a declaration and
        # its definition agree on the type
        key = (c, tuple(fields))
        if key not in self.anons:
            self.anon += 1
            name = '%s_anon%d' % (self.prefix, self.anon)
            self.anons[key] = name
            self.order.append(name)
            self.structs[name] = self._define(name, c, fields)
        return ('struct ' if c == '{' else 'union ') + self.anons[key]

    def _define(self, name, c, fields):
        kw = 'struct' if c == '{' else 'union'
        body = '\n'.join('    ' + self._field(t, f) for t, f in fields)
        return '%s %s {\n%s\n};\n' % (kw, name, body)


def arg_name(part, used):
    words = re.findall(r'[A-Z]?[a-z0-9]+|[A-Z]+(?![a-z])', part) or ['arg']
    n = 'a' + words[-1][0].upper() + words[-1][1:]
    base, k = n, 2
    while n in used:
        n = '%s%d' % (base, k)
        k += 1
    used.add(n)
    return n


def declaration(m, cls_method, types):
    # arrays passed to methods decay to pointers
    ts = [re.sub(r'^(.*?)\[\d+\]((?:\[\d+\])*)$', r'\1 *', t) if '[' in t else t
          for t in types.method_types(m['types'], m['sel'])]
    ret, args = ts[0], ts[3:]
    sign = '+' if cls_method else '-'
    if not args:
        return '%s (%s)%s' % (sign, ret, m['sel']), ret
    parts = m['sel'].split(':')[:-1]
    used = set()
    out = []
    for part, t in zip(parts, args):
        out.append('%s:(%s)%s' % (part, t, arg_name(part, used)))
    return '%s (%s)%s' % (sign, ret, ' '.join(out)), ret


def zero_return(ret):
    if ret == 'void':
        return ''
    if ret in ('id', 'Class', 'SEL') or ret.endswith('*'):
        return '    return 0;\n'
    if ret.startswith('struct ') or ret in ('NSRect', 'NSPoint', 'NSSize', 'NSRange',
                                           'NSAffineTransformStruct', 'CGRect', 'CGPoint',
                                           'CGSize'):
        return '    return (%s){0};\n' % ret
    return '    return 0;\n'


def sdk_protocols():
    sdk = subprocess.run(['xcrun', '--show-sdk-path'], capture_output=True, text=True).stdout.strip()
    out = subprocess.run(['grep', '-RhoE', r'@protocol [A-Za-z_]+',
                          sdk + '/System/Library/Frameworks/AppKit.framework/Headers/',
                          sdk + '/System/Library/Frameworks/Foundation.framework/Headers/',
                          sdk + '/usr/include/objc/'],
                         capture_output=True, text=True).stdout
    return {l.split()[1] for l in out.splitlines()}


def load_decomp(name):
    base = 'analysis/decomp/%s-i386' % name
    by_addr = {}
    funcs = []
    with open(os.path.join(base, 'index.tsv')) as f:
        for line in f:
            addr, size, fname, file = line.rstrip('\n').split('\t')
            by_addr[int(addr, 16)] = os.path.join(base, file)
            funcs.append((int(addr, 16), fname, os.path.join(base, file)))
    return by_addr, funcs


def comment(path):
    if not path or not os.path.exists(path):
        return ''
    text = open(path, errors='replace').read().replace('*/', '* /')
    text = re.sub(r'/\\* WARNING: Function: ___i686.get_pc_thunk.* \\* /\n\n', '', text)
    return '/* --- i386 decompilation (Ghidra) ---\n%s*/\n' % text


def module_name(mod):
    if mod['path']:
        return os.path.splitext(os.path.basename(mod['path']))[0]
    if mod['classes']:
        return mod['classes'][0]['name']
    if mod['categories']:
        c = mod['categories'][0]
        return '%s+%s' % (c['class'], c['name'])
    return None


def main():
    name, target, outdir = sys.argv[1:4]
    deps = sys.argv[4:]
    modules = json.load(open('analysis/objc-%s.json' % name))
    by_addr, funcs = load_decomp(name)
    types = Types(target)
    known_protocols = sdk_protocols()
    for d in deps:
        # protocols a dependency already declares
        known_protocols |= set(re.findall(r'@protocol (\w+)',
                                          open('src/%s/%sPrefix.h' % (d, d)).read()))
    local_classes = {c['name']: module_name(m) for m in modules for c in m['classes']}
    own_protocols = set()
    os.makedirs(outdir, exist_ok=True)

    files = {}
    for mod in modules:
        mn = module_name(mod)
        if mn is None:
            continue
        h, mm = [], []
        imports = set()
        # superclasses before subclasses
        ordered, seen = [], set()
        names = {c['name']: c for c in mod['classes']}
        def visit(c):
            if c['name'] in seen:
                return
            seen.add(c['name'])
            if c['super'] in names:
                visit(names[c['super']])
            ordered.append(c)
        for c in mod['classes']:
            visit(c)
        for c in ordered:
            sup = c['super']
            if sup in local_classes and local_classes[sup] != mn:
                imports.add('#import "%s.h"' % local_classes[sup])
            for p in c['protocols']:
                if p not in known_protocols:
                    own_protocols.add(p)
            protos = ' <%s>' % ', '.join(c['protocols']) if c['protocols'] else ''
            h.append('@interface %s : %s%s\n{\n' % (c['name'], sup or 'NSObject', protos))
            for iv in c['ivars']:
                t = types.decode(iv['type'])
                h.append('    ' + Types._field(t, iv['name']).replace('unsigned int:', '') + '\n'
                         if not t.startswith('unsigned int:') else
                         '    unsigned int %s:%s;\n' % (iv['name'], t.split(':')[1]))
            h.append('}\n')
            mm.append('@implementation %s\n\n' % c['name'])
            if c['ivars']:
                mm.append('/* i386 ivar offsets: %s */\n\n' % ', '.join(
                    '%s=0x%x' % (iv['name'], iv['offset']) for iv in c['ivars']))
            for key, is_cls in (('cmethods', True), ('imethods', False)):
                for m in c[key]:
                    decl, ret = declaration(m, is_cls, types)
                    h.append(decl + ';\n')
                    mm.append(comment(by_addr.get(int(m['imp'], 16))))
                    mm.append('%s\n{\n    WG_UNTRANSLATED();\n%s}\n\n' % (decl, zero_return(ret)))
            h.append('@end\n\n')
            mm.append('@end\n\n')
        for c in mod['categories']:
            if c['class'] in local_classes and local_classes[c['class']] != mn:
                imports.add('#import "%s.h"' % local_classes[c['class']])
            h.append('@interface %s (%s)\n' % (c['class'], c['name']))
            mm.append('@implementation %s (%s)\n\n' % (c['class'], c['name']))
            for key, is_cls in (('cmethods', True), ('imethods', False)):
                for m in c[key]:
                    decl, ret = declaration(m, is_cls, types)
                    h.append(decl + ';\n')
                    mm.append(comment(by_addr.get(int(m['imp'], 16))))
                    mm.append('%s\n{\n    WG_UNTRANSLATED();\n%s}\n\n' % (decl, zero_return(ret)))
            h.append('@end\n\n')
            mm.append('@end\n\n')
        files.setdefault(mn, [set(), [], [], mod['path']])
        files[mn][0] |= imports
        files[mn][1] += h
        files[mn][2] += mm

    prefix = '%sPrefix.h' % target
    written = 0
    for mn, (imports, h, mm, path) in sorted(files.items()):
        hp = os.path.join(outdir, mn + '.h')
        mp = os.path.join(outdir, mn + '.m')
        origin = '// Original module: %s\n' % path if path else ''
        if not os.path.exists(hp):
            with open(hp, 'w') as f:
                f.write('%s#import "%s"\n%s\n\n%s' % (origin, prefix, ''.join(i + '\n' for i in sorted(imports)), ''.join(h)))
            written += 1
        if not os.path.exists(mp):
            with open(mp, 'w') as f:
                f.write('%s#import "%s.h"\n\n%s' % (origin, mn, ''.join(mm)))
            written += 1

    # plain C functions: everything in __text that isn't a method
    imps = {int(m['imp'], 16) for mod in modules
            for c in mod['classes'] + mod['categories'] for k in ('imethods', 'cmethods')
            for m in c[k]}
    cf = os.path.join(outdir, '%sFunctions.m' % target)
    nfun = 0
    if not os.path.exists(cf):
        with open(cf, 'w') as f:
            f.write('// Plain C functions of the original binary, still to translate.\n'
                    '#import "%s"\n\n#if 0\n' % prefix)
            for addr, fname, path in funcs:
                if addr in imps or 'get_pc_thunk' in fname or fname.startswith(('-[', '+[')):
                    continue
                f.write(comment(path) + '\n')
                nfun += 1
            f.write('#endif\n')

    pp = os.path.join(outdir, prefix)
    if not os.path.exists(pp):
        with open(pp, 'w') as f:
            f.write('// Shared declarations for the %s sources.\n'
                    '#import "WGSystem.h"\n' % target)
            for d in deps:
                f.write('#import "%sUmbrella.h"\n' % d)
            f.write('\n')
            for p in sorted(own_protocols):
                f.write('@protocol %s <NSObject>\n@end\n' % p)
            f.write('\n')
            for cname in sorted(local_classes):
                f.write('@class %s;\n' % cname)
            f.write('\n')
            for sname in types.order:
                f.write(types.structs[sname] + '\n')
    up = os.path.join(outdir, '%sUmbrella.h' % target)
    if not os.path.exists(up):
        with open(up, 'w') as f:
            f.write('// Every %s header, for code that uses the framework.\n' % target)
            for mn in sorted(files):
                f.write('#import "%s.h"\n' % mn)
    print('%s: %d files written to %s, %d C functions listed, %d structs'
          % (target, written, outdir, nfun, len(types.order)))


if __name__ == '__main__':
    main()

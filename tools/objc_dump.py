#!/usr/bin/env python3
"""Reads the Objective-C 1 metadata of a 32-bit Mach-O (via `otool -oV`) into
JSON: one entry per compilation module with its original source path, the
classes and categories it defines, their ivars and their methods.

usage: objc_dump.py <thin-i386-binary> <out.json>
"""
import json
import re
import subprocess
import sys

HEXFIELD = re.compile(r'^\s*([A-Za-z_\[\]0-9 ]+?)\s+(0x[0-9a-f]+)\s*(.*)$')
FIELD = re.compile(r'^\s*([A-Za-z_\[\]0-9]+)\s+()()(\S.*)$')


def parse(text):
    modules = []
    mod = None      # current module
    obj = None      # current class / metaclass / category dict
    kind = None     # 'class', 'meta', 'category'
    in_protocols = False
    pending_method = None
    pending_ivar = None
    classes_by_name = {}

    for line in text.splitlines():
        s = line.strip()
        if s.startswith('Module 0x'):
            mod = {'path': None, 'classes': [], 'categories': []}
            modules.append(mod)
            obj = kind = None
            continue
        if mod is None:
            continue
        m = HEXFIELD.match(line) or FIELD.match(line)
        if not m:
            continue
        key = m.group(1).strip()
        val = (m.group(3) if m.group(2) else m.group(4) or '').strip()

        if key == 'name' and mod['path'] is None and obj is None:
            mod['path'] = val
            continue
        if s.startswith('defs['):
            obj = kind = None
            in_protocols = False
            continue
        if s == 'Meta Class':
            kind = 'meta'
            obj = {}
            in_protocols = False
            continue
        if key == 'isa' and obj is None:
            kind = 'class'
            obj = {'name': None, 'super': None, 'ivars': [], 'imethods': [],
                   'cmethods': [], 'protocols': [], 'instance_size': None}
            in_protocols = False
            continue
        if key == 'category name':
            kind = 'category'
            obj = {'name': val, 'class': None, 'imethods': [], 'cmethods': [],
                   'protocols': []}
            mod['categories'].append(obj)
            in_protocols = False
            cat_section = None
            continue

        if kind == 'category':
            if key == 'class name':
                obj['class'] = val
            elif key == 'instance methods':
                cat_section = 'imethods'
            elif key == 'class methods':
                cat_section = 'cmethods'
            elif key == 'protocols':
                in_protocols = True
            elif key == 'protocol_name' and in_protocols:
                obj['protocols'].append(val)
            elif key == 'method_name' and not in_protocols:
                pending_method = {'sel': val}
            elif key == 'method_types' and pending_method is not None:
                pending_method['types'] = val
            elif key == 'method_imp' and pending_method is not None:
                imp, _, sym = val.partition(' ')
                pending_method['imp'] = m.group(2)
                pending_method['symbol'] = val
                obj[cat_section].append(pending_method)
                pending_method = None
            continue

        if kind == 'class':
            if key == 'super_class':
                obj['super'] = val or None
            elif key == 'name' and obj['name'] is None:
                obj['name'] = val
                mod['classes'].append(obj)
                classes_by_name[val] = obj
            elif key == 'instance_size':
                obj['instance_size'] = int(m.group(2), 16)
            elif key == 'ivar_name':
                pending_ivar = {'name': val}
            elif key == 'ivar_type' and pending_ivar is not None:
                pending_ivar['type'] = val
            elif key == 'ivar_offset' and pending_ivar is not None:
                pending_ivar['offset'] = int(m.group(2), 16)
                obj['ivars'].append(pending_ivar)
                pending_ivar = None
            elif key == 'protocols':
                in_protocols = True
            elif key == 'protocol_name' and in_protocols:
                if val not in obj['protocols']:
                    obj['protocols'].append(val)
            elif key == 'method_name' and not in_protocols:
                pending_method = {'sel': val}
            elif key == 'method_types' and pending_method is not None:
                pending_method['types'] = val
            elif key == 'method_imp' and pending_method is not None:
                pending_method['imp'] = m.group(2)
                pending_method['symbol'] = val
                obj['imethods'].append(pending_method)
                pending_method = None
            continue

        if kind == 'meta':
            # the metaclass follows its class; its methods are class methods
            if key == 'name' and 'cls' not in obj:
                obj['cls'] = classes_by_name.get(val)
            elif key == 'protocols':
                in_protocols = True
            elif key == 'method_name' and not in_protocols:
                pending_method = {'sel': val}
            elif key == 'method_types' and pending_method is not None:
                pending_method['types'] = val
            elif key == 'method_imp' and pending_method is not None:
                pending_method['imp'] = m.group(2)
                pending_method['symbol'] = val
                if obj.get('cls') is not None:
                    obj['cls']['cmethods'].append(pending_method)
                pending_method = None
    return modules


def main():
    text = subprocess.run(['otool', '-oV', sys.argv[1]], capture_output=True,
                          text=True, check=True).stdout
    modules = parse(text)
    with open(sys.argv[2], 'w') as f:
        json.dump(modules, f, indent=1)
    nc = sum(len(m['classes']) for m in modules)
    ncat = sum(len(m['categories']) for m in modules)
    nm = sum(len(c['imethods']) + len(c['cmethods'])
             for m in modules for c in m['classes'] + m['categories'])
    print('%s: %d modules, %d classes, %d categories, %d methods'
          % (sys.argv[1], len(modules), nc, ncat, nm))


if __name__ == '__main__':
    main()

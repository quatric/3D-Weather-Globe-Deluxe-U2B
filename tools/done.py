#!/usr/bin/env python3
"""Removes the UNTRANSLATED blocks of functions that were translated in
another file (a header, or the file that now holds them), and notes where.

usage: done.py <worksheet> <where> <regex>...

Each regex is searched in the block's function name (the text after
"UNTRANSLATED" up to the "@address"). "--all" removes every block.
"""
import re
import sys


def main():
    path, where = sys.argv[1], sys.argv[2]
    pats = sys.argv[3:]
    text = open(path).read()
    removed = []

    def sub(m):
        name = m.group(1)
        if pats == ['--all'] or any(re.search(p, name) for p in pats):
            removed.append(name)
            return ''
        return m.group(0)

    text = re.sub(r'/\* UNTRANSLATED (.*?) @[0-9a-f]+\n.*?\n\*/\n\n?', sub, text, flags=re.S)
    if removed:
        note = '// Translated in %s: %s\n' % (where, ', '.join(sorted(set(n.split('(')[0] for n in removed))))
        lines = text.split('\n')
        # after the header comment
        i = 0
        while i < len(lines) and lines[i].startswith('//'):
            i += 1
        lines.insert(i, note.rstrip('\n'))
        text = '\n'.join(lines)
        open(path, 'w').write(text)
    print('%s: removed %d' % (path, len(removed)))


main()

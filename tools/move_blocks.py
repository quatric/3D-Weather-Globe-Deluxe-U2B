#!/usr/bin/env python3
"""Moves UNTRANSLATED blocks whose function name matches a regex from one
worksheet to the end of another.

usage: move_blocks.py <from> <to> <regex>...
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
pats = sys.argv[3:]
text = open(src).read()
moved = []


def sub(m):
    if any(re.search(p, m.group(1)) for p in pats):
        moved.append(m.group(0))
        return ''
    return m.group(0)


text = re.sub(r'/\* UNTRANSLATED (.*?) @[0-9a-f]+\n.*?\n\*/\n\n?', sub, text, flags=re.S)
open(src, 'w').write(text)
with open(dst, 'a') as f:
    for b in moved:
        f.write('\n' + b)
print('%s -> %s: moved %d' % (src, dst, len(moved)))

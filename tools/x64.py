#!/usr/bin/env python3
# x64.py [-o out] <module> <Class|function>...: the x86_64 decompilation of a
# class's functions (or of the named functions), from
# analysis/decomp/<module>-x86_64, without the thunks, getClass, operator new
# and _classImp_ functions. The x86_64 code keeps the float arguments the i386
# decompilation loses.
import os, re, sys

args = sys.argv[1:]
out = None
if args[:1] == ['-o']:
	out, args = args[1], args[2:]
mod, names = args[0], args[1:]
d = f'analysis/decomp/{mod}-x86_64'
rows = [l.rstrip('\n').split('\t') for l in open(f'{d}/index.tsv')]
res = []
for name in names:
	for addr, size, fname, file in rows:
		if fname != name and not fname.startswith(name + '::'):
			continue
		rest = fname[len(name) + 2:]
		if re.match(r'(getClass|operator_new|operator\.new|getOwnerLObject|_classImp_|~)', rest) or '_classImp_' in fname:
			continue
		text = open(f'{d}/{file}').read()
		if 'non-virtual thunk' in text[:300]:
			continue
		text = re.sub(r'\n\s*\n', '\n', text)
		res.append(text.strip() + '\n')
s = '\n'.join(res)
if out:
	open(out, 'w').write(s)
	print(f'{out}: {s.count(chr(10))} lines')
else:
	print(s)

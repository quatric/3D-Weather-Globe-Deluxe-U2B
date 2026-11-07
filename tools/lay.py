#!/usr/bin/env python3
# Prints the i386 layout of classes as the original binaries had it (LObject
# padded to its old size), flattened to "offset  path  type" lines, so the
# offsets in the decompiled worksheets can be matched with the headers' fields.
#   lay.py [-64] [-i header.h]... Class...
# -64 prints the x86_64 layout, for the offsets in the x86_64 decompilation.
# The headers default to every FishHook, AppModel and RenderLib header.
import glob, os, re, subprocess, sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
headers, classes = [], []
target = 'i386-apple-macos10.6'
args = sys.argv[1:]
while args:
	a = args.pop(0)
	if a == '-64':
		target = 'x86_64-apple-macos10.6'
	elif a == '-i':
		headers.append(args.pop(0))
	else:
		classes.append(a)
if not headers:
	for d in ('FishHook', 'RenderLib'):
		headers += sorted(os.path.basename(h) for h in glob.glob(f'{root}/src/{d}/Include/*.h'))
	headers = ['AppModel.h'] + [h for h in headers if h not in ('C3DViewWrapper.h',)]

src = ''.join(f'#include "{h}"\n' for h in headers)
src += ''.join(f'static char _sz{i}[sizeof({c})];\n' for i, c in enumerate(classes))
incs = []
for d in ('LFoundation', 'FishHook', 'SQLiteLib', 'WebCamsDB_C', 'AppModel', 'RenderLib'):
	incs += ['-I', f'{root}/src/{d}/Include']
p = subprocess.run(['clang++', '-x', 'c++', '-target', target, '-std=gnu++17', '-fsyntax-only',
                    '-DLF_I386_LAYOUT', '-Xclang', '-fdump-record-layouts-complete', '-Wno-everything', *incs, '-'],
                   input=src, capture_output=True, text=True)
out = p.stdout
for line in p.stderr.splitlines():
	if 'error' in line:
		print(line, file=sys.stderr)

blocks = {}
for m in re.finditer(r'\*\*\* Dumping AST Record Layout\n\s+0 \| (?:class|struct|union) ([^\n]+)\n(.*?)\n\s+\| \[sizeof=(\d+)', out, re.S):
	blocks.setdefault(m.group(1).strip(), (m.group(2), int(m.group(3))))

for c in classes:
	key = c if c in blocks else next((k for k in blocks if k.endswith('::' + c)), None)
	if not key:
		print(f'{c}: not found')
		continue
	body, size = blocks[key]
	print(f'// {c}, {target.split("-")[0]} size {size:#x}')
	stack = []
	for line in body.splitlines():
		m = re.match(r'\s*(\d+) \|( *)(.*)', line)
		if not m:
			continue
		off, depth, text = int(m.group(1)), len(m.group(2)) // 2, m.group(3)
		del stack[depth - 1:]
		bm = re.match(r'(?:class|struct) (\S+) \((?:primary )?base\)', text)
		if bm:
			stack.append(bm.group(1).split('::')[-1])
			continue
		if re.match(r'(?:struct|union) \S*\(anonymous', text):
			stack.append('')
			continue
		if 'vtable pointer' in text:
			print(f'  {off:#06x}  {".".join(stack)} vptr')
			continue
		fm = re.match(r'(.*?)\s*(\w+)$', text)
		if fm:
			stack.append(fm.group(2))
			print(f'  {off:#06x}  {".".join(x for x in stack if x)}  {fm.group(1)}')

#!/usr/bin/env python3
"""Counts the UNTRANSLATED blocks left in each target's worksheets, against
the functions the worksheets started with (the generators' counts)."""
import glob
import os
import re

ROOT = os.path.join(os.path.dirname(__file__), '..')
TARGETS = ['LFoundation', 'FishHook', 'SQLiteLib', 'WebCamsDB_C', 'AppModel', 'RenderLib',
           'ScreenSaverLib', 'SMKControls', 'SMKiMovieExport', 'WeatherGlobe',
           'GlobeScreenSaver', 'GlobeImporter', 'AtlasRoute', 'URLCreator']

total_left = total = 0
for t in TARGETS:
    d = os.path.join(ROOT, 'src', t)
    files = glob.glob(os.path.join(d, '*.cpp')) + glob.glob(os.path.join(d, '*.mm')) + \
        glob.glob(os.path.join(d, '*.m')) + glob.glob(os.path.join(d, '*.c'))
    if not files:
        continue
    left = sum(len(re.findall(r'^/\* UNTRANSLATED ', open(f, errors='replace').read(), re.M))
               for f in files)
    start = os.path.join(d, '_count.txt')
    n = int(open(start).read()) if os.path.exists(start) else left
    total_left += left
    total += n
    print('%-17s %5d / %5d translated, %5d left' % (t, n - left, n, left))
print('%-17s %5d / %5d translated, %5d left' % ('total', total - total_left, total, total_left))

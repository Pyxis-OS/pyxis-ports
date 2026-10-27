#!/usr/bin/env python3
"""Retain notices from the actual compiled Mesa source/header closure."""
import argparse
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser()
p.add_argument('source', type=Path)
p.add_argument('build', type=Path)
p.add_argument('output', type=Path)
a = p.parse_args()
source, build = a.source.resolve(), a.build.resolve()
deps = subprocess.check_output(['ninja', '-C', str(build), '-t', 'deps'], text=True)
inputs = subprocess.check_output([
    'ninja', '-C', str(build), '-t', 'inputs',
    'src/gallium/targets/osmesa/libOSMesa.a',
], text=True)
inputs = {(build / x).resolve() for x in inputs.splitlines()}
paths = set()
active = False
for line in deps.splitlines():
    if not line.startswith('    '):
        active = (build / line.split(': #deps', 1)[0]).resolve() in inputs
        continue
    if not active:
        continue
    file = (build / line.strip()).resolve()
    if file.is_file() and (file.is_relative_to(source) or file.is_relative_to(build)):
        paths.add(file)
assert paths, 'No compiler dependency records; build the selected archive first'
blocks = {}
for file in sorted(paths):
    text = file.read_text(errors='replace')
    found = re.findall(r'/\*.*?\*/|(?m:^[ \t]*//[^\n]*(?:\n[ \t]*//[^\n]*)*)', text, re.S)
    found = [x for x in found if re.search(r'copyright|SPDX-License|permission is|redistribution|public.domain|license|licence', x, re.I)]
    for notice in found:
        root = source if file.is_relative_to(source) else build
        name = ('source/' if root == source else 'generated/') + str(file.relative_to(root))
        blocks.setdefault(notice, []).append(name)
with a.output.open('w') as out:
    out.write('Selected static Mesa softpipe/llvmpipe source/header notices.\n'
              'Original embedded notices follow verbatim; identical notices are grouped.\n'
              'Complete standard licence texts accompany this file.\n'
              'Source and generated paths identify attribution, not installed runtime paths.\n\n')
    for notice, names in blocks.items():
        out.write('\n'.join(names) + '\n' + notice + '\n\n')
print(f'MESA_NOTICES files={len(paths)} distinct_notices={len(blocks)}')

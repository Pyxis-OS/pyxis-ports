#!/usr/bin/env python3
"""Configure the selected native static Mesa profile with SDK-owned flags."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

p = argparse.ArgumentParser()
commands = p.add_subparsers(dest='command', required=True)
configure = commands.add_parser('configure')
for name in ('source', 'build', 'cc', 'cxx', 'ar', 'c-flags', 'cxx-flags', 'link-flags', 'llvm-development'):
    configure.add_argument('--' + name, required=True)
merge = commands.add_parser('merge')
for name in ('build', 'ar', 'llvm-development'):
    merge.add_argument('--' + name, required=True)
a = p.parse_args()
build = Path(a.build)
llvm_prefix = Path(a.llvm_development).resolve()

if a.command == 'merge':
    dependencies = json.loads((build / 'mesa/meson-info/intro-dependencies.json').read_text())
    llvm = next(dependency for dependency in dependencies if dependency['name'] == 'LLVM')
    archives = []
    for argument in llvm['link_args']:
        if argument.startswith('-L'):
            continue
        archive = (llvm_prefix / 'lib' / ('lib' + argument[2:] + '.a')
                   if argument.startswith('-l') else Path(argument)).resolve()
        if archive.parent != llvm_prefix / 'lib' or archive.suffix != '.a':
            raise ValueError('LLVM dependency cannot be embedded in the static archive: ' + argument)
        if archive not in archives:
            archives.append(archive)
    if not archives:
        raise ValueError('Meson selected no static LLVM archives')
    library = build / 'libOSMesa.a'
    script = '\n'.join(['create ' + str(library),
                        'addlib ' + str(build / 'mesa/src/gallium/targets/osmesa/libOSMesa.a'),
                        *('addlib ' + str(archive) for archive in archives), 'save', 'end', ''])
    subprocess.run(shlex.split(a.ar) + ['-M'], input=script, text=True, check=True)
    (build / 'llvm-archives.json').write_text(json.dumps([
        {'archive': archive.name, 'sha256': hashlib.sha256(archive.read_bytes()).hexdigest()}
        for archive in archives
    ], indent=2) + '\n')
    raise SystemExit(0)

build.mkdir(parents=True, exist_ok=True)

def flags(value):
    return [x for x in shlex.split(value) if x not in ('-MMD', '-MP', '-nostdinc') and not x.startswith('-std=')]

def compile_flags(value):
    # Meson owns the optimization level; inherited SDK flags must not override it.
    return [x for x in flags(value) if not re.fullmatch(r"-O(?:[0-9]+|s|z|g|fast)?", x)]

def value(items):
    return repr(items)

cross = build / 'pyxis.ini'
cross.write_text("[binaries]\n" + '\n'.join([
    'c = ' + value(shlex.split(a.cc)),
    'cpp = ' + value(shlex.split(a.cxx)),
    'ar = ' + value(shlex.split(a.ar)),
    'llvm-config = ' + value([str(llvm_prefix / 'bin/llvm-config')]),
]) + "\n[properties]\nneeds_exe_wrapper = true\n[built-in options]\n" + '\n'.join([
    'c_args = ' + value(compile_flags(a.c_flags)),
    'cpp_args = ' + value(compile_flags(a.cxx_flags) + ['-fno-exceptions']),
    'c_link_args = ' + value(flags(a.link_flags)),
    'cpp_link_args = ' + value(flags(a.link_flags)),
]) + "\n[host_machine]\nsystem = 'pyxis'\ncpu_family = 'x86_64'\ncpu = 'x86-64'\nendian = 'little'\n")
subprocess.run([
    'meson', 'setup', str(build / 'mesa'), a.source, '--cross-file', str(cross),
    '--wrap-mode=nofallback', '--prefix=/usr',
    '-Dbuildtype=release', '-Doptimization=2', '-Ddefault_library=static', '-Db_staticpic=false',
    '-Dc_std=gnu23', '-Dcpp_rtti=false', '-Dplatforms=[]',
    '-Dgallium-drivers=softpipe,llvmpipe', '-Dvulkan-drivers=[]',
    '-Dglx=disabled', '-Degl=disabled', '-Dgbm=disabled',
    '-Dgles1=disabled', '-Dgles2=disabled', '-Dllvm=enabled', '-Dllvm-orcjit=false',
    '-Dshared-llvm=disabled', '-Dvideo-codecs=[]', '-Dgallium-va=disabled',
    '-Dgallium-rusticl=false', '-Dosmesa=true', '-Dopengl=true',
    '-Dglvnd=disabled', '-Dglx-direct=false', '-Dglx-read-only-text=true',
    '-Dbuild-tests=false', '-Dshader-cache=disabled', '-Dzstd=disabled',
    '-Dzlib=disabled', '-Dxmlconfig=disabled', '-Dlibunwind=disabled',
    '-Dlmsensors=disabled',
], check=True)

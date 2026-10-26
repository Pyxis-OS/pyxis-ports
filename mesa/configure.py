#!/usr/bin/env python3
"""Configure the selected native static Mesa profile with SDK-owned flags."""
import argparse
from pathlib import Path
import re
import shlex
import subprocess

p = argparse.ArgumentParser()
for name in ('source', 'build', 'cc', 'cxx', 'ar', 'c-flags', 'cxx-flags', 'link-flags'):
    p.add_argument('--' + name, required=True)
a = p.parse_args()
build = Path(a.build)
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
    '-Dgallium-drivers=softpipe', '-Dvulkan-drivers=[]',
    '-Dglx=disabled', '-Degl=disabled', '-Dgbm=disabled',
    '-Dgles1=disabled', '-Dgles2=disabled', '-Dllvm=disabled',
    '-Dshared-llvm=disabled', '-Dvideo-codecs=[]', '-Dgallium-va=disabled',
    '-Dgallium-rusticl=false', '-Dosmesa=true', '-Dopengl=true',
    '-Dglvnd=disabled', '-Dglx-direct=false', '-Dglx-read-only-text=true',
    '-Dbuild-tests=false', '-Dshader-cache=disabled', '-Dzstd=disabled',
    '-Dzlib=disabled', '-Dxmlconfig=disabled', '-Dlibunwind=disabled',
    '-Dlmsensors=disabled',
], check=True)

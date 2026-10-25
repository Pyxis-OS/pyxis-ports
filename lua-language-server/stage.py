#!/usr/bin/env python3
"""Stage the declared immutable runtime and complete selected source notices."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil


parser = argparse.ArgumentParser()
for name in ("recipe", "source", "build", "stage", "bee", "parser", "json", "serialization-notice", "sdk"):
    parser.add_argument("--" + name, required=True, type=Path)
args = parser.parse_args()
bundle = args.stage / "bin/lua-language-server.pxb"
app = bundle / "app"
runtime = app / "share/lua-language-server"
metadata = app / "metadata"
licenses = metadata / "licenses"


def copy(source, destination, executable=False):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)
    destination.chmod(0o755 if executable else 0o644)


copy(args.build / "lua-language-server.pxe", app / "bin/lua-language-server.pxe", True)
copy(args.recipe / "manifest.json", bundle / "manifest.json")
entries = re.findall(r'^  "([^"\n]+)",$', (args.recipe / "runtime-files.lua").read_text(), re.M)
if not entries or len(entries) != len(set(entries)):
    raise SystemExit("runtime file list is empty or contains duplicate entries")
manifest = []
for entry in entries:
    path = Path(entry)
    if path.is_absolute() or ".." in path.parts:
        raise SystemExit("invalid runtime path")
    source = args.source / path
    if source.is_symlink() or not source.is_file():
        raise SystemExit("missing or nonregular runtime source: " + entry)
    copy(source, runtime / path)
    manifest.append(hashlib.sha256(source.read_bytes()).hexdigest() + "  " + entry)
metadata.mkdir(parents=True, exist_ok=True)
(metadata / "runtime-files.txt").write_text("\n".join(manifest) + "\n")

for source, name in ((args.source / "LICENSE", "LuaLS-LICENSE"),
                     (args.bee / "LICENSE", "bee-LICENSE"),
                     (args.parser / "LICENSE", "LPegLabel-LICENSE"),
                     (args.json / "LICENSE", "json-LICENSE"),
                     (args.serialization_notice / "LICENSE", "lua-serialize-LICENSE"),
                     (args.recipe.parent / "LICENSE", "MPL-2.0"),
                     (args.recipe.parent / "LICENSING.md", "LICENSING.md")):
    copy(source, licenses / name)
lua_header = (args.bee / "3rd/lua55/lua.h").read_text()
start = lua_header.rindex("Copyright (C)")
finish = lua_header.index("*/", start)
notice = lua_header[start:finish].strip()
if "Permission is hereby granted" not in notice or "THE SOFTWARE IS PROVIDED" not in notice:
    raise SystemExit("embedded Lua licence notice is incomplete")
(licenses / "Lua-LICENSE").write_text(notice + "\n")
copy(args.recipe / "PORT-NOTICE", metadata / "PORT-NOTICE")
sources = ["Makefile", "metadata.lua", "build.lua", "stage.py", "runtime-files.lua",
           "manifest.json", "README.md", "PORT-NOTICE"]
sources += [p.relative_to(args.recipe).as_posix() for directory in ("pyxis", "patches")
            for p in sorted((args.recipe / directory).rglob("*")) if p.is_file()]
for source in sources:
    copy(args.recipe / source, metadata / "pyxis" / source)

for source in sorted((args.recipe / "notices").iterdir()):
    if source.is_file():
        copy(source, licenses / "runtime" / source.name)
for name in ("musl-COPYRIGHT", "musl-TRE-COPYRIGHT", "musl-upstream.md",
             "tlsf.h", "tlsf-upstream.md", "musl-fmaf.c", "npfs/LICENSE",
             "npfs/LICENSING.md", "fat32/LICENSE", "fat32/LICENSING.md"):
    copy(args.sdk / "share/licenses" / name, licenses / "sdk" / name)
copy(args.sdk / "share/toolchain/LICENSE.TXT", licenses / "sdk/toolchain-LICENSE")
copy(args.sdk / "share/toolchain/llvm-revision", metadata / "toolchain-revision")
copy(args.sdk / "manifest.txt", metadata / "sdk-manifest.txt")

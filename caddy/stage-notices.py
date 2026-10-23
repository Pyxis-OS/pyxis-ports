#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Retain notices and checksums for the modules actually compiled into Caddy."""
import os
from pathlib import Path
import subprocess
import sys

source, stage, go, cache = (Path(value) for value in sys.argv[1:5])
tags = sys.argv[5]
version = sys.argv[6]
env = dict(os.environ, GOROOT=str(go), GOTOOLCHAIN="local", GOOS="pyxis",
           GOARCH="amd64", GOAMD64="v1", CGO_ENABLED="0", GOPROXY="off",
           GOSUMDB="off", GOCACHE=str(cache), GOENV="off", GOWORK="off",
           GONOPROXY="none", GOFLAGS="")
modules = set()
lines = subprocess.check_output(
    [str(go / "bin/go"), "-C", str(source), "list", "-mod=vendor", "-deps",
     "-tags=" + tags, "-f", "{{if .Module}}{{.Module.Path}} {{.Module.Version}}{{end}}",
     "./cmd/caddy"], env=env, text=True).splitlines()
for line in lines:
    if line.strip():
        fields = line.split()
        modules.add((fields[0], fields[1] if len(fields) > 1 else "v" + version))
sums = {}
for line in (source / "go.sum").read_text().splitlines():
    name, version, checksum = line.split()
    if not version.endswith("/go.mod"):
        sums[name, version] = checksum
base = stage / "bin/caddy.pxb/app/metadata"
(base / "dependencies").mkdir(parents=True, exist_ok=True)
(base / "caddy").mkdir(parents=True, exist_ok=True)
with (base / "caddy/modules.txt").open("w") as output:
    output.write("# Built modules; original go.sum pins (patches recorded in PORT-NOTICE)\n")
    for name, version in sorted(modules):
        checksum = "main-source-archive" if name == "github.com/caddyserver/caddy/v2" else sums[name, version]
        output.write(f"{name} {version} {checksum}\n")
with (base / "dependencies/NOTICES.txt").open("wb") as output:
    for name, version in sorted(modules):
        if name == "github.com/caddyserver/caddy/v2":
            continue
        root = source / "vendor" / name
        notices = []
        for path in root.rglob("*"):
            if path.is_file() and path.name.lower().startswith(
                    ("license", "copying", "notice", "copyright", "authors", "patents")):
                notices.append(path)
        if not notices:
            raise RuntimeError(f"no upstream notices for compiled module {name} {version}")
        for path in sorted(notices):
            output.write(f"\n===== {name} {version}: {path.relative_to(root)} =====\n\n".encode())
            output.write(path.read_bytes())
            output.write(b"\n")

#!/usr/bin/env python3
"""Build only the public, asset-free LUS qualification fixture."""
from pathlib import Path
import json
import sys
import zipfile

source, output = map(Path, sys.argv[1:])
entries = {
    "version": bytes([1, 0, 0, 0, 0]),
    "portVersion": bytes([1, 0, 9, 0, 3, 0, 1]),
    "fixture/message.json": b'{"message":"Hello from a public O2R fixture","value":42}\0',
    "fixture/message.json.meta": json.dumps({"format":"Binary", "type":"Json", "version":0}).encode() + b"\0",
}
entries["fixture/bulk.json"] = b'{"value":43}\0'
entries["fixture/bulk.json.meta"] = entries["fixture/message.json.meta"]
for shader in (source / "src/fast/shaders/opengl").glob("*.glsl"):
    entries["shaders/opengl/" + shader.name] = shader.read_bytes()
with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
    for name, data in sorted(entries.items()):
        info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        archive.writestr(info, data)

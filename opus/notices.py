"""Retain file notices from the compiled Opus sources and source header tree."""
import json
from pathlib import Path
import re
import sys

source, build, output = map(Path, sys.argv[1:])
files = {Path(entry["file"]) for entry in json.loads((build / "compile_commands.json").read_text())}
for tree in ("include", "celt", "silk"):
  files.update((source / tree).rglob("*.h"))
with output.open("w") as notices:
  notices.write("Opus compiled source and header notices. Header inventory also includes\n"
                "unselected architecture variants; COPYING gives the package terms.\n\n")
  for path in sorted(files):
    text = path.read_text()
    blocks = re.match(r"\s*((?:/\*.*?\*/\s*)+)", text, re.S)
    if blocks and re.search(r"copyright|license|licence|redistribution", blocks[1], re.I):
      notices.write(str(path.relative_to(source)) + "\n" + blocks[1] + "\n")

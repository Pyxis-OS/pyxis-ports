#!/usr/bin/env python3
"""Stage the licences and embedded notices reached by the static profile."""
import pathlib
import shutil
import sys

source, native, recipe, destination = map(pathlib.Path, sys.argv[1:])
ft = native / "FTHB-prefix/src/FTHB-build/FT2-prefix/src/FT2"
hb = native / "FTHB-prefix/src/FTHB-build/HB-prefix/src/HB"
files = {
    "libraqm-MIT": source / "deps/libraqm/COPYING",
    "SheenBidi-Apache-2.0": source / "deps/SheenBidi/LICENSE",
    "libunibreak-zlib": source / "deps/libunibreak/LICENCE",
    "plutosvg-MIT": source / "deps/plutosvg/LICENSE",
    "plutovg-MIT": source / "deps/plutosvg/plutovg/LICENSE",
    "plutovg-FTL": source / "deps/plutosvg/plutovg/source/FTL.TXT",
    "FreeType-LICENSES": ft / "LICENSE.TXT",
    "FreeType-FTL": ft / "docs/FTL.TXT",
    "FreeType-BDF": ft / "src/bdf/README",
    "FreeType-PCF": ft / "src/pcf/README",
    "FreeType-hash-source": ft / "src/base/fthash.c",
    "FreeType-zlib-header": ft / "src/gzip/zlib.h",
    "HarfBuzz-Old-MIT": hb / "COPYING",
    "HarfBuzz-Microsoft-MIT": hb / "src/ms-use/COPYING",
    "HarfBuzz-algorithms-source": hb / "src/hb-algs.hh",
    "hashmap-source": source / "src/hashmap.c",
    "Khronos-khrplatform.h": source / "demo/glad/include/KHR/khrplatform.h",
    "stb-image-header": source / "deps/ft-hb-bundle/stb_image.h",
    "plutovg-stb-image-header": source / "deps/plutosvg/plutovg/source/plutovg-stb-image.h",
    "plutovg-stb-image-write-header": source / "deps/plutosvg/plutovg/source/plutovg-stb-image-write.h",
    "plutovg-stb-truetype-header": source / "deps/plutosvg/plutovg/source/plutovg-stb-truetype.h",
}
destination.mkdir(parents=True, exist_ok=True)
for name, path in files.items():
    shutil.copyfile(path, destination / name)
for path in (recipe / "notices").iterdir():
    if path.is_file():
        shutil.copyfile(path, destination / path.name)
(destination / "ATTRIBUTION").write_text(
    "blurgtext: Copyright 2024 Callum McGing and contributors, MIT.\n"
    "SheenBidi: Copyright 2014-2026 Muhammad Tayyab Akram, Apache-2.0.\n"
    "hashmap: Copyright 2020 Joshua J Baker, MIT; SipHash by Aumasson and Bernstein, CC0; "
    "MurmurHash3 by Austin Appleby, public domain; xxHash copyright 2012-2021 Yann Collet, BSD-2-Clause.\n"
    "stb headers: MIT option selected; original full headers retained.\n"
    "GLAD generated loader: CC0 option selected; generation metadata retained in source. "
    "Khronos registry material retains its Apache-2.0/permissive notices.\n"
    "Portions of this software are copyright 2026 The FreeType Project (www.freetype.org). "
    "All rights reserved. FreeType License option selected, including PlutoVG's copied FreeType code.\n"
    "FreeType uses blurgtext's upstream stb PNG shim. Its embedded zlib, BDF, PCF and hash notices are retained.\n"
    "HarfBuzz uses Old MIT plus Microsoft USE, Zilong Tan fasthash and Unicode notices.\n"
    "Generated Unicode data in HarfBuzz, SheenBidi and libunibreak retains Unicode-3.0.\n"
    "Fonts are separately installed read-only assets; each family retains its own licence.\n",
    encoding="utf-8",
)

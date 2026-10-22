# Licensing

Except for the third-party material identified below and files with their own
license notices, original Pyxis source code, build/configuration files and
project documentation in this repository are licensed under the Mozilla Public
License, version 2.0. The complete, unmodified license is in [LICENSE](LICENSE).

This Source Code Form is subject to the terms of the Mozilla Public License,
v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain
one at https://mozilla.org/MPL/2.0/.

This directory-level notice applies to the original material described above;
it does not replace third-party notices. No Exhibit B incompatibility notice is
applied: MPL's standard secondary-license provisions remain available.

## Upstream ports and data

MPL applies to original Pyxis runner code, build recipes, configuration and
project documentation. It does not replace the license of software or data
those recipes fetch, adapt or package.

- Each port's `metadata.lua`, README and source notices identify its upstream
  license, pinned revision and local changes.
- `fonts/notices/` contains verbatim third-party notices, pinned by original
  URL and SHA-256 in [fonts/metadata.lua](fonts/metadata.lua). The staged font
  files retain OFL-1.1 and their embedded Nerd icon terms: MIT, CC-BY-4.0,
  Apache-2.0, OFL-1.1, Bitstream Vera and Unlicense. See
  [fonts/README.md](fonts/README.md) and the staged per-family `source.txt`
  attribution; the original recipe and documentation remain MPL-2.0.
- `libuv/patches/` and `libuv/pyxis/watchers.c` retain upstream libuv's MIT
  license. Its other native platform files, recipe and documentation are original
  MPL-2.0 material; see `libuv/PORT-NOTICE`.
- Upstream-derived files and patches, including `busybox/patches/`, `doom/patches/`, `jq/patches/`,
  `kilo/patches/`, `links/patches/`, `lua/patches/`, `sbase/patches/`, `fastfetch/patches/`,
  `quake/patches/` and `sdl2/patches/`, retain the licenses of the upstream files they modify. This
  includes applicable runtime exceptions.
- jq's `COPYING` is staged unmodified. It states jq's MIT terms, David Gay's dtoa
  notice (a permissive Lucent grant) and decNumber's ICU License, which cover the
  compiled sources; see [jq/PORT-NOTICE](jq/PORT-NOTICE). The recipe, `Makefile`
  and README are original MPL-2.0 material.
- `quake/sys_pyxis.c` derives from quakegeneric's `sys_null.c` and keeps its
  GPL-2.0-or-later notice.
- `busybox/libbb.h`, `busybox/*_config.h`, `busybox/*_support.h`,
  `busybox/busybox_pyxis.c` and `busybox/tar_pyxis.c` replace the selected
  BusyBox libbb and configuration interfaces for vi, less and tar. They are
  GPL-2.0-only, matching the BusyBox programs they are linked into.
- `links/pyxis.c`, `links/pyxis_console.c`, `links/pyxis_console.h`,
  `links/network.c`, `links/config.h` and the headers under `links/include/`
  are the platform layer and configuration linked into Links. Like Links,
  whose sources say "released under GPL" and whose `COPYING` is GPL version 2,
  they are distributed under the GPL.
- `lua51/patches/` retain the licenses of the upstream files they modify: MIT
  for Lua 5.1 and lua-compat-5.3, Apache-2.0 for luv. The `src/pyxis.c` the luv
  patch adds copies luv's connect callback and is Apache-2.0 like luv.
- `lua/main.c` retains Lua's MIT terms for its upstream-derived CLI flow and
  local adaptation, as documented in [lua/README.md](lua/README.md). The staged
  upstream `lua.h` contains the complete license notice.
- The Mozilla-derived CA data retains its own MPL notice and source provenance
  in [ca-certificates/NOTICE](ca-certificates/NOTICE) and
  [LICENSE](ca-certificates/LICENSE). This project-wide choice does not replace
  that upstream attribution.
- The PCI ID database is distributed under the 3-clause BSD option of its
  dual license, as recorded in [pciids/NOTICE](pciids/NOTICE) and
  [LICENSE](pciids/LICENSE). The staged `pci.ids` keeps its upstream header.
- The USB ID database is distributed under the project site's 3-clause BSD
  option, as recorded in [usbids/NOTICE](usbids/NOTICE) and
  [LICENSE](usbids/LICENSE). The staged `usb.ids` keeps its upstream header;
  the Git mirror's GPLv3 LICENSE is distinct from the site's database grant.
- The SDL2 backend in `sdl2/pyxis/` and `sdl2/SDL_config.h` are original
  Pyxis material under MPL-2.0, as recorded in `sdl2/PORT-NOTICE`. SDL itself
  and the `sdl2/patches/` that modify it remain under SDL's zlib license.
- `devilutionx/patches/` modify DevilutionX and remain under its Sustainable
  Use License. The program it builds also links GPL and LGPL libraries, so
  an image containing it is for the person who built it and must not be
  shared; see `devilutionx/PORT-NOTICE`. Pyxis builds it only on request.
- `eduke32/patches/` modify EDuke32 and remain under the licence of each file
  they change: the GPL-2.0 for game code and Ken Silverman's Build licence for
  the engine. An image containing EDuke32 is for the person who built it and
  must not be shared; see `eduke32/PORT-NOTICE`. Pyxis builds it only on
  request.
- Fastfetch's new native adapter/build files and project ASCII logo are original
  Pyxis material under MPL-2.0, as marked in the patch and `fastfetch/PORT-NOTICE`.
  Bundled yyjson remains MIT; its staged header preserves its notice.
- `wpa-supplicant/patches/` retain the upstream files' BSD-3-Clause terms.
  The reached internal AES/hash sources include public-domain implementation
  notices; the recipe stages those and all reached copyright notices with the
  upstream README and COPYING. Its original native profile/recipe/documentation
  remain MPL-2.0; see `wpa-supplicant/PORT-NOTICE`.
- Imported libraries, generated trust/zone data, staged headers, game assets and
  other downloaded inputs retain their individual upstream terms. Consult each
  recipe and staged license directory rather than treating a bundle as all MPL.

Preserve notices and source-availability obligations when distributing patched
upstream binaries. Adding a new recipe does not authorize relicensing its input.

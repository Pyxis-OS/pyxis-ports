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
- Upstream-derived files and patches, including `doom/patches/`, `kilo/patches/`,
  `lua/patches/`, `sbase/patches/`, `fastfetch/patches/`, `tcc/patches/` and
  `quake/patches/`, retain the licenses of the upstream files they modify. This
  includes applicable runtime exceptions.
- `quake/sys_pyxis.c` derives from quakegeneric's `sys_null.c` and keeps its
  GPL-2.0-or-later notice.
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
- Fastfetch's new native adapter/build files and project ASCII logo are original
  Pyxis material under MPL-2.0, as marked in the patch and `fastfetch/PORT-NOTICE`.
  Bundled yyjson remains MIT; its staged header preserves its notice.
- Imported libraries, generated trust/zone data, staged headers, game assets and
  other downloaded inputs retain their individual upstream terms. Consult each
  recipe and staged license directory rather than treating a bundle as all MPL.

Preserve notices and source-availability obligations when distributing patched
upstream binaries. Adding a new recipe does not authorize relicensing its input.

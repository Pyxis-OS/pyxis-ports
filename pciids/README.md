# PCI ID database

The recipe stages `pci.ids` from the [PCI ID Project](https://pci-ids.ucw.cz/)
at the pinned [pciids](https://github.com/pciutils/pciids) commit recorded in
`metadata.lua`. The file is installed unchanged as `share/hwdata/pci.ids` for
native hardware tools and ordinary text use. No compiled database, update
service or pciutils port is involved.

`share/licenses/pciids` contains the 3-clause BSD license elected from upstream's
GPL-2.0-or-later or BSD-3-Clause choice, and a notice describing that election.
`share/pciids/source.txt` records the source URL, commit and database version.

Missing database entries are normal: consumers must keep numeric identification
usable. Updates are manual: select a new upstream commit, update `metadata.lua`,
rebuild the image and review consumer output. Do not validate against fixed line
counts or specific entries of a particular snapshot.

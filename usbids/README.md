# USB ID database

The recipe stages `usb.ids` from the [USB ID Repository](https://usb-ids.gowdy.us/)
at the pinned [usbids mirror](https://github.com/usbids/usbids) commit recorded
in `metadata.lua`. The file is installed unchanged as `share/hwdata/usb.ids` for
native hardware tools and ordinary text use. No compiled database, update service
or usbutils port is involved.

`share/licenses/usbids` contains the 3-clause BSD license elected from the project
site's GPL-2.0-or-later or BSD-3-Clause grant for the database, and a notice
distinguishing that grant from the mirror repository's GPLv3 LICENSE.
`share/usbids/source.txt` records the source URL, commit, database version and
license source. The upstream maintainer header remains in the database.

Missing database entries are normal: consumers must keep numeric identification
usable. Updates are manual: select a new upstream commit, update `metadata.lua`,
rebuild the image and review consumer output. Do not validate against fixed line
counts or specific entries of a particular snapshot.

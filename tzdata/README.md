# IANA timezone data

Pin: tzdb/tzcode **2026d**, commit
`d633fe7ed3de8e00ce7cac991376a064a1373bb1` from the
[upstream repository](https://github.com/eggert/tz/tree/d633fe7ed3de8e00ce7cac991376a064a1373bb1).
The same source supplies the native host `zic` and its input data. No patches
are applied. The recipe supplies the generated release `version` file instead
of deriving it from tags absent in a commit-only fetch. Neither the installed
host `zic` nor host zoneinfo files are consulted. The host C compiler builds zic; no target program is linked
against host libc. The common runner still selects an SDK, but this recipe has
no Pyxis library dependencies.

Build upstream's `main` data with `backward`, then compile slim TZif files with
no date-range cutoff and no leap-second table. This includes all standard zones
and aliases, not a geographic subset. Optional `backzone` historical alternatives
are not enabled. Do not add a leap-counting `right` tree: Pyxis timestamps are
Unix seconds without distinct leap seconds. No host/system localtime is changed.

`share/zoneinfo` contains the compiled zones/aliases, `tzdata.zi`, the release
version, country table and zone-selection tables. Aliases are copied into
independent regular files so the boot archive requires no link support. The
host zic executable and build intermediates stay out of the guest payload.
`share/tzdata/source.txt` records the pin/compiler options, and
`share/licenses/tzdata/LICENSE` preserves upstream's license notice. The selected
code/data are public domain; upstream's notice also identifies BSD-licensed
files that are not compiled or installed by this recipe.

The files carry historical transitions and TZif future-rule footers. A runtime
reader must interpret those rules beyond the explicit transitions; freezing the
last UTC offset is incorrect. This recipe does not implement timezone conversion
or choose a timezone. Pyxis defaults to UTC when `TZ` is absent; Bucharest is an
explicit selection, eventually supplied by userspace configuration.

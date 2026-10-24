# OpenArena data

Stages the nine unchanged OpenArena 0.8.8 `baseoa` PK3 files under
`share/openarena/baseoa`, the upstream notices under `share/licenses/openarena`,
and archive/member provenance in `share/openarena/source.txt`. The parent
OpenArena package should place these PK3 files at `baseoa/` and the notices at
`licenses/openarena/`. Preserve case, including `pak5-TA.pk3`.

This recipe stages runtime data and compiled game QVMs. It does not build an
engine or stage the ZIP's foreign engines, shared libraries or `missionpack`.
Build inputs come from the pinned internal archive cache. The package needs
the separate Pyxis ioquake3 bundle.

Public binary package publication still requires a corresponding-source
distribution arrangement. The runtime ZIP is insufficient: exported assets
need their preferred source forms and QVMs need their matching C sources and
build material. See `PORT-NOTICE` for the reviewed licences and source gap.

## Inventory checked on 2026-10-24

The ZIP SHA-256 matches `metadata.lua`. Each pack's full directory inventory
matches Debian's `openarena-data` 0.8.5split-17 `debian/*.pk3.contents`, including
all 5,433 names, CRC-32 values and ordering. No ZIP or PK3 entry has an absolute
path, a `..` component or a symlink type. The files are copied unchanged,
preserving their inner notices and upstream pure-server checksums.

| PK3 | Debian copyright review | Licence |
| --- | --- | --- |
| `pak0.pk3` | openarena-data 0.8.5split-17 | GPL-2.0-or-later |
| `pak1-maps.pk3` | openarena-maps 0.8.5split-17 | GPL-2.0-or-later |
| `pak2-players-mature.pk3` | openarena-players-mature 0.8.5split-17 | GPL-2.0-or-later |
| `pak2-players.pk3` | openarena-players 0.8.5split-17 | GPL-2.0-or-later; merman GPL-2.0-only |
| `pak4-textures.pk3` | openarena-textures 0.8.5split-17 | GPL-2.0-or-later |
| `pak5-TA.pk3`, `pak6-misc.pk3` | openarena-misc 0.8.5split-17 | GPL-2.0-or-later |
| `pak6-patch085.pk3` | openarena-085-data 0.8.5split-17 | GPL-2.0-or-later |
| `pak6-patch088.pk3` | openarena-088-data 0.8.8-17 | GPL-2.0-or-later; oa_thor GPL-2.0-only; pul1duel notice says GPLv2 |

The pack filenames and SHA-256 values are recorded in `metadata.lua`.
`pak0.pk3`, `pak6-patch085.pk3` and `pak6-patch088.pk3` each contain exactly
`vm/cgame.qvm`, `vm/qagame.qvm` and `vm/ui.qvm`. The last patch supplies the
effective 0.8.8 modules when normal PK3 override order applies.
Their C runtime replacement code also carries the BSD-3-Clause notice retained
in `PORT-NOTICE`.

`pak6-patch088.pk3` contains seven music files named `music/OA01.ogg`,
`OA02.ogg`, `OA06.ogg`, `OA07.ogg`, `OA08.ogg`, `OA13.ogg` and `OA14.ogg`
under `music/`, plus Ogg player sounds. Retain ioquake3's Vorbis decoder.

The same pack contains `demos/demo088-test1.dm_71`, a protocol-71 recording.
`pak6-misc.pk3` contains nine protocol-70 recordings under `demos/`:
`flyby_compromise.dm_70`, `flyby_hydronex.dm_70`, `flyby_oaspirit.dm_70`,
`flyby_oa_dm3.dm_70`, `flyby_suspended.dm_70`, `stress_bases3.dm_70`,
`stress_q3dm6ishv2.dm_70`, `stress_sago.dm_70` and `stress_schism.dm_70`.
Some reference removed maps; these are an inventory, not a playback claim.
`demo088-test1.dm_71` is the first candidate for manual playback qualification.

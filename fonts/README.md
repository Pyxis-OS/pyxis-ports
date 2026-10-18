# Font assets

This data-only recipe stages nine unchanged static TrueType fonts under
`share/fonts/`: CaskaydiaCove Nerd Font Regular, Bold, Italic and Bold Italic
(Nerd Fonts v3.5.1), IBM Plex Sans Regular and Bold (1.1.0), and IBM Plex Serif
Regular, Bold and Italic (2.0.0). Pyxis includes this shared data in its ordinary
boot archive. The SDK exports libfont separately; these files are runtime data.

```sh
lua build.lua fonts --sdk /path/to/pyxis/build/sdk
```

Inputs come only from the owner's cached release archives, with mandatory
SHA-256 verification. The flat Nerd Fonts tar preserves its paths; the two Plex
ZIPs are extracted with Python 3's standard library. Existing builder images
already provide Python 3; no compiler-container rebuild is needed. No font
compiler, patcher, subsetting, renaming or target compilation is involved.

`metadata.lua` is the staged-file inventory and records exact archive and member
hashes. It selects the plain NerdFont files, excluding Mono/Propo variants, and
Plex's `fonts/complete/ttf/` files. CaskaydiaCove text is monospaced; some icons
have wider advances. Each family directory includes its original OFL notice
and generated `source.txt` with styles, byte counts, hashes and upstream URLs.
CaskaydiaCove also retains the release README and full icon-source notices under
`icons-notices/`. Verbatim vendored notices retain their upstream licences;
their exact sources and hashes are in the inventory and staged provenance.

The Cascadia base and Plex fonts are OFL-1.1. Embedded Nerd icons also use
CC-BY-4.0, MIT, Apache-2.0, Bitstream Vera and Unlicense terms; see the staged
individual notices and attribution table. MaterialDesign retains the full
Apache text and Pictogrammers statement. Hack retains both MIT and Bitstream
notices. Font Awesome Extension's MIT classification comes from the pinned
Nerd Fonts metadata; no separate original-author licence was independently
verified. The already patched release supplies the renamed CaskaydiaCove
family; this recipe makes no modified font.

The nine fonts total 12,629,928 bytes. Original notices add 94,098 bytes, before
the generated provenance and boot-archive headers/padding. A 13 MiB planning
allowance is an estimate, not a runtime limit. No terminal integration, font
discovery, default selection or hosted-Clang example is included here.

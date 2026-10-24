# Font assets

This data-only recipe stages thirty unchanged TrueType/OpenType fonts under
`share/fonts/`: CaskaydiaCove Nerd Font Regular, Bold, Italic and Bold Italic
(Nerd Fonts v3.5.1), IBM Plex Sans and IBM Plex Serif Regular, Bold, Italic and BoldItalic
(1.1.0 and 2.0.0), Noto Sans and Noto Serif Regular, Bold, Italic
and Bold Italic (release 2.015), and Noto Sans Mono Regular and Bold (release
2.014, which has no italics). Pyxis prepares this tree only for the `fonts`
package, outside the boot archive. The SDK exports libfont separately; these files are runtime data.

```sh
lua build.lua fonts --sdk /path/to/pyxis/build/sdk
```

Inputs come only from the owner's cached release archives, with mandatory
SHA-256 verification. The flat Nerd Fonts tar preserves its paths; the two Plex
ZIPs and the three Noto ZIPs are extracted with Python 3's standard library. Existing builder images
already provide Python 3; no compiler-container rebuild is needed. No font
compiler, patcher, subsetting, renaming or target compilation is involved.

`metadata.lua` is the staged-file inventory and records exact archive and member
hashes. Staged copies are checked against those hashes before provenance is
written. It selects the plain NerdFont files, excluding Mono/Propo variants, and
Plex's `fonts/complete/ttf/` files and the `hinted/ttf/` static files of the Noto
ZIPs (Sans, Serif and Sans Mono come from separate `notofonts/latin-greek-cyrillic`
releases). CaskaydiaCove text is monospaced; some icons have wider advances. Each
family directory includes its applicable original licence (the Noto families also their
`AUTHORS.txt`, vendored once under `notices/Noto/`)
and generated `source.txt` with styles, byte counts, hashes and upstream URLs.
CaskaydiaCove also retains the release README and full icon-source notices under
`icons-notices/`. Verbatim vendored notices retain their upstream licences;
their exact sources and hashes are in the inventory and staged provenance.

The Cascadia base, Plex and Noto fonts are OFL-1.1. Roboto is Apache-2.0;
Adobe EmojiOne Color is MIT. Embedded Nerd icons also use
CC-BY-4.0, MIT, Apache-2.0, Bitstream Vera and Unlicense terms; see the staged
individual notices and attribution table. MaterialDesign retains the full
Apache text and Pictogrammers statement. Hack retains both MIT and Bitstream
notices. Font Awesome Extension's MIT classification comes from the pinned
Nerd Fonts metadata; no separate original-author licence was independently
verified. The already patched release supplies the renamed CaskaydiaCove
family; this recipe makes no modified font.

The thirty fonts total 41,705,932 bytes. Original notices and generated
provenance are additional package files. The earlier 13 MiB planning allowance
covered the first nine fonts only. This recipe supplies data; font discovery
and selection belong to consumers.

## Demo and multilingual additions

The single `fonts` package also carries the six unchanged Roboto 2.137 files
from blurgtext's pinned demo (Regular, Bold, Italic, BoldItalic, ThinItalic and
Black), Noto Sans CJK SC 2.004 Regular, and Adobe EmojiOne Color 1.000. The latter
is the demo's exact OpenType-SVG font, under MIT, not OFL. CJK is single-face
OpenType/CFF under OFL-1.1; the three demo characters `成就感` are covered. Roboto's
embedded Apache-2.0 grant and Google copyright are retained, with the full Apache
text from the pinned official Roboto repository. No font bytes are altered.

Plex Sans gains its actual Italic and BoldItalic; Plex Serif gains BoldItalic,
from their existing release archives. Noto Sans Mono's pinned release has no
italic fonts, and Noto CJK's selected face is upright; neither is relabelled or
synthetically slanted. This adds 22,334,776 font bytes. The asset inventory and build output describe the complete package. These additions were accepted (chronium, 2026-10-24).

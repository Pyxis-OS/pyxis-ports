# fmt

The {fmt} 12.2.0 formatting library, under the MIT license, built as a static
C++ library against the SDK's libc++. The source is pinned to the `12.2.0`
commit and fetched only from the owner's `mirrors/fmt` mirror.
`dev/share/fmt/source.txt` records the commit, patches and profile, and
`dev/share/licenses/fmt/LICENSE` keeps the upstream license.

```sh
lua build.lua fmt --sdk /path/to/pyxis/build/sdk
```

The recipe builds upstream `src/format.cc` with CMake and the SDK's C++
settings. `FMT_OS` is off: `fmt/os.h` wraps POSIX descriptors and processes,
which Pyxis libc does not provide. Shared libraries, modules, documentation and
upstream tests are not built.

The SDK's libc++ provides localization and wide characters. The source is
unpatched and uses upstream's locale and wide-character support; locale
behavior follows the SDK's C locale. `os.h` needs the omitted `FMT_OS`
implementation. Formatting errors throw `fmt::format_error`.

The Pyxis console shows the 16 standard colors and reverse video only. Use
`fmt::terminal_color` with `fg`/`bg` from `color.h`: named `fmt::color` values
and RGB colors produce 24-bit escape sequences that the console ignores, and it
ignores emphasis such as bold.

Development outputs are `stage/dev/lib/libfmt.a` and `stage/dev/include/fmt`,
installed by fmt's own CMake install. It also writes `lib/cmake/fmt`, so CMake
projects using the SDK's `share/pyxis.cmake` find `fmt::fmt` with
`find_package(fmt)`, and `lib/pkgconfig/fmt.pc`.
Pyxis exports them at `build/ports-dev/fmt`, outside the base and guest SDK.
Consumers compile with that prefix's `include` path and the SDK's C++ settings,
and link `libfmt.a` before the SDK runtime libraries. No program in the boot
archive uses fmt yet, so nothing from it enters the boot payload. A port that
ships an fmt program must also install fmt's license into the boot payload.

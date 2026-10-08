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

The SDK's libc++ is built without localization or wide characters. Two patches
make fmt follow libc++'s own feature macros, so the library and its consumers
agree without extra flags:

- `0001` sets `FMT_USE_LOCALE` to 0 when libc++ has no localization. The `L`
  specifier then follows upstream's fallback: integers stay ungrouped, while
  floating-point values use `.` and group thousands with `,`
  (`{:L}` gives `1234567` and `1,234.5`).
- `0002` omits `utf8_to_utf16::str()`, a Windows helper returning
  `std::wstring`, when libc++ has no wide characters.

`args.h`, `base.h`, `color.h`, `compile.h`, `core.h`, `format.h` and `ranges.h`
are usable. `chrono.h`, `ostream.h`, `std.h` and `xchar.h` need `std::locale`,
`printf.h` needs `std::wstring`, and `os.h` needs `FMT_OS`; including any of
them fails to compile. Formatting errors throw `fmt::format_error`.

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

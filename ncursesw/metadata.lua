return {
  source = {
    url = "https://invisible-island.net/ncurses/",
    archive = {
      url = "https://ftp.gnu.org/gnu/ncurses/ncurses-6.6.tar.gz",
      mirror = "https://repo.internal/repository/raw-gnu/gnu/ncurses/ncurses-6.6.tar.gz",
      sha256 = "355b4cbbed880b0381a04c46617b7656e362585d52e9cf84a67e2009b749ff11",
    },
  },
  version = "6.6",
  license = "ncurses",
  dependencies = {
    host = { "make", "cc", "curl", "sha256sum", "tar", "gzip", "git", "awk", "tic", "infocmp" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-native-terminal-driver.patch" },
  outputs = {
    library = "dev/lib/libncursesw.a",
    header = "dev/include/curses.h",
    header_alias = "dev/include/ncurses.h",
    term_header = "dev/include/term.h",
    termcap_header = "dev/include/termcap.h",
    unctrl_header = "dev/include/unctrl.h",
    dll_header = "dev/include/ncurses_dll.h",
    pc = "dev/lib/pkgconfig/ncursesw.pc",
    pc_alias = "dev/lib/pkgconfig/ncurses.pc",
    terminfo_source = "dev/share/terminfo/pyxis.ti",
    terminfo = "dev/share/terminfo/p/pyxis",
    license = "dev/share/licenses/ncursesw/COPYING",
    provenance = "dev/share/ncursesw/source.txt",
    executable = "bin/ncurses-demo.pxb/app/bin/ncurses-demo.pxe",
    manifest = "bin/ncurses-demo.pxb/manifest.json",
  },
}

return {
  source = {
    url = "https://github.com/Henrique194/chocolate-quake.git",
    commit = "8ee22175e3a5613e94ad2f9776aadc428b948cf3",
    archive = {
      url = "https://github.com/Henrique194/chocolate-quake/archive/refs/tags/chocolate-quake-2.1.0.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/Henrique194/chocolate-quake/archive/refs/tags/chocolate-quake-2.1.0.tar.gz",
      sha256 = "a760630778031a0d60ecfd0ba2afb75872f32bffb236a7215c8f513e0449fd76",
    },
  },
  version = "2.1.0",
  license = "GPL-3.0-or-later",
  dependencies = {
    host = { "make", "cmake", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-no-network-or-music-codecs.patch",
    "patches/0002-pyxis-platform.patch",
    "patches/0003-pyxis-frame-sleep.patch",
  },
  outputs = {
    executable = "bin/chocolate-quake.pxe",
    license = "share/licenses/chocolate-quake/LICENSE",
    notice = "share/licenses/chocolate-quake/PORT-NOTICE",
    sdl2_license = "share/licenses/chocolate-quake/SDL2-LICENSE.txt",
  },
}

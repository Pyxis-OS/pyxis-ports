return {
  source = {
    url = "https://github.com/chocolate-doom/chocolate-doom.git",
    commit = "410d96855b5df5410ff591a90efeafa889119224",
    archive = {
      url = "https://github.com/chocolate-doom/chocolate-doom/archive/refs/tags/chocolate-doom-3.1.1.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/chocolate-doom/chocolate-doom/archive/refs/tags/chocolate-doom-3.1.1.tar.gz",
      sha256 = "1edcc41254bdc194beb0d33e267fae306556c4d24110a1d3d3f865717f25da23",
    },
  },
  version = "3.1.1",
  license = "GPL-2.0-or-later",
  dependencies = {
    host = { "make", "cmake", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-sdl2-static-target.patch",
    "patches/0002-pyxis-platform.patch",
    "patches/0003-pyxis-software-scaling.patch",
  },
  outputs = {
    executable = "bin/chocolate-doom.pxe",
    license = "share/licenses/chocolate-doom/COPYING.md",
    notice = "share/licenses/chocolate-doom/PORT-NOTICE",
    sdl2_license = "share/licenses/chocolate-doom/SDL2-LICENSE.txt",
  },
}

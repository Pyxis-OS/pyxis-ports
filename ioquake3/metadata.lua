return {
  source = {
    url = "https://github.com/ioquake/ioq3.git",
    mirror = "https://git.internal/mirrors/ioq3",
    commit = "87a99d18206e878a20289f05ac3c16005dcae5c3",
  },
  version = "1.36-g87a99d1820",
  license = "GPL-2.0-or-later AND Zlib AND BSD-3-Clause AND IJG AND MPL-2.0",
  dependencies = {
    host = { "make", "cmake", "git", "tar", "gzip", "install" },
    pyxis = { "libc", "libpyxis", "libc++", "libc++abi", "libunwind" },
  },
  patches = {
    "patches/0001-pyxis-static-engine.patch",
    "patches/0002-pyxis-native-platform.patch",
  },
  outputs = {
    executable = "bin/ioquake3.pxb/app/bin/ioquake3.pxe",
    manifest = "bin/ioquake3.pxb/manifest.json",
    license = "bin/ioquake3.pxb/app/share/licenses/ioquake3/COPYING.txt",
    notice = "bin/ioquake3.pxb/app/share/licenses/ioquake3/PORT-NOTICE",
    mpl = "bin/ioquake3.pxb/app/share/licenses/ioquake3/MPL-2.0",
    licensing = "bin/ioquake3.pxb/app/share/licenses/ioquake3/LICENSING.md",
    source = "bin/ioquake3.pxb/app/share/sources/ioquake3/ioquake3-source.tar.gz",
    provenance = "bin/ioquake3.pxb/app/share/ioquake3/source.txt",
    sdl_license = "bin/ioquake3.pxb/app/share/licenses/sdl2/LICENSE.txt",
    sdl_backend_source = "bin/ioquake3.pxb/app/share/sources/sdl2/pyxis/SDL_pyxisopengl.c",
    mesa_notices = "bin/ioquake3.pxb/app/share/licenses/mesa/NOTICES",
    cxx_license = "bin/ioquake3.pxb/app/share/licenses/sdk/toolchain/LICENSE.TXT",
  },
}

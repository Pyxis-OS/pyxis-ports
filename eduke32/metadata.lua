return {
  source = {
    url = "https://voidpoint.io/terminx/eduke32.git",
    mirror = "https://git.internal/mirrors/eduke32",
    -- Upstream has no release tags; this is master on 2026-08-07.
    commit = "ec5824db81817866f70da326d3811bb0f52b3517",
  },
  version = "ec5824db",
  license = "GPL-2.0-only AND LicenseRef-BUILDLIC",
  dependencies = {
    host = { "make", "install" },
    pyxis = { "libc", "libpyxis", "libc++" },
  },
  patches = {
    "patches/0001-pyxis-platform.patch",
    "patches/0002-pyxis-thread-limits.patch",
    "patches/0003-pyxis-paths.patch",
    "patches/0004-pyxis-frame-sleep.patch",
  },
  outputs = {
    sdl2_yuv_license = "share/licenses/eduke32/SDL2-yuv2rgb-LICENSE",
    sdl2_libm_notice = "share/licenses/eduke32/SDL2-fdlibm-NOTICE",
    executable = "bin/eduke32.pxb/app/bin/eduke32.pxe",
    manifest = "bin/eduke32.pxb/manifest.json",
    license = "share/licenses/eduke32/gpl-2.0.txt",
    buildlic = "share/licenses/eduke32/buildlic.txt",
    notice = "share/licenses/eduke32/PORT-NOTICE",
    sdl2_license = "share/licenses/eduke32/SDL2-LICENSE.txt",
  },
}

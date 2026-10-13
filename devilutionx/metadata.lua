return {
  source = {
    url = "https://github.com/diasurgical/DevilutionX.git",
    mirror = "https://git.internal/mirrors/DevilutionX",
    commit = "7223eeac9e8274fbf665b4de86fda26d3b22c52f",
    -- The dependencies DevilutionX 1.5.5 itself pins and builds from source.
    extra = {
      { name = "bzip2", url = "https://sourceware.org/git/bzip2.git",
        mirror = "https://git.internal/mirrors/bzip2",
        commit = "6a8690fc8d26c815e798c588f796eabe9d684cf0" },
      { name = "libmpq", url = "https://github.com/diasurgical/libmpq.git",
        mirror = "https://git.internal/mirrors/libmpq",
        commit = "b78d66c6fee6a501cc9b95d8556a129c68841b05" },
      { name = "libsmackerdec", url = "https://github.com/diasurgical/libsmackerdec.git",
        mirror = "https://git.internal/mirrors/libsmackerdec",
        commit = "91e732bb6953489077430572f43fc802bf2c75b2" },
      { name = "simpleini", url = "https://github.com/brofield/simpleini.git",
        mirror = "https://git.internal/mirrors/simpleini",
        commit = "56499b5af5d2195c6acfc58c4630b70e0c9c4c21" },
      { name = "sdl_image", url = "https://github.com/libsdl-org/SDL_image",
        archive = {
          url = "https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-2.0.5.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/libsdl-org/SDL_image/archive/refs/tags/release-2.0.5.tar.gz",
          sha256 = "76b7f67f4c1a5f8368658f0e1e59bdaa4555d1cc7f3a4413178cd735019983ff",
        } },
      -- Upstream pins this archive by MD5 0e8174264ac9c6b314c6b2d9a5f72efd.
      { name = "sdl_audiolib", url = "https://github.com/realnc/SDL_audiolib",
        archive = {
          url = "https://github.com/realnc/SDL_audiolib/archive/cc1bb6af8d4cf5e200259072bde1edd1c8c5137e.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/realnc/SDL_audiolib/archive/cc1bb6af8d4cf5e200259072bde1edd1c8c5137e.tar.gz",
          sha256 = "5adc71bee3506b16c505c1662da73fcfdfd878914645784f80fc70db979533a5",
        },
        patches = { "patches/sdl_audiolib-0001-fmt12.patch" } },
    },
  },
  version = "1.5.5",
  license = "LicenseRef-Sustainable-Use-1.0",
  dependencies = {
    host = { "make", "cmake", "install" },
    pyxis = { "libc", "libpyxis", "libc++" },
  },
  patches = {
    "patches/0001-pyxis-platform.patch",
    "patches/0002-pyxis-file-checks.patch",
    "patches/0003-libcxx-fmt12.patch",
    "patches/0004-pyxis-paths-and-defaults.patch",
    "patches/0005-pyxis-command-modifier-snapshot.patch",
  },
  outputs = {
    executable = "bin/devilutionx.pxb/app/bin/devilutionx.pxe",
    manifest = "bin/devilutionx.pxb/manifest.json",
    assets = "share/devilutionx/assets/ui_art/diablo.pal",
    license = "share/licenses/devilutionx/LICENSE.md",
    notice = "share/licenses/devilutionx/PORT-NOTICE",
    provenance = "share/devilutionx/source.txt",
  },
}

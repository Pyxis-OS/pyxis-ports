return {
  source = {
    url = "https://github.com/libsdl-org/SDL.git",
    mirror = "https://git.internal/mirrors/SDL",
    commit = "5d249570393f7a37e037abf22cd6012a4cc56a71",
  },
  version = "2.32.10",
  license = "Zlib",
  dependencies = {
    host = { "make", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-disable-the-dynamic-api-on-pyxis.patch",
    "patches/0002-register-the-pyxis-video-driver.patch",
    "patches/0003-steam-gamepad-info-without-modification-time.patch",
    "patches/0004-native-pyxis-pointer-authority.patch",
    "patches/0005-blocking-wait-without-thread-wakeup.patch",
    "patches/0006-private-pyxis-clipboard-command-identity.patch",
  },
  outputs = {
    library = "dev/lib/libSDL2.a",
    header = "dev/include/SDL2/SDL.h",
    configuration = "dev/include/SDL2/SDL_config.h",
    package = "dev/lib/cmake/SDL2/SDL2Config.cmake",
    license = "dev/share/licenses/sdl2/LICENSE.txt",
    notice = "dev/share/licenses/sdl2/PORT-NOTICE",
    provenance = "dev/share/sdl2/source.txt",
  },
}

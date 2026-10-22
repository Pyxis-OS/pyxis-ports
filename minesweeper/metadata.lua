return {
  source = {
    url = "https://github.com/AndrewHamel111/raylib-minesweeper",
    commit = "d5f81cc2a700411beca73216c819050223235d5c",
    archive = {
      url = "https://github.com/AndrewHamel111/raylib-minesweeper/archive/d5f81cc2a700411beca73216c819050223235d5c.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/AndrewHamel111/raylib-minesweeper/archive/d5f81cc2a700411beca73216c819050223235d5c.tar.gz",
      sha256 = "346b6d05f67973c243ba2f8a66c2751d96933bf9201576753442d11d743e95ac",
    },
  },
  version = "upstream-d5f81cc",
  license = "MIT",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-pyxis-runtime.patch" },
  outputs = {
    executable = "bin/minesweeper.pxb/app/bin/minesweeper.pxe",
    texture = "bin/minesweeper.pxb/app/resources/flag.png",
    manifest = "bin/minesweeper.pxb/manifest.json",
    license = "share/licenses/minesweeper/LICENSE",
    notice = "share/licenses/minesweeper/PORT-NOTICE",
    provenance = "share/minesweeper/source.txt",
  },
}

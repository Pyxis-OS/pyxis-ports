local games = {
  "arkanoid", "asteroids", "asteroids_survival", "floppy", "gold_fever", "gorilas",
  "missile_commander", "pang", "platformer", "snake", "space_invaders", "tetris",
}
local outputs = {
  license = "share/licenses/raylib-games/LICENSE",
  classics_license = "share/licenses/raylib-games/CLASSICS-LICENSE",
  notice = "share/licenses/raylib-games/PORT-NOTICE",
  provenance = "share/raylib-games/source.txt",
}
for _, game in ipairs(games) do
  outputs[game] = "bin/" .. game .. ".pxb/app/bin/" .. game .. ".pxe"
  outputs[game .. "_manifest"] = "bin/" .. game .. ".pxb/manifest.json"
end

return {
  source = {
    url = "https://github.com/raysan5/raylib-games",
    commit = "318688c759eaf391b36b9f893355c038fcf56419",
    archive = {
      url = "https://github.com/raysan5/raylib-games/archive/318688c759eaf391b36b9f893355c038fcf56419.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/raysan5/raylib-games/archive/318688c759eaf391b36b9f893355c038fcf56419.tar.gz",
      sha256 = "e6d16554997e455aff5767cec951001e7eba07015a87b7928b3d94e71717f020",
    },
  },
  version = "upstream-318688c",
  license = "Zlib",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {},
  games = games,
  outputs = outputs,
}

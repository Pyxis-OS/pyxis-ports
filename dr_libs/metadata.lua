return {
  source = {
    url = "https://github.com/mackron/dr_libs.git",
    commit = "da35f9d6c7374a95353fd1df1d394d44ab66cf01",
    archive = {
      url = "https://github.com/mackron/dr_libs/archive/da35f9d6c7374a95353fd1df1d394d44ab66cf01.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/mackron/dr_libs/archive/da35f9d6c7374a95353fd1df1d394d44ab66cf01.tar.gz",
      sha256 = "2704d347f480ca1bc92233fb01747e4550cc8031735b6ea62ca9990ebb8851ae",
    },
  },
  version = "0.git.da35f9d6c737",
  license = "MIT-0 OR Unlicense",
  dependencies = {
    host = { "curl", "sha256sum", "tar", "gzip", "install", "cp" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    wav_header = "dev/include/dr_wav.h",
    mp3_header = "dev/include/dr_mp3.h",
    flac_header = "dev/include/dr_flac.h",
    pc_dr_libs = "dev/lib/pkgconfig/dr_libs.pc",
    license = "dev/share/licenses/dr_libs/LICENSE",
    wav_license = "dev/share/licenses/dr_libs/dr_wav-LICENSE.txt",
    mp3_license = "dev/share/licenses/dr_libs/dr_mp3-LICENSE.txt",
    flac_license = "dev/share/licenses/dr_libs/dr_flac-LICENSE.txt",
    notice = "dev/share/licenses/dr_libs/PORT-NOTICE",
    pyxis_license = "dev/share/licenses/dr_libs/MPL-2.0",
    licensing = "dev/share/licenses/dr_libs/LICENSING.md",
    provenance = "dev/share/dr_libs/source.txt",
    recipe = "dev/share/dr_libs/pyxis/build.lua",
    recipe_metadata = "dev/share/dr_libs/pyxis/metadata.lua",
  },
}

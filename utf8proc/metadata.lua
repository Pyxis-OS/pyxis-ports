return {
  source = {
    url = "https://github.com/juliastrings/utf8proc",
    archive = {
      url = "https://github.com/juliastrings/utf8proc/archive/v2.11.3.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/juliastrings/utf8proc/archive/v2.11.3.tar.gz",
      sha256 = "abfed50b6d4da51345713661370290f4f4747263ee73dc90356299dfc7990c78",
    },
  },
  version = "2.11.3",
  license = "MIT and Unicode data license",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    library = "dev/lib/libutf8proc.a",
    header = "dev/include/utf8proc.h",
    license = "share/licenses/utf8proc/LICENSE.md",
    provenance = "share/utf8proc/source.txt",
  },
}

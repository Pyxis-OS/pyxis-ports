return {
  source = {
    url = "https://github.com/tree-sitter/tree-sitter",
    archive = {
      url = "https://github.com/tree-sitter/tree-sitter/archive/v0.26.13.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/tree-sitter/tree-sitter/archive/v0.26.13.tar.gz",
      sha256 = "ece24c3c5e2a76384075e830c7139b59fce8fb01e4ef8436fab08bbe10444c89",
    },
  },
  version = "0.26.13",
  license = "MIT AND Unicode-DFS-2016",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install", "git" },
    pyxis = { "libc" },
  },
  patches = { "patches/0001-pyxis-endian.patch" },
  outputs = {
    library = "dev/lib/libtree-sitter.a",
    header = "dev/include/tree_sitter/api.h",
    license = "share/licenses/tree-sitter/LICENSE",
    unicode_license = "share/licenses/tree-sitter/unicode-LICENSE",
    notice = "share/licenses/tree-sitter/PORT-NOTICE",
    provenance = "share/tree-sitter/source.txt",
  },
}

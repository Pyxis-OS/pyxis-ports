return {
  source = {
    url = "https://github.com/sharkdp/hexyl",
    commit = "8eb6d4771ce1ec7af65d06bd335457783b77d557",
    archive = {
      url = "https://github.com/sharkdp/hexyl/archive/refs/tags/v0.17.0.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/sharkdp/hexyl/archive/refs/tags/v0.17.0.tar.gz",
      sha256 = "72fa17397ad187eec6b295d02c7caabbb209a6e0d5706187b8a599bd5df8615e",
    },
    extra = {
      {
        name = "terminal-size",
        url = "https://crates.io/api/v1/crates/terminal_size/0.4.0/download",
        archive = {
          url = "https://crates.io/api/v1/crates/terminal_size/0.4.0/download",
          mirror = "https://repo.internal/repository/cargo-proxy/crates/terminal_size/0.4.0/download",
          sha256 = "4f599bd7ca042cfdf8f4512b277c02ba102247820f9d9d4a9f521f496751a6ef",
        },
        patches = { "patches/0003-terminal-size-native-pyxis.patch" },
      },
    },
  },
  version = "0.17.0",
  license = "(MIT OR Apache-2.0) AND EPL-1.0 AND LicenseRef-CP437",
  dependencies = {
    host = { "python3", "make", "curl", "sha256sum", "tar", "git", "install" },
    pyxis = { "libc", "libpyxis", "Rust std cross-toolchain" },
  },
  patches = {
    "patches/0001-native-seek-and-locked-terminal.patch",
    "patches/0002-propagate-input-and-final-output-errors.patch",
  },
  outputs = {
    executable = "bin/hexyl.pxe",
    license = "share/licenses/hexyl/LICENSE-MIT",
    apache_license = "share/licenses/hexyl/LICENSE-APACHE",
    notice = "share/licenses/hexyl/PORT-NOTICE",
    provenance = "share/hexyl/source.json",
    sources = "share/licenses/hexyl/sources.tar.xz",
    dependencies = "share/licenses/hexyl/dependencies.json",
    rust_receipt = "share/licenses/hexyl/rust/fork-installation.json",
  },
}

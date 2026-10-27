return {
  source = {
    -- Links is published as release archives; there is no public Git history.
    archive = {
      url = "https://links.twibright.com/download/links-2.30.tar.bz2",
      mirror = "https://repo.internal/repository/raw-links/download/links-2.30.tar.bz2",
      sha256 = "c4631c6b5a11527cdc3cb7872fc23b7f2b25c2b021d596be410dadb40315f166",
    },
  },
  -- Each source file says "released under GPL" with no version; COPYING is
  -- GPL version 2.
  license = "GPL",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "bzip2" },
    pyxis = { "libc", "libterm", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-platform.patch",
    "patches/0002-narrow-file-metadata.patch",
    "patches/0003-load-pages-through-libc.patch",
    "patches/0004-save-configuration-under-home.patch",
    "patches/0005-adopt-provider-redirect-snapshots.patch",
  },
  outputs = {
    executable = "bin/links.pxe",
    license = "share/licenses/links/COPYING",
  },
}

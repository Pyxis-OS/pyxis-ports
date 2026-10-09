return {
  source = {
    url = "https://git.busybox.net/busybox",
    mirror = "https://git.internal/mirrors/busybox",
    commit = "f96d33d28a1f70fda5f27d221d5012b1ac0b7dad",
  },
  license = "GPL-2.0-only",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libterm", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-terminal-and-file-access.patch",
    "patches/0002-clear-read-only-file-between-buffers.patch",
    "patches/0003-less-native-console.patch",
    "patches/0004-pyxis-tar-ustar-subset.patch",
    "patches/0005-vi-replace-file-on-save.patch",
  },
  outputs = {
    executable = "bin/vi.pxe",
    less = "bin/less.pxe",
    tar = "bin/tar.pxe",
    license = "share/licenses/busybox/LICENSE",
  },
}

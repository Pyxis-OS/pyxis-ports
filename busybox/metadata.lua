return {
  source = {
    url = "https://github.com/mirror/busybox",
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
  },
  outputs = {
    executable = "bin/vi.pxe",
    license = "share/licenses/busybox/LICENSE",
  },
}

return {
  source = {
    url = "https://git.suckless.org/sbase",
    commit = "c546c3a5724c81cee9a11d816a38ccdf17472129",
  },
  license = "MIT",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-narrow-cksum-helper-header.patch",
  },
  outputs = {
    executable = "bin/cksum.pxe",
    license = "share/licenses/sbase/LICENSE",
    argument_notice = "share/licenses/sbase/arg.h",
  },
}

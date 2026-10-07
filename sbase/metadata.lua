return {
  source = {
    url = "https://git.suckless.org/sbase",
    mirror = "https://git.internal/mirrors/sbase",
    commit = "c546c3a5724c81cee9a11d816a38ccdf17472129",
  },
  license = "MIT AND ISC",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-narrow-cksum-helper-header.patch",
    "patches/0002-restrict-tee-options-and-close-outputs.patch",
    "patches/0003-declare-uniq-helpers.patch",
  },
  outputs = {
    executable = "bin/cksum.pxe",
    tee = "bin/tee.pxe",
    uniq = "bin/uniq.pxe",
    sha256sum = "bin/sha256sum.pxe",
    license = "share/licenses/sbase/LICENSE",
    argument_notice = "share/licenses/sbase/arg.h",
    strtonum_notice = "share/licenses/sbase/strtonum.c",
  },
}

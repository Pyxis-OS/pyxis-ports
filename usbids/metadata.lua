return {
  source = {
    url = "https://github.com/usbids/usbids.git",
    mirror = "https://git.internal/mirrors/usbids",
    commit = "5eb613762db1e34b9218da63a88f22dd26f743c5",
  },
  version = "2026.06.26",
  license = "BSD-3-Clause (project site offers GPL-2.0-or-later or BSD-3-Clause for the database)",
  dependencies = {
    host = { "install" },
    pyxis = {},
  },
  patches = {},
  outputs = {
    database = "share/hwdata/usb.ids",
    provenance = "share/usbids/source.txt",
    license = "share/licenses/usbids/LICENSE",
    notice = "share/licenses/usbids/NOTICE",
  },
}

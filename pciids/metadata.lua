return {
  source = {
    url = "https://github.com/pciutils/pciids.git",
    mirror = "https://git.internal/mirrors/pciids",
    commit = "75ba6ed06a7f85a4e27dbdbf777eec88c0115c4a",
  },
  version = "2026.10.01",
  license = "BSD-3-Clause (upstream offers GPL-2.0-or-later or BSD-3-Clause)",
  dependencies = {
    host = { "install" },
    pyxis = {},
  },
  patches = {},
  outputs = {
    database = "share/hwdata/pci.ids",
    provenance = "share/pciids/source.txt",
    license = "share/licenses/pciids/LICENSE",
    notice = "share/licenses/pciids/NOTICE",
  },
}

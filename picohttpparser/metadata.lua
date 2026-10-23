return {
  source = {
    url = "https://github.com/h2o/picohttpparser.git",
    mirror = "https://git.internal/mirrors/picohttpparser",
    commit = "f4d94b48b31e0abae029ebeafcfd9ca0680ede58",
  },
  license = "MIT",
  dependencies = {
    host = { "make" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    dev_pyxis_license = "dev/share/licenses/picohttpparser/MPL-2.0",
    dev_licensing = "dev/share/licenses/picohttpparser/LICENSING.md",
    pc_picohttpparser = "dev/lib/pkgconfig/picohttpparser.pc",

    library = "dev/lib/libpicohttpparser.a",
    header = "dev/include/picohttpparser.h",
    license = "share/licenses/picohttpparser/picohttpparser.h",
    dev_license = "dev/share/licenses/picohttpparser/picohttpparser.h",
    dev_provenance = "dev/share/picohttpparser/source.txt",
  },
}

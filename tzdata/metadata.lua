return {
  source = {
    url = "https://github.com/eggert/tz.git",
    mirror = "https://git.internal/mirrors/tz",
    commit = "d633fe7ed3de8e00ce7cac991376a064a1373bb1",
  },
  version = "2026d",
  license = "Public domain; see upstream LICENSE for the unused BSD-licensed files",
  dependencies = {
    host = { "make", "cc", "awk" },
    pyxis = {},
  },
  patches = {},
  outputs = {
    utc = "share/zoneinfo/UTC",
    bucharest = "share/zoneinfo/Europe/Bucharest",
    data = "share/zoneinfo/tzdata.zi",
    version = "share/zoneinfo/version",
    license = "share/licenses/tzdata/LICENSE",
    provenance = "share/tzdata/source.txt",
  },
}

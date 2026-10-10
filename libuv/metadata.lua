return {
  source = {
    url = "https://github.com/libuv/libuv.git",
    commit = "1cfa32ff59c076ffb6ed735bbc8c18361558661f",
    archive = {
      url = "https://github.com/libuv/libuv/archive/v1.52.1.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/libuv/libuv/archive/v1.52.1.tar.gz",
      sha256 = "478baf2599bfbc882c355288c9cb6f92e0e7dda435fa04031fa5b607cf3f414c",
    },
  },
  version = "1.52.1",
  license = "MIT",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install", "patch" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-pyxis-platform.patch" },
  outputs = {
    library = "dev/lib/libuv.a",
    header = "dev/include/uv.h",
    errors = "dev/include/uv/errno.h",
    version_header = "dev/include/uv/version.h",
    platform = "dev/include/uv/pyxis.h",
    native = "dev/include/uv/pyxis-native.h",
    work_types = "dev/include/uv/threadpool.h",
    license = "share/licenses/libuv/LICENSE",
    extra_license = "share/licenses/libuv/LICENSE-extra",
    notice = "share/licenses/libuv/PORT-NOTICE",
    provenance = "share/libuv/source.txt",
    sample_manifest = "share/libuv/uv-relay.pxb/manifest.json",
    sample_program = "share/libuv/uv-relay.pxb/app/bin/relay.pxe",
  },
}

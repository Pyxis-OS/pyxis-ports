return {
  source = {
    url = "https://archive.mesa3d.org/glu/glu-9.0.3.tar.xz",
    archive = {
      url = "https://archive.mesa3d.org/glu/glu-9.0.3.tar.xz",
      mirror = "https://repo.internal/repository/raw-mesa/glu/glu-9.0.3.tar.xz",
      sha256 = "bd43fe12f374b1192eb15fe20e45ff456b9bc26ab57f0eee919f96ca0f8a330f",
    },
  },
  version = "9.0.3",
  license = "SGI-B-2.0 AND SGI-B-1.1",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "xz", "install" },
    pyxis = { "libc", "libc++", "libc++abi", "libunwind" },
  },
  patches = {},
  outputs = {
    library = "dev/lib/libGLU.a",
    header = "dev/include/GL/glu.h",
    package = "dev/lib/cmake/GLU/GLUConfig.cmake",
    pc = "dev/lib/pkgconfig/glu.pc",
    license = "dev/share/licenses/glu/SGI-B-2.0",
    nurbs_license = "dev/share/licenses/glu/SGI-B-1.1-NOTICE",
    notice = "dev/share/licenses/glu/PORT-NOTICE",
    pyxis_license = "dev/share/licenses/glu/MPL-2.0",
    licensing = "dev/share/licenses/glu/LICENSING.md",
    provenance = "dev/share/glu/source.txt",
  },
}

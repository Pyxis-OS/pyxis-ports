# GLU

GLU 9.0.3, the Mesa OpenGL Utility Library, built unmodified as a static
library over Pyxis's static OSMesa. The release archive is fetched only from
the owner's `raw-mesa` mirror and checked against its SHA-256.
`dev/share/glu/source.txt` records the archive and profile;
`dev/share/licenses/glu/` keeps the SGI notices and the port notice.

```sh
lua build.lua mesa --sdk /path/to/pyxis/build/sdk
lua build.lua glu --sdk /path/to/pyxis/build/sdk --mesa build/mesa/stage/dev
```

The whole library is built: the utility functions (projection, matrices,
mipmaps, quadrics, errors), the polygon tessellator and the NURBS renderer.

## Using it

Include `<GL/glu.h>` and link `libGLU.a` before Mesa's `libOSMesa.a` and the
C++ runtime. `pkg-config --static glu` gives the closure through `osmesa`.
CMake projects call `find_package(GLU CONFIG)` and link `GLU::GLU`, which
brings OSMesa and the C++ runtime. GLU draws through whichever OSMesa context
is current, for example one created by `sdl2-osmesa` or `sdl3-osmesa`.

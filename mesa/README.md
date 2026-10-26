# Mesa softpipe

Build Mesa 26.2.5 with the restored 25.0.7 OSMesa context/render frontend:

```sh
lua build.lua mesa --sdk /absolute/path/to/pyxis/build/sdk --work /absolute/path/to/mesa-work
```

The host needs Meson 1.4+, Ninja, Python Mako/PyYAML/packaging, flex, bison,
pkg-config and the matching SDK compiler. Only the pinned mirror is downloaded.
`stage/dev` holds the desktop OpenGL headers, static `libOSMesa.a`, SDK-view
pkg-config metadata, original source/header notices and licence texts. Debug
sections are stripped from the staged archive; the build archive retains them.
C clients must link the SDK C++ closure because Mesa includes C++ compiler code.
The release profile explicitly selects O2. Compiler optimization switches from
the SDK are removed from cross-file C/C++ args so Meson owns the level; link
flags and upstream floating-point policy are preserved. Target C is hosted,
with libc/startup implementation exceptions supplied by the SDK.

Copy `stage/manual/mesa-triangle.pxb` into an ordinary writable capability root
and run it with a writable output path. It reports softpipe and writes a
256x256 PPM. It requires declared memory, threads and monotonic clock/sleep,
with no display or JIT grant. This is an opt-in manual program, not an image app.

The Pyxis OSMesa extension accepts `OSMESA_DOUBLEBUFFER` (`0x38`) with
`GL_TRUE` in `OSMesaCreateContextAttribs`. It creates distinct front/back
resources and reports `GL_DOUBLEBUFFER`; drawing to `GL_BACK` works normally.
`OSMesaSwapBuffers(context)` finishes rendering, exchanges those resources and
copies the new front into the caller's `OSMesaMakeCurrent` CPU buffer, honoring
`OSMESA_ROW_LENGTH` and `OSMESA_Y_UP`. Back contents are undefined after swapping.
The context must be current on the calling thread; a failed completion or
readback returns `GL_FALSE` and may already have exchanged the resources.
Default contexts remain single-buffered, and swapping them finishes/copies the
front without exchanging resources. The new function is also available through
`OSMesaGetProcAddress`. Allocation or binding failures in `OSMesaMakeCurrent`
return `GL_FALSE` while preserving the previous current binding.

The profile has no LLVM/JIT, GPU, GLX/EGL/GBM, GLES, shared objects or disk cache.
The optional SSE runtime vertex translator uses the generic C fallback instead;
executable allocation is unavailable. OSMesaPostprocess is absent and its lookup
returns NULL. CPU availability remains the conservative one-CPU fallback;
physical memory is unknown, with no memory-info capability advertised. No Unix
PID/mode/DRM metadata or success is fabricated. Automatic log filenames are
unsupported; an explicit native MESA_LOG_FILE works through normal FILE grants.

The static closure also links with every archive member retained and section
collection disabled, so unused disabled-feature objects do not hide missing
imports. OSMesa source provenance is recorded in metadata and source.txt. Restored files
retain their original notices. Original Pyxis recipe/config/manual sources are
MPL-2.0; patches retain upstream terms. NOTICES is derived from the actual
compiler source/header dependencies, including generated Bison notices and its
exception. This profile is not formally OpenGL-conformance qualified. SDL2
OpenGL glue and game integration are later tasks in the parent milestone.

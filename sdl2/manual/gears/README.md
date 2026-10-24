# SDL OpenGL gears

An asset-free desktop GL program using SDL's default double-buffer request.
Build against the opt-in SDL/OSMesa profile and copy the executable plus manifest
into a normal bundle directory (`manifest.json`, `app/bin/sdl-gears.pxe`):

```sh
make -f sdl2/manual/gears/Makefile SDK=/absolute/path/to/sdk \
  SDL2_STAGE=/absolute/path/to/sdl2/stage MESA_STAGE=/absolute/path/to/mesa/stage/dev
```

Drag or use arrow keys to rotate; Space pauses; C changes the logical canvas;
H hides the help line; Escape exits. Moving to another space releases input
while animation continues. The renderer name is drawn in the frame and included
in the SDL window title. No data assets, JIT or audio authority is needed.

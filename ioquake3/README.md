# ioquake3 on Pyxis

The pinned client uses the SDL2 OSMesa link profile, Mesa softpipe, one static
opengl1 renderer and interpreted QVMs. It has no JIT or external networking.
The public OpenArena package supplies `pkg://openarena/baseoa`; preferences and
saves use `home://ioquake3`. Retail Quake 3 data is not included.

Build with `make game-packages GAMES=openarena REPOSITORY=/path/to/repository`
from the parent checkout. Required bundle grants cover memory, threads, precise
clock, display and keyboard. Pointer and audio are optional. Software drawing
starts at a 320x240 logical canvas; change the ordinary `r_mode` setting for a
larger canvas. This build uses the real double-buffer GL path.

Publication is pending preferred asset/QVM source delivery and the complete
bundled-code licence notices; the runtime archive alone is not a source release.

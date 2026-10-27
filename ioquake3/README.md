# ioquake3 on Pyxis

The pinned client uses the SDL2 OSMesa link profile, one static opengl1 renderer
and interpreted QVMs. Mesa selects llvmpipe when the optional JIT grant is held,
otherwise softpipe. QVMs remain interpreted; external networking is unavailable.
The public OpenArena package supplies `pkg://openarena/baseoa`; preferences and
saves use `home://ioquake3`. Retail Quake 3 data is not included.

Build with `make game-packages GAMES=openarena REPOSITORY=/path/to/repository`
from the parent checkout. Required bundle grants cover memory, threads, precise
clock, display and keyboard. Pointer, audio and JIT are optional. The space must opt into JIT for llvmpipe;
an explicitly forced llvmpipe without that authority refuses. Software drawing uses upstream desktop-resolution selection and its 640x480
fallback; the ordinary `r_mode` setting can select a different canvas. This build uses the real double-buffer GL path.

Supplementary Ogg, Vorbis and JPEG notices are included. OpenArena preferred
asset/QVM sources are separate public downloads; the parent publication target
requires their checked source set before packing the binary.

# raylib-games classics

Twelve upstream classics from raysan5/raylib-games, zlib licensed. The exact
commit, owner cache URL and archive SHA-256 are in metadata.lua. The source
files are unchanged; only the static SDK build and bundle manifests are local.
Build after raylib and SDL2, using their explicit development prefixes:

```sh
lua ports/build.lua raylib-games --sdk build/sdk --work build/ports/raylib-games \
  --raylib build/ports/raylib/stage/dev --sdl2 build/ports/sdl2/stage/dev
```

The recipe stages one `bin/NAME.pxb` bundle per game, with executable
`app/bin/NAME.pxe` and the matching command name. No external assets, writable
paths, network or audio are used. Every bundle requests memory, precise sleeping
clock, display, keyboard and gamepad input; raylib's SDL startup needs gamepad
authority even though these games do not use controllers. Gorilas and Missile
Commander additionally require pointer input.

| Command | Controls |
| --- | --- |
| `arkanoid` | Left/Right move the paddle; Space launches the ball. |
| `asteroids` | Left/Right rotate; Up accelerates; Down brakes; Space fires. |
| `asteroids_survival` | Left/Right rotate; Up accelerates; Down brakes; avoid asteroids. |
| `floppy` | Hold Space to rise; release to fall. |
| `gold_fever` | Arrow keys move and collect gold. |
| `gorilas` | Aim above the active player toward the opposite team; left click throws. |
| `missile_commander` | Point at the target; left/middle/right click fire launchers 1/2/3. |
| `pang` | Left/Right move; Space fires the vertical shot. |
| `platformer` | A/D or Left/Right move; Space jumps; collect coins. |
| `snake` | Arrow keys turn. |
| `space_invaders` | Arrow keys move; hold Space to fire. |
| `tetris` | Left/Right move; Up rotates; Down drops faster. |

Escape exits all games. Enter restarts after game over or a completed level;
P pauses every game except platformer. The original logical canvas is 800x450
except platformer's 640x384; upstream caps each game at 60 FPS. Raylib scales
the canvas into the native display area. These controls and dimensions come
from source review; build success alone does not qualify gameplay, pointer
mapping, visuals or timing. Runtime qualification belongs to parent integration.

Notices and provenance stage under `share/licenses/raylib-games` and
`share/raylib-games`. Public packages also need the raylib, SDL2 and SDK runtime
license closure. Source screenshots and unrelated game collections are omitted.

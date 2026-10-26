#ifndef SDL_pyxisjoystick_h_
#define SDL_pyxisjoystick_h_

#include <abi/handle.h>

/* The delegated handle is retained until process exit. The interest remains
 * safe if joystick shutdown releases acquisition while video is waiting. */
handle_t PYXIS_GamepadWaitHandle(void);

#endif

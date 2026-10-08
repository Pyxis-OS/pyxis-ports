/* SDL ticks and delays from the Pyxis monotonic clock. Sleeps end on the
 * kernel's preemption tick, so a delay can run up to 8.33 ms long. */

#include "SDL_internal.h"

#ifdef SDL_TIMER_PYXIS

#include <clock.h>
#include <startup.h>

#include "SDL_timer.h"

#define NANOSECONDS_PER_MILLISECOND UINT64_C(1000000)
#define NANOSECONDS_PER_SECOND UINT64_C(1000000000)

static handle_t clock_handle = HANDLE_INVALID;
static uint64_t start_ns;
static SDL_bool ticks_started = SDL_FALSE;

/* Without a clock grant time stands still at zero; the video driver refuses
 * to start in that case, so this only affects programs without video. */
static uint64_t now_ns(void)
{
  uint64_t now = 0;
  if (clock_handle != HANDLE_INVALID) {
    clock_now(clock_handle, &now);
  }
  return now;
}

void SDL_TicksInit(void)
{
  if (ticks_started) {
    return;
  }
  ticks_started = SDL_TRUE;
  clock_handle = startup_resource("clock");
  start_ns = now_ns();
}

void SDL_TicksQuit(void)
{
  ticks_started = SDL_FALSE;
}

Uint64 SDL_GetTicks64(void)
{
  if (!ticks_started) {
    SDL_TicksInit();
  }
  return (now_ns() - start_ns) / NANOSECONDS_PER_MILLISECOND;
}

Uint64 SDL_GetPerformanceCounter(void)
{
  if (!ticks_started) {
    SDL_TicksInit();
  }
  return now_ns();
}

Uint64 SDL_GetPerformanceFrequency(void)
{
  return NANOSECONDS_PER_SECOND;
}

void SDL_Delay(Uint32 ms)
{
  if (!ticks_started) {
    SDL_TicksInit();
  }
  if (clock_handle != HANDLE_INVALID) {
    clock_sleep_for(clock_handle, (uint64_t)ms * NANOSECONDS_PER_MILLISECOND);
  }
}

#endif /* SDL_TIMER_PYXIS */

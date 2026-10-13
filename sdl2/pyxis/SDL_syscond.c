/* SDL condition variables over libc conditions on the monotonic clock, so
 * timed waits (and SDL's generic semaphores built on them) are unaffected by
 * wall-clock changes. Timed waits need the clock grant libc retains. */

#include "SDL_internal.h"

#ifdef SDL_THREAD_PTHREAD

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "SDL_thread.h"
#include "thread/pthread/SDL_sysmutex_c.h"

#define NANOSECONDS_PER_MILLISECOND 1000000L
#define NANOSECONDS_PER_SECOND 1000000000L
#define MILLISECONDS_PER_SECOND 1000

struct SDL_cond {
  pthread_cond_t cond;
};

SDL_cond *SDL_CreateCond(void)
{
  SDL_cond *cond = SDL_malloc(sizeof(*cond));
  if (!cond) {
    SDL_OutOfMemory();
    return NULL;
  }
  pthread_condattr_t attributes;
  int error = pthread_condattr_init(&attributes);
  if (error == 0) {
    error = pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    if (error == 0) {
      error = pthread_cond_init(&cond->cond, &attributes);
    }
    pthread_condattr_destroy(&attributes);
  }
  if (error != 0) {
    SDL_SetError("pthread_cond_init() failed: %s", strerror(error));
    SDL_free(cond);
    return NULL;
  }
  return cond;
}

void SDL_DestroyCond(SDL_cond *cond)
{
  if (cond) {
    pthread_cond_destroy(&cond->cond);
    SDL_free(cond);
  }
}

int SDL_CondSignal(SDL_cond *cond)
{
  if (!cond) {
    return SDL_InvalidParamError("cond");
  }
  int error = pthread_cond_signal(&cond->cond);
  if (error != 0) {
    return SDL_SetError("pthread_cond_signal() failed: %s", strerror(error));
  }
  return 0;
}

int SDL_CondBroadcast(SDL_cond *cond)
{
  if (!cond) {
    return SDL_InvalidParamError("cond");
  }
  int error = pthread_cond_broadcast(&cond->cond);
  if (error != 0) {
    return SDL_SetError("pthread_cond_broadcast() failed: %s", strerror(error));
  }
  return 0;
}

int SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex)
{
  if (!cond) {
    return SDL_InvalidParamError("cond");
  }
  if (!mutex) {
    return SDL_InvalidParamError("mutex");
  }
  int error = pthread_cond_wait(&cond->cond, &mutex->id);
  if (error != 0) {
    return SDL_SetError("pthread_cond_wait() failed: %s", strerror(error));
  }
  return 0;
}

/* SDL_MUTEX_MAXWAIT waits indefinitely, as SDL's generic semaphores expect. */
int SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, Uint32 ms)
{
  if (ms == SDL_MUTEX_MAXWAIT) {
    return SDL_CondWait(cond, mutex);
  }
  if (!cond) {
    return SDL_InvalidParamError("cond");
  }
  if (!mutex) {
    return SDL_InvalidParamError("mutex");
  }

  struct timespec deadline;
  if (clock_gettime(CLOCK_MONOTONIC, &deadline) != 0) {
    return SDL_SetError("clock_gettime() failed: %s", strerror(errno));
  }
  /* A Uint32 millisecond count stays far below time_t's range. */
  deadline.tv_sec += (time_t)(ms / MILLISECONDS_PER_SECOND);
  deadline.tv_nsec += (long)(ms % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND;
  if (deadline.tv_nsec >= NANOSECONDS_PER_SECOND) {
    deadline.tv_sec += 1;
    deadline.tv_nsec -= NANOSECONDS_PER_SECOND;
  }

  int error = pthread_cond_timedwait(&cond->cond, &mutex->id, &deadline);
  if (error == ETIMEDOUT) {
    return SDL_MUTEX_TIMEDOUT;
  }
  if (error != 0) {
    return SDL_SetError("pthread_cond_timedwait() failed: %s", strerror(error));
  }
  return 0;
}

#endif /* SDL_THREAD_PTHREAD */

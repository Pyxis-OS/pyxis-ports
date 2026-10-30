/* SDL threads over libc's pthread profile. Creation needs the program's
 * declared threads grant; libc refuses without it, and SDL reports that
 * refusal. Names stay in SDL; Pyxis threads have no native name. */

#include "SDL_internal.h"

#ifdef SDL_THREAD_PTHREAD

#include <abi/memory.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "thread/SDL_systhread.h"
#include "thread/SDL_thread_c.h"

/* libc's current smallest accepted stack. SDL sizes are minimums, so smaller
 * requests receive this; libc refuses sizes above its maximum (8 MiB). */
#define PYXIS_THREAD_STACK_MIN ((size_t)1 << 20)

static void *run_thread(void *data)
{
  SDL_RunThread((SDL_Thread *)data);
  return NULL;
}

static size_t stack_size(size_t requested)
{
  if (requested < PYXIS_THREAD_STACK_MIN) {
    return PYXIS_THREAD_STACK_MIN;
  }
  size_t page_offset = requested % MEMORY_PAGE_SIZE;
  if (page_offset == 0) {
    return requested;
  }
  size_t padding = MEMORY_PAGE_SIZE - page_offset;
  return requested > SIZE_MAX - padding ? SIZE_MAX : requested + padding;
}

bool SDL_SYS_CreateThread(SDL_Thread *thread, SDL_FunctionPointer begin, SDL_FunctionPointer end)
{
  (void)begin;
  (void)end;
  pthread_attr_t attributes;
  int error = pthread_attr_init(&attributes);
  if (error != 0) {
    return SDL_SetError("pthread_attr_init() failed: %s", strerror(error));
  }
  if (thread->stacksize != 0) {
    size_t size = stack_size(thread->stacksize);
    error = pthread_attr_setstacksize(&attributes, size);
    if (error != 0) {
      pthread_attr_destroy(&attributes);
      return SDL_SetError("Thread stack size %zu is outside libc's limits", size);
    }
  }
  error = pthread_create(&thread->handle, &attributes, run_thread, thread);
  pthread_attr_destroy(&attributes);
  if (error != 0) {
    return SDL_SetError("pthread_create() failed: %s", strerror(error));
  }
  return true;
}

void SDL_SYS_SetupThread(const char *name)
{
  (void)name;
}

SDL_ThreadID SDL_GetCurrentThreadID(void)
{
  return (SDL_ThreadID)pthread_self();
}

/* Pyxis has no thread priorities; only the one every thread runs at succeeds. */
bool SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
  if (priority != SDL_THREAD_PRIORITY_NORMAL) {
    return SDL_Unsupported();
  }
  return true;
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
  pthread_join(thread->handle, NULL);
}

void SDL_SYS_DetachThread(SDL_Thread *thread)
{
  pthread_detach(thread->handle);
}

#endif /* SDL_THREAD_PTHREAD */

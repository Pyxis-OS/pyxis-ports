#include "internal.h"
#include <limits.h>
#include <stdlib.h>

/* These objects describe the sole executing task. They are not synchronization
 * between tasks, and must be replaced before shared-process threads are enabled. */
int uv_mutex_init(uv_mutex_t *mutex)
{
  if (!mutex) {
    return UV_EINVAL;
  }
  *mutex = (uv_mutex_t){0};
  return 0;
}

int uv_mutex_init_recursive(uv_mutex_t *mutex)
{
  int result = uv_mutex_init(mutex);
  if (!result) {
    mutex->recursive = 1;
  }
  return result;
}

void uv_mutex_destroy(uv_mutex_t *mutex)
{
  if (!mutex || mutex->depth) {
    abort();
  }
  *mutex = (uv_mutex_t){0};
}

int uv_mutex_trylock(uv_mutex_t *mutex)
{
  if (!mutex || mutex->depth == UINT_MAX) {
    return UV_EINVAL;
  }
  if (mutex->depth && !mutex->recursive) {
    return UV_EBUSY;
  }
  ++mutex->depth;
  return 0;
}

void uv_mutex_lock(uv_mutex_t *mutex)
{
  /* Blocking on oneself cannot make progress on the one-thread platform. */
  if (uv_mutex_trylock(mutex)) {
    abort();
  }
}

void uv_mutex_unlock(uv_mutex_t *mutex)
{
  if (!mutex || !mutex->depth) {
    abort();
  }
  --mutex->depth;
}

void uv_once(uv_once_t *guard, void (*callback)(void))
{
  if (!guard || !callback || guard->state == 1) {
    abort();
  }
  if (!guard->state) {
    guard->state = 1;
    callback();
    guard->state = 2;
  }
}

int uv_key_create(uv_key_t *key)
{
  if (!key) {
    return UV_EINVAL;
  }
  *key = (uv_key_t){.initialized = 1};
  return 0;
}

void uv_key_delete(uv_key_t *key)
{
  if (!key || !key->initialized) {
    abort();
  }
  *key = (uv_key_t){0};
}

void *uv_key_get(uv_key_t *key)
{
  if (!key || !key->initialized) {
    abort();
  }
  return key->value;
}

void uv_key_set(uv_key_t *key, void *value)
{
  if (!key || !key->initialized) {
    abort();
  }
  key->value = value;
}

uv_thread_t uv_thread_self(void)
{
  return 1;
}

int uv_thread_equal(const uv_thread_t *first, const uv_thread_t *second)
{
  return first && second && *first == *second;
}

int uv_thread_create(uv_thread_t *thread, uv_thread_cb entry, void *argument)
{
  (void)thread;
  (void)entry;
  (void)argument;
  return UV_ENOSYS;
}

int uv_thread_create_ex(uv_thread_t *thread, const uv_thread_options_t *options,
    uv_thread_cb entry, void *argument)
{
  (void)options;
  return uv_thread_create(thread, entry, argument);
}

int uv_thread_join(uv_thread_t *thread)
{
  (void)thread;
  return UV_ENOSYS;
}

int uv_thread_detach(uv_thread_t *thread)
{
  (void)thread;
  return UV_ENOSYS;
}

int uv_thread_setaffinity(uv_thread_t *thread, char *mask, char *old_mask, size_t size)
{
  (void)thread;
  (void)mask;
  (void)old_mask;
  (void)size;
  return UV_ENOSYS;
}

int uv_thread_getaffinity(uv_thread_t *thread, char *mask, size_t size)
{
  (void)thread;
  (void)mask;
  (void)size;
  return UV_ENOSYS;
}

int uv_thread_getcpu(void)
{
  return UV_ENOSYS;
}

int uv_thread_setname(const char *name)
{
  (void)name;
  return UV_ENOSYS;
}

int uv_thread_getname(uv_thread_t *thread, char *name, size_t size)
{
  (void)thread;
  (void)name;
  (void)size;
  return UV_ENOSYS;
}

int uv_thread_getpriority(uv_thread_t thread, int *priority)
{
  (void)thread;
  (void)priority;
  return UV_ENOSYS;
}

int uv_thread_setpriority(uv_thread_t thread, int priority)
{
  (void)thread;
  (void)priority;
  return UV_ENOSYS;
}

/* Initializers reject unsupported primitives. A void operation on such an
 * uninitialized object diagnoses misuse rather than pretending to synchronize. */
int uv_rwlock_init(uv_rwlock_t *lock)
{
  (void)lock;
  return UV_ENOSYS;
}
void uv_rwlock_destroy(uv_rwlock_t *lock)
{
  (void)lock;
  abort();
}
void uv_rwlock_rdlock(uv_rwlock_t *lock)
{
  (void)lock;
  abort();
}
int uv_rwlock_tryrdlock(uv_rwlock_t *lock)
{
  (void)lock;
  return UV_ENOSYS;
}
void uv_rwlock_rdunlock(uv_rwlock_t *lock)
{
  (void)lock;
  abort();
}
void uv_rwlock_wrlock(uv_rwlock_t *lock)
{
  (void)lock;
  abort();
}
int uv_rwlock_trywrlock(uv_rwlock_t *lock)
{
  (void)lock;
  return UV_ENOSYS;
}
void uv_rwlock_wrunlock(uv_rwlock_t *lock)
{
  (void)lock;
  abort();
}
int uv_sem_init(uv_sem_t *sem, unsigned int value)
{
  (void)sem;
  (void)value;
  return UV_ENOSYS;
}
void uv_sem_destroy(uv_sem_t *sem)
{
  (void)sem;
  abort();
}
void uv_sem_post(uv_sem_t *sem)
{
  (void)sem;
  abort();
}
void uv_sem_wait(uv_sem_t *sem)
{
  (void)sem;
  abort();
}
int uv_sem_trywait(uv_sem_t *sem)
{
  (void)sem;
  return UV_ENOSYS;
}
int uv_cond_init(uv_cond_t *cond)
{
  (void)cond;
  return UV_ENOSYS;
}
void uv_cond_destroy(uv_cond_t *cond)
{
  (void)cond;
  abort();
}
void uv_cond_signal(uv_cond_t *cond)
{
  (void)cond;
  abort();
}
void uv_cond_broadcast(uv_cond_t *cond)
{
  (void)cond;
  abort();
}
void uv_cond_wait(uv_cond_t *cond, uv_mutex_t *mutex)
{
  (void)cond;
  (void)mutex;
  abort();
}
int uv_cond_timedwait(uv_cond_t *cond, uv_mutex_t *mutex, uint64_t timeout)
{
  (void)cond;
  (void)mutex;
  (void)timeout;
  return UV_ENOSYS;
}
int uv_barrier_init(uv_barrier_t *barrier, unsigned int count)
{
  (void)barrier;
  (void)count;
  return UV_ENOSYS;
}
void uv_barrier_destroy(uv_barrier_t *barrier)
{
  (void)barrier;
  abort();
}
int uv_barrier_wait(uv_barrier_t *barrier)
{
  (void)barrier;
  return UV_ENOSYS;
}

#include "internal.h"

/* These entry points reject before allocating handles or submitting work. */

int uv_loop_fork(uv_loop_t* loop)
{
  (void)loop;
  return UV_ENOSYS;
}

int uv_send_buffer_size(uv_handle_t* handle, int* value)
{
  (void)handle;
  (void)value;
  return UV_ENOSYS;
}

int uv_recv_buffer_size(uv_handle_t* handle, int* value)
{
  (void)handle;
  (void)value;
  return UV_ENOSYS;
}

int uv_pipe(uv_file fds[2], int read_flags, int write_flags)
{
  (void)fds;
  (void)read_flags;
  (void)write_flags;
  return UV_ENOSYS;
}

int uv_socketpair(int type, int protocol, uv_os_sock_t socket_vector[2], int flags0, int flags1)
{
  (void)type;
  (void)protocol;
  (void)socket_vector;
  (void)flags0;
  (void)flags1;
  return UV_ENOSYS;
}

int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
{
  (void)stream;
  (void)backlog;
  (void)cb;
  return UV_ENOSYS;
}

int uv_accept(uv_stream_t* server, uv_stream_t* client)
{
  (void)server;
  (void)client;
  return UV_ENOSYS;
}

int uv_tcp_init(uv_loop_t* argument_0, uv_tcp_t* handle)
{
  (void)argument_0;
  (void)handle;
  return UV_ENOSYS;
}

int uv_tcp_init_ex(uv_loop_t* argument_0, uv_tcp_t* handle, unsigned int flags)
{
  (void)argument_0;
  (void)handle;
  (void)flags;
  return UV_ENOSYS;
}

int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock)
{
  (void)handle;
  (void)sock;
  return UV_ENOSYS;
}

int uv_tcp_nodelay(uv_tcp_t* handle, int enable)
{
  (void)handle;
  (void)enable;
  return UV_ENOSYS;
}

int uv_tcp_keepalive(uv_tcp_t* handle, int enable, unsigned int delay)
{
  (void)handle;
  (void)enable;
  (void)delay;
  return UV_ENOSYS;
}

int uv_tcp_keepalive_ex(uv_tcp_t* handle, int on, unsigned int idle, unsigned int intvl, unsigned int cnt)
{
  (void)handle;
  (void)on;
  (void)idle;
  (void)intvl;
  (void)cnt;
  return UV_ENOSYS;
}

int uv_tcp_simultaneous_accepts(uv_tcp_t* handle, int enable)
{
  (void)handle;
  (void)enable;
  return UV_ENOSYS;
}

int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
{
  (void)handle;
  (void)addr;
  (void)flags;
  return UV_ENOSYS;
}

int uv_tcp_getsockname(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
{
  (void)handle;
  (void)name;
  (void)namelen;
  return UV_ENOSYS;
}

int uv_tcp_getpeername(const uv_tcp_t* handle, struct sockaddr* name, int* namelen)
{
  (void)handle;
  (void)name;
  (void)namelen;
  return UV_ENOSYS;
}

int uv_tcp_close_reset(uv_tcp_t* handle, uv_close_cb close_cb)
{
  (void)handle;
  (void)close_cb;
  return UV_ENOSYS;
}

int uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct sockaddr* addr, uv_connect_cb cb)
{
  (void)req;
  (void)handle;
  (void)addr;
  (void)cb;
  return UV_ENOSYS;
}

int uv_udp_init(uv_loop_t* argument_0, uv_udp_t* handle)
{
  (void)argument_0;
  (void)handle;
  return UV_ENOSYS;
}

int uv_udp_init_ex(uv_loop_t* argument_0, uv_udp_t* handle, unsigned int flags)
{
  (void)argument_0;
  (void)handle;
  (void)flags;
  return UV_ENOSYS;
}

int uv_udp_open(uv_udp_t* handle, uv_os_sock_t sock)
{
  (void)handle;
  (void)sock;
  return UV_ENOSYS;
}

int uv_udp_open_ex(uv_udp_t* handle, uv_os_sock_t sock, unsigned int flags)
{
  (void)handle;
  (void)sock;
  (void)flags;
  return UV_ENOSYS;
}

int uv_udp_bind(uv_udp_t* handle, const struct sockaddr* addr, unsigned int flags)
{
  (void)handle;
  (void)addr;
  (void)flags;
  return UV_ENOSYS;
}

int uv_udp_connect(uv_udp_t* handle, const struct sockaddr* addr)
{
  (void)handle;
  (void)addr;
  return UV_ENOSYS;
}

int uv_udp_getpeername(const uv_udp_t* handle, struct sockaddr* name, int* namelen)
{
  (void)handle;
  (void)name;
  (void)namelen;
  return UV_ENOSYS;
}

int uv_udp_getsockname(const uv_udp_t* handle, struct sockaddr* name, int* namelen)
{
  (void)handle;
  (void)name;
  (void)namelen;
  return UV_ENOSYS;
}

int uv_udp_set_membership(uv_udp_t* handle, const char* multicast_addr, const char* interface_addr, uv_membership membership)
{
  (void)handle;
  (void)multicast_addr;
  (void)interface_addr;
  (void)membership;
  return UV_ENOSYS;
}

int uv_udp_set_source_membership(uv_udp_t* handle, const char* multicast_addr, const char* interface_addr, const char* source_addr, uv_membership membership)
{
  (void)handle;
  (void)multicast_addr;
  (void)interface_addr;
  (void)source_addr;
  (void)membership;
  return UV_ENOSYS;
}

int uv_udp_set_multicast_loop(uv_udp_t* handle, int on)
{
  (void)handle;
  (void)on;
  return UV_ENOSYS;
}

int uv_udp_set_multicast_ttl(uv_udp_t* handle, int ttl)
{
  (void)handle;
  (void)ttl;
  return UV_ENOSYS;
}

int uv_udp_set_multicast_interface(uv_udp_t* handle, const char* interface_addr)
{
  (void)handle;
  (void)interface_addr;
  return UV_ENOSYS;
}

int uv_udp_set_broadcast(uv_udp_t* handle, int on)
{
  (void)handle;
  (void)on;
  return UV_ENOSYS;
}

int uv_udp_set_ttl(uv_udp_t* handle, int ttl)
{
  (void)handle;
  (void)ttl;
  return UV_ENOSYS;
}

int uv_udp_send(uv_udp_send_t* req, uv_udp_t* handle, const uv_buf_t bufs[], unsigned int nbufs, const struct sockaddr* addr, uv_udp_send_cb send_cb)
{
  (void)req;
  (void)handle;
  (void)bufs;
  (void)nbufs;
  (void)addr;
  (void)send_cb;
  return UV_ENOSYS;
}

int uv_udp_try_send(uv_udp_t* handle, const uv_buf_t bufs[], unsigned int nbufs, const struct sockaddr* addr)
{
  (void)handle;
  (void)bufs;
  (void)nbufs;
  (void)addr;
  return UV_ENOSYS;
}

int uv_udp_try_send2(uv_udp_t* handle, unsigned int count, uv_buf_t* bufs[], unsigned int nbufs[], struct sockaddr* addrs[], unsigned int flags)
{
  (void)handle;
  (void)count;
  (void)bufs;
  (void)nbufs;
  (void)addrs;
  (void)flags;
  return UV_ENOSYS;
}

int uv_udp_recv_start(uv_udp_t* handle, uv_alloc_cb alloc_cb, uv_udp_recv_cb recv_cb)
{
  (void)handle;
  (void)alloc_cb;
  (void)recv_cb;
  return UV_ENOSYS;
}

int uv_udp_using_recvmmsg(const uv_udp_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_udp_recv_stop(uv_udp_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_tty_set_mode(uv_tty_t* argument_0, uv_tty_mode_t mode)
{
  (void)argument_0;
  (void)mode;
  return UV_ENOSYS;
}

int uv_tty_reset_mode()
{
  return UV_ENOSYS;
}

int uv_tty_get_vterm_state(uv_tty_vtermstate_t* state)
{
  (void)state;
  return UV_ENOSYS;
}

int uv_pipe_bind(uv_pipe_t* handle, const char* name)
{
  (void)handle;
  (void)name;
  return UV_ENOSYS;
}

int uv_pipe_bind2(uv_pipe_t* handle, const char* name, size_t namelen, unsigned int flags)
{
  (void)handle;
  (void)name;
  (void)namelen;
  (void)flags;
  return UV_ENOSYS;
}

int uv_pipe_connect2(uv_connect_t* req, uv_pipe_t* handle, const char* name, size_t namelen, unsigned int flags, uv_connect_cb cb)
{
  (void)req;
  (void)handle;
  (void)name;
  (void)namelen;
  (void)flags;
  (void)cb;
  return UV_ENOSYS;
}

int uv_pipe_getsockname(const uv_pipe_t* handle, char* buffer, size_t* size)
{
  (void)handle;
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_pipe_getpeername(const uv_pipe_t* handle, char* buffer, size_t* size)
{
  (void)handle;
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_pipe_pending_count(uv_pipe_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_pipe_chmod(uv_pipe_t* handle, int flags)
{
  (void)handle;
  (void)flags;
  return UV_ENOSYS;
}

int uv_poll_init(uv_loop_t* loop, uv_poll_t* handle, int fd)
{
  (void)loop;
  (void)handle;
  (void)fd;
  return UV_ENOSYS;
}

int uv_poll_init_socket(uv_loop_t* loop, uv_poll_t* handle, uv_os_sock_t socket)
{
  (void)loop;
  (void)handle;
  (void)socket;
  return UV_ENOSYS;
}

int uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb)
{
  (void)handle;
  (void)events;
  (void)cb;
  return UV_ENOSYS;
}

int uv_poll_stop(uv_poll_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_timer_init(uv_loop_t* argument_0, uv_timer_t* handle)
{
  (void)argument_0;
  (void)handle;
  return UV_ENOSYS;
}

int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat)
{
  (void)handle;
  (void)cb;
  (void)timeout;
  (void)repeat;
  return UV_ENOSYS;
}

int uv_timer_stop(uv_timer_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_timer_again(uv_timer_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_getaddrinfo(uv_loop_t* loop, uv_getaddrinfo_t* req, uv_getaddrinfo_cb getaddrinfo_cb, const char* node, const char* service, const struct addrinfo* hints)
{
  (void)loop;
  (void)req;
  (void)getaddrinfo_cb;
  (void)node;
  (void)service;
  (void)hints;
  return UV_ENOSYS;
}

int uv_getnameinfo(uv_loop_t* loop, uv_getnameinfo_t* req, uv_getnameinfo_cb getnameinfo_cb, const struct sockaddr* addr, int flags)
{
  (void)loop;
  (void)req;
  (void)getnameinfo_cb;
  (void)addr;
  (void)flags;
  return UV_ENOSYS;
}

int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb)
{
  (void)loop;
  (void)req;
  (void)work_cb;
  (void)after_work_cb;
  return UV_ENOSYS;
}

int uv_cancel(uv_req_t* req)
{
  (void)req;
  return UV_ENOSYS;
}

int uv_get_process_title(char* buffer, size_t size)
{
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_set_process_title(const char* title)
{
  (void)title;
  return UV_ENOSYS;
}

int uv_resident_set_memory(size_t* rss)
{
  (void)rss;
  return UV_ENOSYS;
}

int uv_uptime(double* uptime)
{
  (void)uptime;
  return UV_ENOSYS;
}

int uv_open_osfhandle(uv_os_fd_t os_fd)
{
  (void)os_fd;
  return UV_ENOSYS;
}

int uv_getrusage(uv_rusage_t* rusage)
{
  (void)rusage;
  return UV_ENOSYS;
}

int uv_getrusage_thread(uv_rusage_t* rusage)
{
  (void)rusage;
  return UV_ENOSYS;
}

int uv_os_homedir(char* buffer, size_t* size)
{
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_os_tmpdir(char* buffer, size_t* size)
{
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_os_get_passwd(uv_passwd_t* pwd)
{
  (void)pwd;
  return UV_ENOSYS;
}

int uv_os_get_passwd2(uv_passwd_t* pwd, uv_uid_t uid)
{
  (void)pwd;
  (void)uid;
  return UV_ENOSYS;
}

int uv_os_get_group(uv_group_t* grp, uv_uid_t gid)
{
  (void)grp;
  (void)gid;
  return UV_ENOSYS;
}

int uv_os_getpriority(uv_pid_t pid, int* priority)
{
  (void)pid;
  (void)priority;
  return UV_ENOSYS;
}

int uv_os_setpriority(uv_pid_t pid, int priority)
{
  (void)pid;
  (void)priority;
  return UV_ENOSYS;
}

int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count)
{
  (void)cpu_infos;
  (void)count;
  return UV_ENOSYS;
}

int uv_cpumask_size()
{
  return UV_ENOSYS;
}

int uv_interface_addresses(uv_interface_address_t** addresses, int* count)
{
  (void)addresses;
  (void)count;
  return UV_ENOSYS;
}

int uv_os_environ(uv_env_item_t** envitems, int* count)
{
  (void)envitems;
  (void)count;
  return UV_ENOSYS;
}

int uv_os_setenv(const char* name, const char* value)
{
  (void)name;
  (void)value;
  return UV_ENOSYS;
}

int uv_os_unsetenv(const char* name)
{
  (void)name;
  return UV_ENOSYS;
}

int uv_os_gethostname(char* buffer, size_t* size)
{
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_os_uname(uv_utsname_t* buffer)
{
  (void)buffer;
  return UV_ENOSYS;
}

int uv_metrics_info(uv_loop_t* loop, uv_metrics_t* metrics)
{
  (void)loop;
  (void)metrics;
  return UV_ENOSYS;
}

int uv_signal_init(uv_loop_t* loop, uv_signal_t* handle)
{
  (void)loop;
  (void)handle;
  return UV_ENOSYS;
}

int uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum)
{
  (void)handle;
  (void)signal_cb;
  (void)signum;
  return UV_ENOSYS;
}

int uv_signal_start_oneshot(uv_signal_t* handle, uv_signal_cb signal_cb, int signum)
{
  (void)handle;
  (void)signal_cb;
  (void)signum;
  return UV_ENOSYS;
}

int uv_signal_stop(uv_signal_t* handle)
{
  (void)handle;
  return UV_ENOSYS;
}

int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
{
  (void)ip;
  (void)port;
  (void)addr;
  return UV_ENOSYS;
}

int uv_ip6_addr(const char* ip, int port, struct sockaddr_in6* addr)
{
  (void)ip;
  (void)port;
  (void)addr;
  return UV_ENOSYS;
}

int uv_ip4_name(const struct sockaddr_in* src, char* dst, size_t size)
{
  (void)src;
  (void)dst;
  (void)size;
  return UV_ENOSYS;
}

int uv_ip6_name(const struct sockaddr_in6* src, char* dst, size_t size)
{
  (void)src;
  (void)dst;
  (void)size;
  return UV_ENOSYS;
}

int uv_ip_name(const struct sockaddr* src, char* dst, size_t size)
{
  (void)src;
  (void)dst;
  (void)size;
  return UV_ENOSYS;
}

int uv_inet_ntop(int af, const void* src, char* dst, size_t size)
{
  (void)af;
  (void)src;
  (void)dst;
  (void)size;
  return UV_ENOSYS;
}

int uv_inet_pton(int af, const char* src, void* dst)
{
  (void)af;
  (void)src;
  (void)dst;
  return UV_ENOSYS;
}

int uv_random(uv_loop_t* loop, uv_random_t* req, void *buf, size_t buflen, unsigned flags, uv_random_cb cb)
{
  (void)loop;
  (void)req;
  (void)buf;
  (void)buflen;
  (void)flags;
  (void)cb;
  return UV_ENOSYS;
}

int uv_if_indextoname(unsigned int ifindex, char* buffer, size_t* size)
{
  (void)ifindex;
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_if_indextoiid(unsigned int ifindex, char* buffer, size_t* size)
{
  (void)ifindex;
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_exepath(char* buffer, size_t* size)
{
  (void)buffer;
  (void)size;
  return UV_ENOSYS;
}

int uv_chdir(const char* dir)
{
  (void)dir;
  return UV_ENOSYS;
}

int uv_dlopen(const char* filename, uv_lib_t* lib)
{
  (void)filename;
  (void)lib;
  return UV_ENOSYS;
}

int uv_dlsym(uv_lib_t* lib, const char* name, void** ptr)
{
  (void)lib;
  (void)name;
  (void)ptr;
  return UV_ENOSYS;
}

int uv_utf16_to_wtf8(const uint16_t* utf16, ssize_t utf16_len, char** wtf8_ptr, size_t* wtf8_len_ptr)
{
  (void)utf16;
  (void)utf16_len;
  (void)wtf8_ptr;
  (void)wtf8_len_ptr;
  return UV_ENOSYS;
}

ssize_t uv_wtf8_length_as_utf16(const char* wtf8)
{
  (void)wtf8;
  return UV_ENOSYS;
}

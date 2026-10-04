/* Port-local: name lookup for Links' unused socket code; see sys/socket.h.
 * gethostbyname always fails. */
#ifndef LINKS_PYXIS_NETDB_H
#define LINKS_PYXIS_NETDB_H

struct hostent {
  char *h_name;
  char **h_aliases;
  int h_addrtype;
  int h_length;
  char **h_addr_list;
};
#define h_addr h_addr_list[0]

struct hostent *gethostbyname(const char *name);

#endif

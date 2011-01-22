#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

int print_sockaddr(const struct sockaddr *sa) {
  struct sockaddr_in *s4;
  struct sockaddr_in6 *s6;
  char dst[128];
  if(AF_INET == sa->sa_family) {
    s4 = (void*)sa;
    fprintf(stderr, "sockaddr: %s:%d\n", inet_ntop(AF_INET, &s4->sin_addr, dst, sizeof(dst)-1),
                             ntohs(s4->sin_port));
    
    return AF_INET;
  } else if (AF_INET6 == sa->sa_family) {
    s6 = (void*)sa;
    fprintf(stderr, "sockaddr: %s:%d\n", inet_ntop(AF_INET6, &s6->sin6_addr, dst, sizeof(dst)-1),
                             ntohs(s6->sin6_port));
    return AF_INET6;
  } else {
    return 0;
  }
}

int is_ip46_addr(const struct sockaddr *sa) {
  return ((sa->sa_family == AF_INET) || (sa->sa_family == AF_INET6));
}

int close(int fd) {
  static int (*real_close)(int fd) = NULL;
  if(!real_close) real_close = dlsym(RTLD_NEXT, "close");
  int ret = real_close(fd);
  return ret;
}

int socket(int domain, int type, int protocol) {
  static int (*real_socket)(int domain, int type, int protocol) = NULL;
  if(!real_socket) real_socket = dlsym(RTLD_NEXT, "socket");
  int ret = real_socket(domain, type, protocol);
  fprintf(stderr, "socket(%d, %d, %d): %d\n", domain, type, protocol, ret);
  return ret;
}

int connect(int sockfd, const struct sockaddr *addr,
                  socklen_t addrlen) {
  static int (*real_connect)(int sockfd, const struct sockaddr *addr, 
                  socklen_t addrlen) = NULL;
  int ret;
  if(!real_connect) real_connect = dlsym(RTLD_NEXT, "connect");

  if(is_ip46_addr(addr)) {
    ret = real_connect(sockfd, addr, addrlen);
#ifdef NOTYET
    hostnode = find_hostname_node(addr);
    if(hostnode) {
    } else { 
      /* no hostnode = no pending connections. */
      ret = make_new_connect(sockfd, hostnode, sockfd, addr);
    }
#endif
  } else {
    ret = real_connect(sockfd, addr, addrlen);
  }
  if (print_sockaddr(addr)) {
    fprintf(stderr, " connect(%d): %d\n", sockfd, ret);
    fprintf(stderr, "  errno: %s\n", strerror(errno));
  }
  return ret;
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  static int (*real_poll)(struct pollfd *fds, nfds_t nfds, int timeout) = NULL;
  if(!real_poll) real_poll = dlsym(RTLD_NEXT, "poll");
  int ret = real_poll(fds, nfds, timeout);
  // fprintf(stderr, "poll(%ld, %d): %d\n", nfds, timeout, ret);
  return ret;
}

int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout) {
  static int (*real_select)(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout) = NULL;
  if(!real_select) real_select = dlsym(RTLD_NEXT, "select");
  int ret = real_select(nfds, readfds, writefds, exceptfds, timeout);
  fprintf(stderr, "select(nfds:%d): %d\n", nfds, ret);
  return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  static ssize_t (*real_send)(int sockfd, const void *buf, 
                              size_t len, int flags) = NULL;
  if(!real_send) real_send = dlsym(RTLD_NEXT, "send");

  ssize_t ret = real_send(sockfd, buf, len, flags);
  fprintf(stderr, " send(%d) = %ld\n", sockfd, ret);
  return ret;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen) {
  static ssize_t (*real_sendto)(int sockfd, const void *buf, size_t len, 
                                int flags, const struct sockaddr *dest_addr, 
                                socklen_t addrlen) = NULL;

  if(!real_sendto) real_sendto = dlsym(RTLD_NEXT, "sendto");
  int i = real_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
  if(print_sockaddr(dest_addr)) {
    fprintf(stderr, " sendto(%d) = %d\n", sockfd, i);
  }
  return i;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  static ssize_t (*real_recv)(int sockfd, void *buf, size_t len, int flags) = NULL;
  
  if(!real_recv) real_recv = dlsym(RTLD_NEXT, "recv");
  ssize_t ret = real_recv(sockfd, buf, len, flags);
  fprintf(stderr, "recv(%d): %ld\n", sockfd, ret);
  return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) {
  static ssize_t (*real_recvfrom)(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) = NULL;

  if(!real_recvfrom) real_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
  ssize_t ret = real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
  if(print_sockaddr(src_addr)) {
    fprintf(stderr, " recvfrom(%d): %ld\n", sockfd, ret);
  }
  return ret;
}


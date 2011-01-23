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
#include <stdlib.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


#define lua_pcall_with_debug(L, nargs, nres) lua_pcall_with_debug_ex(L, nargs, nres, __FILE__, __LINE__)

int lua_pcall_with_debug_ex(lua_State *L, int nargs, int nresults, char *file, int lineno)
{
  int err = lua_pcall(L, nargs, nresults, 0);
  if(err != 0) {
    fprintf(stderr, "Lua error while performing lua_pcall at %s:%d: %s",
         file, lineno, lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  return err;
}

static void Lseti(lua_State *L, char *key, int value) {
  lua_pushstring(L, key);
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

static void Lsets(lua_State *L, char *key, char *value) {
  lua_pushstring(L, key);
  lua_pushstring(L, value);
  lua_settable(L, -3);
}

static void Lsetls(lua_State *L, char *key, char *value, int vlen) {
  lua_pushstring(L, key);
  lua_pushlstring(L, value, vlen);
  lua_settable(L, -3);
}

static void lua_pushsockaddr(lua_State *L, const struct sockaddr *sa) {
  struct sockaddr_in *s4;
  struct sockaddr_in6 *s6;
  char dst[128];
  lua_newtable(L);
  if(AF_INET == sa->sa_family) {
    s4 = (void*)sa;
    Lseti(L, "af", AF_INET);
    Lsets(L, "addr", (char *)inet_ntop(AF_INET, &s4->sin_addr, dst, sizeof(dst)-1));
    Lseti(L, "port", ntohs(s4->sin_port));
  } else if (AF_INET6 == sa->sa_family) {
    s6 = (void*)sa;
    Lseti(L, "af", AF_INET6);
    Lsets(L, "addr", (char *)inet_ntop(AF_INET6, &s6->sin6_addr, dst, sizeof(dst)-1));
    Lseti(L, "port", ntohs(s6->sin6_port));
  }
}



int lua_fill_sockaddr(lua_State *L, int index, struct sockaddr_storage *sa) {
  /* fill sockaddr from a table with af, addr, port elements */
  int af;
  char *addr;
  uint16_t port; 
  struct sockaddr_in *s4;
  struct sockaddr_in6 *s6;
  int ret = 0;

  lua_pushstring(L, "af");
  lua_gettable(L, index);
  af = luaL_checkint(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "port");
  lua_gettable(L, index);
  port = luaL_checkint(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "addr");
  lua_gettable(L, index);
  addr = (char *)luaL_checkstring(L, -1);
 
  if(af == AF_INET) {
    s4 = (void*)sa;
    s4->sin_family = AF_INET;
    s4->sin_port = htons(port);
    if(inet_pton(af, addr, &s4->sin_addr)) {
      ret = sizeof(struct sockaddr_in);
    }
  } else if (af == AF_INET6) {
    s6 = (void*)sa;
    s6->sin6_family = AF_INET6;
    s6->sin6_port = htons(port);
    if(inet_pton(af, addr, &s6->sin6_addr)) {
      ret = sizeof(struct sockaddr_in6);
    }
  } 
  lua_pop(L, 1);
  return ret;
   
}


static int Lsocket(lua_State *L) {
  int domain = luaL_checkint(L, 1);
  int type = luaL_checkint(L, 2);
  int protocol = luaL_checkint(L, 3);

  static int (*real_socket)(int domain, int type, int protocol) = NULL;
  if(!real_socket) real_socket = dlsym(RTLD_NEXT, "socket");

  int ret = real_socket(domain, type, protocol);

  ret = real_socket(domain, type, protocol);
  lua_pushinteger(L, ret);
  return 1;
}

static int Lconnect(lua_State *L) {
  static int (*real_connect)(int sockfd, const struct sockaddr *addr, 
                  socklen_t addrlen) = NULL;
  int ret;
  int sockfd = luaL_checkint(L, 1);
  struct sockaddr_storage sa;
  int sa_len;
  sa_len = lua_fill_sockaddr(L, 2, &sa);

  if(!real_connect) real_connect = dlsym(RTLD_NEXT, "connect");
  ret = real_connect(sockfd, (void*)&sa, sa_len);
  lua_pushinteger(L, ret);
  return 1;
}

static int Lsend(lua_State *L) {
  static ssize_t (*real_send)(int sockfd, const void *buf, 
                              size_t len, int flags) = NULL;
  int sockfd = luaL_checkint(L, 1);
  void *buf = (void*)luaL_checkstring(L, 2);
  int len = lua_objlen(L, 2);
  int flags = luaL_checkint(L, 3);

  if(!real_send) real_send = dlsym(RTLD_NEXT, "send");


  ssize_t ret = real_send(sockfd, buf, len, flags);
  lua_pushinteger(L, ret);
  return 1;
}

static int Lsendto(lua_State *L) {
  static ssize_t (*real_sendto)(int sockfd, const void *buf, size_t len, 
                                int flags, const struct sockaddr *dest_addr, 
                                socklen_t addrlen) = NULL;
  int sockfd = luaL_checkint(L, 1);
  void *buf = (void*) luaL_checkstring(L, 2);
  size_t len = lua_objlen(L, 2);
  int flags = luaL_checkint(L, 3);
  struct sockaddr_storage sa;
  int sa_len = lua_fill_sockaddr(L, 4, &sa);
  int ret;

  if(!real_sendto) real_sendto = dlsym(RTLD_NEXT, "sendto");
  ret = real_sendto(sockfd, buf, len, flags, (void*)&sa, sa_len);
  lua_pushinteger(L, ret); 
  return 1;
}

static int Lrecv(lua_State *L) {
  static ssize_t (*real_recv)(int sockfd, void *buf, size_t len, int flags) = NULL;

  int sockfd = luaL_checkint(L, 1);
  int len = luaL_checkint(L, 2);
  void *buf = malloc(len);
  int flags = luaL_checkint(L, 3);

  if(!real_recv) real_recv = dlsym(RTLD_NEXT, "recv");
  ssize_t ret = real_recv(sockfd, buf, len, flags);
  if (ret >= 0) {
    lua_pushlstring(L, buf, ret);
    free(buf);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Lrecvfrom(lua_State *L) {
  static ssize_t (*real_recvfrom)(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) = NULL;
  int sockfd = luaL_checkint(L, 1);
  int len = luaL_checkint(L, 2);
  void *buf = malloc(len);
  int flags = luaL_checkint(L, 3);
  struct sockaddr_storage sa;
  int sa_len = sizeof(sa);

  if(!real_recvfrom) real_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
  ssize_t ret = real_recvfrom(sockfd, buf, len, flags, (void*)&sa, &sa_len);
  lua_pushsockaddr(L, (void*)&sa);
  if (ret >= 0) {
    lua_pushlstring(L, buf, ret);
    free(buf);
  } else {
    lua_pushnil(L);
  }
  return 2;
}


static const luaL_reg o_funcs[] = {
  {"socket", Lsocket},
  {"connect", Lconnect},
  {"send", Lsend},
  {"sendto", Lsendto},
  {"recv", Lrecv},
  {"recvfrom", Lrecvfrom},
  {NULL, NULL}
};


static void check_lock(int latch) {
  static volatile int lock = 0;
  if(latch) {
    while (lock > 0) { 
      printf("locked, waiting!\n");
    }
    lock++;
  } else {
    lock--;
  }
}

static lua_State *Lua() {
  static lua_State *L = NULL;
  check_lock(1);
  if(!L) {
    L = lua_open();
    luaL_openlibs(L);
    luaL_openlib(L, "o", o_funcs, 0);

    if (luaL_dofile(L,"gncci.lua")!=0) {
      fprintf(stderr,"%s\n",lua_tostring(L,-1));
    } else {
      lua_getglobal(L, "gncci_init");
      lua_pcall_with_debug(L, 0, 0); 
    }
  } else {
    lua_gc(L, LUA_GCSTEP, 100);
  }
  return L;
}

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
/* 
int close(int fd) {
  static int (*real_close)(int fd) = NULL;
  if(!real_close) real_close = dlsym(RTLD_NEXT, "close");
  int ret = real_close(fd);
  return ret;
}
*/

int socket(int domain, int type, int protocol) {
  lua_State *L = Lua();
  int ret;

  lua_getglobal(L, "gncci_socket");
  lua_pushnumber(L, domain);
  lua_pushnumber(L, type);
  lua_pushnumber(L, protocol);
  lua_pcall_with_debug(L, 3, 1); 
  ret = lua_tonumber(L, -1);
  check_lock(0);
  return ret;
}

int connect(int sockfd, const struct sockaddr *addr,
                  socklen_t addrlen) {
  static int (*real_connect)(int sockfd, const struct sockaddr *addr, 
                  socklen_t addrlen) = NULL;
  int ret;
  lua_State *L = Lua();
  if(!real_connect) real_connect = dlsym(RTLD_NEXT, "connect");

  if(is_ip46_addr(addr)) {
    lua_getglobal(L, "gncci_connect");
    lua_pushnumber(L, sockfd);
    lua_pushsockaddr(L, addr); 
    lua_pcall_with_debug(L, 2, 1); 
    ret = lua_tonumber(L, -1);
  } else {
    ret = real_connect(sockfd, addr, addrlen);
  }
  check_lock(0);
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
  // fprintf(stderr, "select(nfds:%d): %d\n", nfds, ret);
  return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  int ret;
  lua_State *L = Lua();
  lua_getglobal(L, "gncci_send");
  lua_pushnumber(L, sockfd);
  lua_pushlstring(L, buf, len); 
  lua_pushnumber(L, flags);
  lua_pcall_with_debug(L, 3, 1); 
  ret = lua_tonumber(L, -1);
  check_lock(0);
  return ret;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen) {
  int ret;
  lua_State *L = Lua();
  lua_getglobal(L, "gncci_sendto");
  lua_pushnumber(L, sockfd);
  lua_pushlstring(L, buf, len); 
  lua_pushnumber(L, flags);
  lua_pushsockaddr(L, dest_addr); 
  lua_pcall_with_debug(L, 4, 1); 
  ret = lua_tonumber(L, -1);
  check_lock(0);
  return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  int ret;
  lua_State *L = Lua();
  int retlen;
  char *retbuf;
  lua_getglobal(L, "gncci_recv");
  lua_pushnumber(L, sockfd);
  lua_pushnumber(L, len); 
  lua_pushnumber(L, flags);
  lua_pcall_with_debug(L, 3, 1); 
  if (lua_isnil(L, -1)) {
    ret = -1;
  } else {
    retbuf = (void *)lua_tostring(L, -1);
    retlen = lua_objlen(L, -1);
    memcpy(buf, retbuf, retlen);
    ret = retlen;
  }
  check_lock(0);
  return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) {
  int ret;
  lua_State *L = Lua();
  int retlen;
  char *retbuf;
  int retsalen;
  struct sockaddr_storage sa;
  lua_getglobal(L, "gncci_recvfrom");
  lua_pushnumber(L, sockfd);
  lua_pushnumber(L, len); 
  lua_pushnumber(L, flags);
  lua_pushsockaddr(L, src_addr); 
  lua_pcall_with_debug(L, 4, 2); 
  retsalen = lua_fill_sockaddr(L, lua_gettop(L)-1, &sa);
  memcpy(src_addr, &sa, *addrlen);
  *addrlen = retsalen;
  
  if (lua_isnil(L, -1)) {
    ret = -1;
  } else {
    retbuf = (void *)lua_tostring(L, -1);
    retlen = lua_objlen(L, -1);
    memcpy(buf, retbuf, retlen);
    ret = retlen;
  }
  check_lock(0);
  return ret;
}


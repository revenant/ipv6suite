/*
 * uml switch client 
 * Greg Daley greg.daley@eng.monash.edu.au
 * Copyright (C) 2002 
 *
 * status : rude!
 * 
 * Adapted from code provide by the following Authors:
 * Copyright (C) 2001 Lennert Buytenhek (buytenh@gnu.org) and
 * James Leu (jleu@mindspring.net).
 * Copyright (C) 2001 by various other people who didn't put their name here.
 * Licensed under the GPL.
 */

#ifndef SW_CONSTANTS_H
#define SW_CONSTANTS_H
#include <sys/un.h>
#include <netinet/ether.h>

#ifndef UNIX_MAX_PATH
#define UNIX_MAX_PATH 108
#endif  /* UNIX_MAX_PATH */


#ifndef UNIX_SOCKET_CONTROL_PATH_DFL 
#define UNIX_SOCKET_CONTROL_PATH_DFL  "/tmp/uml.ctl"
#endif  /* UNIX_SOCKET_CONTROL_PATH_DFL */


#ifndef UNIX_SOCKET_DATA_PATH_DFL 
#define UNIX_SOCKET_DATA_PATH_DFL     "/tmp/uml.data"
#endif  /* UNIX_SOCKET_DATA_PATH_DFL */

enum request_type { REQ_NEW_CONTROL };
 
struct request {
  enum request_type type;
  union {
    struct {
            unsigned char addr[ETH_ALEN];
            struct sockaddr_un name;
    } new_control;
    struct {
            unsigned long cookie;
    } new_data;
  } u;
};
#endif /* SW_CONSTANTS_H */

// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Nodes/IPv6/Attic/UmlSwitch.h,v 1.2 2005/02/10 04:00:43 andras Exp $
//
// Uml Switch Client interface for Omnet++ IPv6.
// * Copyright (C) 2002 Greg Daley Monash University, Melbourne, Australia
//
// This work is  Adapted from code provided by the following Authors:
// * Copyright (C) 2001 Johnny Lai Monash University, Melbourne, Australia
// * Copyright (C) 2001 Eric Wu    Monash University, Melbourne, Australia
// * Copyright (C) 2001 Lennert Buytenhek (buytenh@gnu.org) and
// * James Leu (jleu@mindspring.net).
// * Copyright (C) 2001 by various other people who didn't put their name here.
// * Licensed under the GPL.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
	@file: UmlSwitch.h
	Purpose: Implements the NetworkInterface Ned module Interfaces to \
                 uml_switch
*/

#ifndef __UMLSWITCHINTERFACE_H__
#define __UMLSWITCHINTERFACE_H__

#include <omnetpp.h>

#ifdef TESTIPv6
#undef NDEBUG
#endif
#include <cassert>


#include "LinkLayerModule.h"
#include "MACAddress.h"
#if defined __CN_PAYLOAD_H
extern "C" {
#include "UmlSwitchDefs.h"
}


/**
   @class UmlSwitchInterface
   comment: trying to create an interface to send/receive data from UML
 */
class UmlSwitchInterface: public LinkLayerModule
{
public:
  Module_Class_Members(UmlSwitchInterface,LinkLayerModule,16384);

  virtual void initialize();
  virtual void activity();

  int  set_new_data_path(const char *newval, char * dest);
  int  set_new_control_path(const char *newval, char * dest);

private:

  void initfds(int fds[]);
  void sw_init_args(char *control_path, char *data_path);
    /* return the fd tuple (control, send, receive) or NULL */
  int *init_swdriver(char *control_path, char *data_path,
		unsigned char *macaddr, int fds[]);

  struct sockaddr_un *new_addr(void *name, int len);
  struct sockaddr_un *gen_local_addr(void);

  int sw_unix_connect(struct sockaddr_un *unixsock, int type);
  int sw_data_connect(char *data_path);
  int sw_control_connect(char *control_path);

  int send_control_request(int fd,struct sockaddr_un *unix_sock,
                                              unsigned char *addr);


  int pack_mac_addr(const char * straddr, unsigned char *pack);

  char control_path[ UNIX_MAX_PATH ];
  char data_path[  UNIX_MAX_PATH ];
  int ctlfd;
  int readfd;
  int writefd;
  MACAddress my_addr;
};
#endif //__CN_PAYLOAD_H
#endif /* __UMLSWITCHINTERFACE_H__  */




// -*- C++ -*-
// Copyright (C) 2001, 2004 Monash University, Melbourne, Australia
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

/*
	@file LinkLayerModule.h
	@brief Header file for LinkLayerModule.h

	An 'abstract' class for all network interface classes

	@author Eric Wu
*/

#ifndef LINKLAYER_MODULE_H
#define LINKLAYER_MODULE_H

#ifndef ROUTINGTABLEACCESS_H
#include "RoutingTableAccess.h"
#endif //ROUTINGTABLEACCESS_H

#ifndef STRING
#define STRING
#include <string>
#endif //STRING


// XXX TBD eliminate this constant from everywhere!
#define NWI_IDLE    13

class LinkLayerModule : public RoutingTableAccess
{
 public:
  Module_Class_Members(LinkLayerModule, RoutingTableAccess, 0);
  virtual ~LinkLayerModule();
  virtual void initialize();  

  const char* getInterfaceName() const {return iface_name.c_str();}  
  int getInterfaceType() const {return iface_type;}

 protected:
  simtime_t delay;

  std::string iface_name;
  int iface_type; // confront with OMNeT++ protocol.h

  void setIface_name(int llProtocol);

  ///counter for number of incoming packets 
  long cntReceivedPackets;

};

#endif

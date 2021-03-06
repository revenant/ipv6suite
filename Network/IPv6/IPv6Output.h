// -*- C++ -*-
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef IPv6OUTPUTCORE_H
#define IPv6OUTPUTCORE_H

#include "QueueBase.h"

class IPv6Datagram;
class IPv6Forward;
class RoutingTable6;
class InterfaceTable;

/**
 * Queues up outgoing IPv6 datagrams, and assigns MAC addresses to them.
 * TBD refine comment.
 *
 * @author Johnny Lai
 */
class IPv6Output : public QueueBase
{
public:
  Module_Class_Members(IPv6Output, QueueBase, 0);

  virtual void initialize();
  virtual void endService(cMessage* msg);
  virtual void finish();

private:
  void processArrivingMessage(IPv6Datagram* msg);

  InterfaceTable *ift;
  RoutingTable6 *rt;

  unsigned int ctrIP6OutForwDatagrams;
  unsigned int ctrIP6OutMcastPkts;
  
  unsigned int ctrOutPackets;
  unsigned int ctrOutOctets;
  ::IPv6Forward* forwardMod; // XXX why's this needed? why not connection? --AV
};

#endif


// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/LocalDeliver6Core.h,v 1.2 2005/02/10 06:26:20 andras Exp $
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
   @file LocalDeliver6Core.h
   @brief Receive IPv6 datagrams for local delivery

   Responsibilities:
     strip off IP header
     Process destination options
     Forward to IPv6Encapsulation module if decapsulated payload is another datagram.
     buffer fragments for ip_fragmenttime
     wait until all fragments of one fragment number are received
     discard without notification if not all fragments arrive in
     ip_fragmenttime
     Defragment once all fragments have arrived
     send Transport packet up to the transport layer
     send ICMP packet to ICMP module
     send IGMP group management packet to Multicast module
     send tunneled IP datagram to PreRouting

   @author Johnny Lai

*/


#ifndef LOCALDELIVER6CORE_H
#define LOCALDELIVER6CORE_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

/*  -------------------------------------------------
    Constants
    -------------------------------------------------   */

const int FRAGMENT_BUFFER_MAXIMUM = 1000;

/*  -------------------------------------------------
    structures
    -------------------------------------------------   */
struct FragmentationBufferEntry
{
  bool isFree;

  int fragmentId;
  int fragmentOffset; // unit: 8 bytes
  bool moreFragments;
  int length; // length of fragment excluding header, in byte

  simtime_t timeout;
};

/*  -------------------------------------------------
    Main Module class: LocalDeliverCore
    -------------------------------------------------    */
class IPv6InterfacePacket;
class IPv6Datagram;

/**
 * @class LocalDeliver6Core
 * @brief Simple module to handle local delivery of IPv6 Datagrams
 */

class LocalDeliver6Core: public cSimpleModule
{
public:
  Module_Class_Members(LocalDeliver6Core, cSimpleModule, 0);

  virtual void initialize();
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

private:
  // functions to handle Fragmentation Buffer

  void eraseTimeoutFragmentsFromBuf();
  void insertInFragmentBuf(IPv6Datagram *d);
  int getPayloadSizeFromBuf(int fragmentId);
  bool datagramComplete(int fragmentId);
  void removeFragmentFromBuf(int fragmentId);

  ///Returns true if packet needs further processing as InterfacePacket
  bool processDatagram(IPv6Datagram* datagram);

  IPv6InterfacePacket *setInterfacePacket(IPv6Datagram *);

  simtime_t fragmentTimeoutTime;
  simtime_t delay;
  bool hasHook;
  unsigned int ctrIP6InUnknownProtos;
  unsigned int ctrIP6InDeliver;

  int fragmentBufSize;
  FragmentationBufferEntry fragmentBuf [FRAGMENT_BUFFER_MAXIMUM];
  cMessage* waitTmr;
  IPv6InterfacePacket *interfacePacket;
  IPv6Datagram* dgram;
  simtime_t lastCheckTime;
  cQueue waitQueue;

};

#endif

//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2001 CTIE, Monash University
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
   @file IPv6LocalDeliver.cc
   @brief Implementation for Simple Module LocalDeliverCore

   Responsibilities:
        strip off IP header
        Process Destination Options
        Forward to IPv6Encapsulation module if decapsulated payload is another datagram.
        buffer fragments for ip_fragmenttime
        wait until all fragments of one fragment number are received
        discard without notification if not all fragments arrive in
        ip_fragmenttime
        Defragment once all fragments have arrived
        send Transport packet up to the transport layer
        send ICMP packet to ICMP module
        send IGMP group management packet to Multicast module
   Notation:
        TCP-Packets --> transportOut[0]
        UDP-Packets --> transportOut[1]
   Based on LocalDeliverCore by Jochen Reber
   @author Johnny Lai

*/

#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>
#include <omnetpp.h>

#ifdef TESTIPv6
#undef NDEBUG
#endif //TESTIPv6

#include <cassert>


#include "IPv6LocalDeliver.h"
#include "HdrExtFragProc.h"
#include "HdrExtRteProc.h"
#include "IPv6InterfacePacket_m.h"
#include "IPv6Datagram.h"


Define_Module( IPv6LocalDeliver );

void IPv6LocalDeliver::initialize()
{
  ctrIP6InUnknownProtos = 0;
  ctrIP6InDeliver = 0;
}

void IPv6LocalDeliver::handleMessage(cMessage* theMsg)
{
  // TBD implement fragmentation reassembly

  IPv6Datagram* datagram = check_and_cast<IPv6Datagram*>(theMsg);
  if (!processDatagram(datagram))
  {
    delete theMsg;
    return;
  }

  //Give ICMP the direct packet as it will need all the gory details
  if (datagram->transportProtocol() == IP_PROT_IPv6_ICMP)
  {
    send(datagram, "ICMPOut");
    return;
  }

  if (datagram->transportProtocol() == IP_PROT_IPv6)
  {
    send(datagram, "tunnelOut");
    return;
  }

#ifdef USE_MOBILITY
  if (datagram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
  {
    send(datagram, "mobilityOut");
    return;
  }
#endif // USE_MOBILITY

  IPv6InterfacePacket *interfacePacket = decapsulateDatagram(datagram);

  switch(interfacePacket->protocol())
  {
    case IP_PROT_IGMP:
      send(interfacePacket, "multicastOut");
      break;

    case IP_PROT_IP: //IPv4 packets
      //send(interfacePacket, "preRoutingOut");
      cerr<<"IPv4 in IPv6 tunnels not implemented "<<endl;
      delete interfacePacket;
      break;

    case IP_PROT_TCP:
      send(interfacePacket, "transportOut",0);
      ctrIP6InDeliver++;
      break;

    case IP_PROT_UDP:
      send(interfacePacket, "transportOut",1);
      ctrIP6InDeliver++;
      break;

    default:
      cerr << "LocalDeliver6 Error: "
           << "Transport protocol invalid: "
           << (int)(interfacePacket->protocol())
           << "\n";
      //TODO send a parameter problem with code 1 (unrecognised Next header)
      ctrIP6InUnknownProtos++;
      delete interfacePacket;
      break;
  } // end switch
}

void IPv6LocalDeliver::finish()
{
  recordScalar("IP6InUnknownProtos", ctrIP6InUnknownProtos);
  recordScalar("IP6InDeliver", ctrIP6InDeliver);
}

bool IPv6LocalDeliver::processDatagram(IPv6Datagram* datagram)
{
    //Process Destination options
    HdrExtProc* proc = 0;

    bool success = true;
    bool localdeliver = true;

    //Todo Fragments processHeader will send dgram to Fragmentation.
    //Fragmatentation mod will collect these dgrams.  When it has enough
    //it will send it back here to be delivered to upper layer directly
    for ( proc = datagram->getNextHeader(0); proc != 0 && success ;
          proc = datagram->getNextHeader(proc))
    {
      HdrExtRteProc* rtProc = 0;
      switch(proc->type())
      {
        case NEXTHDR_DEST:
          success = proc->processHeader(this, datagram);
          break;

        case NEXTHDR_ROUTING:
          localdeliver = false;
          if (proc->type() == EXTHDR_ROUTING &&
              ((rtProc = boost::polymorphic_downcast<HdrExtRteProc*>(proc)) != 0) &&
              !rtProc->isSegmentsLeft())
            localdeliver = true;
          else
          {
            success = proc->processHeader(this, datagram);
          }
          break;

        case NEXTHDR_FRAGMENT:
          error("fragmented IPv6 datagram encountered -- fragmentation not implemented!");
          break;

        case NEXTHDR_HOP:
          // nothing to do
          break;

        default:
          cerr<<className()<<" Unknown type "<<dec<<proc->type()<<" processed"<<endl;
          //send ICMP parameter problem with code 2 unrecognised option?
          break;
      }
    }

    //ProcessHeader should take care of pdu lifetime if failed
    return success && localdeliver;
}

IPv6InterfacePacket *IPv6LocalDeliver::decapsulateDatagram(IPv6Datagram *datagram)
{
  IPv6InterfacePacket *interfacePacket = new IPv6InterfacePacket;

  cMessage *packet = datagram->decapsulate();
  interfacePacket->encapsulate(packet);
  interfacePacket->setName(packet->name());
  interfacePacket->setProtocol(datagram->transportProtocol());
  interfacePacket->setSrcAddr(datagram->srcAddress());
  interfacePacket->setDestAddr(datagram->destAddress());
  delete datagram;

  return interfacePacket;
}


// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/LocalDeliver6Core.cc,v 1.2 2005/02/10 05:43:47 andras Exp $
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
   @file LocalDeliver6Core.cc
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

#include <omnetpp.h>

#ifdef TESTIPv6
#undef NDEBUG
#endif //TESTIPv6

#include <cassert>
#include <boost/cast.hpp>


#include "LocalDeliver6Core.h"
#include "HdrExtFragProc.h"
#include "HdrExtRteProc.h"
#include "IPv6InterfacePacket.h"
#include "IPv6Datagram.h"


using boost::polymorphic_downcast;


Define_Module( LocalDeliver6Core );

/*  ----------------------------------------------------------
    Public Functions
    ----------------------------------------------------------  */

// initialisation: set fragmentTimeout
// tbd: include fragmentTimeout in .ned files
void LocalDeliver6Core::initialize()
{

  fragmentTimeoutTime = strToSimtime(par("fragmentTimeout"));
  delay = par("procdelay");
  hasHook = (findGate("netfilterOut") != -1);
  ctrIP6InUnknownProtos = 0;
  ctrIP6InDeliver = 0;

  int i;
  for (i=0; i < FRAGMENT_BUFFER_MAXIMUM; i++)
  {
    fragmentBuf[i].isFree = true;
    fragmentBuf[i].fragmentId = -1;
  }

  fragmentBufSize = 0;
  lastCheckTime = 0;
  interfacePacket = 0;
  dgram = 0;
  waitTmr = new cMessage("LocalDeliver6CoreWait");
  waitQueue.setName("localDeliver6WaitQ");

  std::string display(parentModule()->displayString());
  display += ";q=";
  display += waitQueue.name();
  parentModule()->setDisplayString(display.c_str());

  //so display at self
  display = static_cast<const char*>(displayString());
  display += ";q=";
  display += waitQueue.name();
  setDisplayString(display.c_str());
}

void LocalDeliver6Core::handleMessage(cMessage* theMsg)
{
  // erase timed out fragments in fragmentation buffer
  // check every 1 second max
  if (simTime() >= lastCheckTime + 1)
  {
    lastCheckTime = simTime();
    eraseTimeoutFragmentsFromBuf();
  }


  if (!theMsg->isSelfMessage())
  {
    IPv6Datagram* datagram(boost::polymorphic_downcast<IPv6Datagram*> (theMsg));
    assert(datagram);

    if (waitTmr->isScheduled())
    {
      //cerr<<fullPath()<<" "<<simTime()<<" received new packet "<<*datagram
      //    <<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime();
      Dout(dc::custom|flush_cf, fullPath()<<" "<<simTime()<<" received new packet "<<*datagram
           <<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime());
      waitQueue.insert(datagram);
      return;
    }

    //TODO Take care of a dest of ALL_NODES_NODE_ADDRESS perhaps
    //By dup the packet and passing it to every ipv6 address on the host

    // pass Datagram through netfilter if it exists
/*
    if (hasHook)
    {
      send(datagram, "netfilterOut");
      dfmsg = receiveNewOn("netfilterIn");
      if (dfmsg->kind() == DISCARD_PACKET)
      {
        delete dfmsg;

        continue;
      }

      datagram = polymorphic_downcast<IPv6Datagram*>(dfmsg);
    }
*/

    // Defragmentation
    // skip Degragmentation if single Fragment Datagram
//     if (datagram->findNextHdr(NEXTHDR_FRAGMENT) > 0)
//     {
//       HdrExtFragProc* proc = datagram->acquireFragInterface();
//       assert(proc);

//       insertInFragmentBuf( datagram );
//       if (!datagramComplete(proc->fragmentId()))
//       {
//         delete(datagram);
//         continue;
//       }
//       //datagram->setLength( datagram->headerLength()*8 +
//       //  datagram->encapsulatedMsg()->length() );

//       //Looks like this function is deprecated perhaps?
//       //getPayloadSizeFromBuf( datagram->fragmentId() ) );

//       /*
//         ev << "\ndefragment\n";
//         ev << "\nheader length: " << datagram->headerLength()*8
//         << "  encap length: " << datagram->encapsulatedMsg()->length()
//         << "  new length: " << datagram->length() << "\n";
//       */

//       removeFragmentFromBuf(proc->fragmentId());
//     }

    if (!processDatagram(datagram))
      return;

    if (!waitTmr->isScheduled() && 0 == interfacePacket)
    {
      scheduleAt(delay + simTime(), waitTmr);
      interfacePacket = setInterfacePacket(datagram);
      delete datagram;
      return;
    }
    Dout(dc::custom|flush_cf, fullPath()<<" Where the hell is this packet from protocol="
         <<(datagram?(int)datagram->transportProtocol():0)<<" dgram="
         <<(datagram?*datagram:*(new IPv6Datagram)));
    assert(false);
    return;

  } //end Received a packet


  if (dgram != 0)
  {

    if (processDatagram(dgram))
    {
      scheduleAt(delay + simTime(), waitTmr);
      interfacePacket = setInterfacePacket(dgram);
      delete dgram;
    }
    dgram = 0;
    return;
  }

  assert(interfacePacket);

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

  if (!waitQueue.empty())
  {
    dgram = polymorphic_downcast<IPv6Datagram*>(waitQueue.pop());
    assert(dgram != 0);
    scheduleAt(delay + simTime(), waitTmr);
  }
  else
    dgram = 0;

  interfacePacket = 0;
}

void LocalDeliver6Core::finish()
{
  recordScalar("IP6InUnknownProtos", ctrIP6InUnknownProtos);
  recordScalar("IP6InDeliver", ctrIP6InDeliver);

}

bool LocalDeliver6Core::processDatagram(IPv6Datagram* datagram)
{

    //Process Destination options
    HdrExtProc* proc = 0;

    bool success = true;
    bool localdeliver = true;
    bool processFurther = false;

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
              ((rtProc = polymorphic_downcast<HdrExtRteProc*>(proc)) != 0) &&
              !rtProc->isSegmentsLeft())
            localdeliver = true;
          else
          {
            success = proc->processHeader(this, datagram);
          }
          break;

        case NEXTHDR_FRAGMENT:
          break;

        case NEXTHDR_HOP:
          proc = datagram->getNextHeader(proc);
          break;

        default:
          cerr<<className()<<" Unknown type "<<dec<<proc->type()<<" processed"<<endl;
          //send ICMP parameter problem with code 2 unrecognised option?
          break;
      }
    }

    //ProcessHeader should take care of pdu lifetime if failed
    if (!success || !localdeliver)
      return processFurther;

//end processHeader

	//Give ICMP the direct packet as it will need all the gory details
    if (datagram->transportProtocol() == IP_PROT_IPv6_ICMP)
    {
      send(datagram, "ICMPOut");
      return processFurther;
    }

    if (datagram->transportProtocol() == IP_PROT_IPv6)
    {
      send(datagram, "tunnelOut");
      return processFurther;
    }

#ifdef USE_MOBILITY
    if (datagram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
    {
      send(datagram, "mobilityOut");

      return processFurther;
    }
#endif // USE_MOBILITY

    return true;
}

/*  ----------------------------------------------------------
		Private functions
    ----------------------------------------------------------  */

IPv6InterfacePacket *LocalDeliver6Core::setInterfacePacket
(IPv6Datagram *datagram)
{
  cPacket *packet;
  IPv6InterfacePacket *interfacePacket = new IPv6InterfacePacket;

  packet = datagram->decapsulate();
  interfacePacket->encapsulate(packet);
  interfacePacket->setName(packet->name());
  interfacePacket->setProtocol(datagram->transportProtocol());
  interfacePacket->setSrcAddr(datagram->srcAddress());
  interfacePacket->setDestAddr(datagram->destAddress());

  return interfacePacket;
}

/*  ----------------------------------------------------------
		Private functions: Fragmentation Buffer management
    ----------------------------------------------------------  */

// erase those fragments from the buffer that have timed out
void LocalDeliver6Core::eraseTimeoutFragmentsFromBuf()
{
/*
  int i;
  simtime_t curTime = simTime();

  for (i=0; i < fragmentBufSize; i++)
  {
  if (!fragmentBuf[i].isFree &&
  curTime > fragmentBuf[i].timeout)
  {
      // debugging output
      ev << "++++ fragment kicked out: "
      << i << " :: "
      << fragmentBuf[i].fragmentId << " / "
      << fragmentBuf[i].fragmentOffset << " : "
      << fragmentBuf[i].timeout << "\n";

      fragmentBuf[i].isFree = true;
      } // end if
      } // end for
*/
}

void LocalDeliver6Core::insertInFragmentBuf(IPv6Datagram *d)
{
/*
  int i;
  FragmentationBufferEntry *e;

  for (i=0; i < fragmentBufSize; i++)
  {
  if (fragmentBuf[i].isFree == true)
  {
  break;
  }
  } // end for

	// if no free place found, increase Buffersize to append entry
	if (i == fragmentBufSize)
    fragmentBufSize++;

	e = &fragmentBuf[i];
	e->isFree = false;
	e->fragmentId = d->fragmentId();
	e->fragmentOffset = d->fragmentOffset();
	e->moreFragments = d->moreFragments();
	e->length = d->totalLength() - d->headerLength();
	e->timeout= simTime() + fragmentTimeoutTime;
*/
}

bool LocalDeliver6Core::datagramComplete(int fragmentId)
{
/*
  bool isComplete = false;
  int nextFragmentOffset = 0; // unit: 8 bytes
  bool newFragmentFound = true;
  int i;

  while(newFragmentFound)
  {
  newFragmentFound = false;
  for (i=0; i < fragmentBufSize; i++)
  {
  if (!fragmentBuf[i].isFree &&
  fragmentId == fragmentBuf[i].fragmentId &&
  nextFragmentOffset == fragmentBuf[i].fragmentOffset)
  {
  newFragmentFound = true;
  nextFragmentOffset += fragmentBuf[i].length;
      // Datagram complete if last Fragment found
      if (!fragmentBuf[i].moreFragments)
      {
      return true;
      }
          // reset i to beginning of buffer
          } // end if
          } // end for
          } // end while

              // when no new Fragment found, Datagram is not complete
              */
  return false;
}

int LocalDeliver6Core::getPayloadSizeFromBuf(int fragmentId)
{
/*
  int i;
  int payload = 0;

  for (i=0; i < fragmentBufSize; i++)
  {
  if (!fragmentBuf[i].isFree &&
  fragmentBuf[i].fragmentId == fragmentId)
  {
  payload += fragmentBuf[i].length;
  } // end if
  } // end for

  return payload;
*/
  return 0;
}

void LocalDeliver6Core::removeFragmentFromBuf(int fragmentId)
{
/*
  int i;

  for (i=0; i < fragmentBufSize; i++)
  {
  if (!fragmentBuf[i].isFree &&
  fragmentBuf[i].fragmentId == fragmentId)
  {
  fragmentBuf[i].fragmentId = -1;
  fragmentBuf[i].isFree = true;
  } // end if
  } // end for
*/
}


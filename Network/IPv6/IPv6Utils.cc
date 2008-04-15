//
// Copyright (C) 2001, 2004 CTIE, Monash University
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

/**
   @file opp_utils.cc
   @brief Global utility functions
   @author Johnny Lai
 */


#include "sys.h"
#include "debug.h"
#include <cstring>
#include <fstream>

#include <omnetpp.h>
#include "IPv6Datagram.h"
#include "HdrExtProc.h"
#include "MobilityHeaderBase.h"
#include "RTPPacket.h"
#include "RTCPSR.h"
#include "ICMPv6NDMessage.h"
#include <sstream>
#include "RTP.h"

namespace IPv6Utils
{
  std::ostream& printRoutingInfo(bool routingInfoDisplay, IPv6Datagram* datagram, const char* name, bool directionOut)
  {
    static bool encapsulate = false;
    static ostream* osp = 0;
    std::string filename = "test.out";

    if (!osp)
    {
      if (simulation.moduleByPath("worldProcessor"))
      {
      filename = simulation.moduleByPath("worldProcessor")->par("datagramTraceFile").stringValue();
      if (filename != "")
      {
	std::ostringstream ostr;
	ostr<<simulation.runNumber();
	if (filename == "autocreate")
	  filename = std::string(simulation.systemModule()->name()) + "-" + ostr.str() + ".out";
      }
      else
	filename = "test.out";
      }
	
      osp = new ofstream(filename.c_str(), ios_base::out|ios_base::binary);

    }
    ostream& os = *osp;
        
    if (!datagram)
      return os;

    if (datagram->kind() == 1 || routingInfoDisplay)
    {
      if (!encapsulate)
	os<<name<<" "<<(directionOut?"-->":"<--")<<" "<<simulation.simTime();

      if (datagram->transportProtocol() != IP_PROT_IPv6)
	os<<" payload="<<datagram->name();

      os<<" src="<<datagram->srcAddress()<<" dest="
	<<datagram->destAddress()<<" len="<<(datagram->length()/BITS)<<" bytes"<<" hl="<<datagram->hopLimit();

      for (HdrExtProc* proc = datagram->getNextHeader(0); proc != 0;
	   proc = datagram->getNextHeader(proc))
      {
	proc->operator<<(os);
      }
      if (datagram->transportProtocol() == IP_PROT_IPv6)
      {
	os<<" [encapsulating]=>";
	encapsulate = true;
	IPv6Datagram* dgram = 
	  check_and_cast<IPv6Datagram*> (datagram->encapsulatedMsg());
	printRoutingInfo(true, dgram, name, directionOut);
	encapsulate = false;
      }
      if (datagram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
      {
	MobilityHeaderBase* mhb = 
	  check_and_cast<MobilityHeaderBase*>(datagram->encapsulatedMsg());
	if (mhb)
	  os<<*mhb;
      }
      if (datagram->transportProtocol() == IP_PROT_UDP)
      {
	if (!datagram->encapsulatedMsg())
	  assert(false);
	RTPPacket* pkt = dynamic_cast<RTPPacket*> (datagram->encapsulatedMsg()->encapsulatedMsg());
	if (pkt)
	  os<<" seq no " <<pkt->seqNo();
	else
	{
	  RTCPPacket* pkt = dynamic_cast<RTCPPacket*> (datagram->encapsulatedMsg()->encapsulatedMsg());
	  if (!pkt)
	    os<<" unknown udp packet";
	  else
	    {
	      RTCPSR* sr = dynamic_cast<RTCPSR*> (pkt);
	      if (sr)
	      {
		os<<" "<<sr->ssrc()<<" pkt="<<sr->packetCount()<<" oct="<<sr->octetCount()
		  <<" rtcplen="<<sr->byteLength()<<" blks="<<sr->reportBlocksArraySize();
		  for (unsigned int i = 0; i <sr->reportBlocksArraySize();++i)
		    os<<" blk["<<i<<"] "<<sr->reportBlocks(i);
	      }
	      else
	      {		
		os<<" "<<pkt->ssrc()<<" rtcplen="<<pkt->byteLength();
	      }
	    }
	}
	  
      }
      if (datagram->transportProtocol() == IP_PROT_IPv6_ICMP)
      {
	ICMPv6Message* icmpmsg = 
	  check_and_cast<ICMPv6Message*>(datagram->encapsulatedMsg());
	if (icmpmsg->type() == ICMPv6_ROUTER_AD)
	  os<<*(check_and_cast<IPv6NeighbourDiscovery::ICMPv6NDMRtrAd*> (icmpmsg));
      }
      os<<endl;

    }

    return os;
    
  }
}

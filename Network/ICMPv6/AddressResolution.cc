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
   @file AddressResolution.cc
   @brief Implementation of Address Resolution
   @date 13.11.01
*/

#include "sys.h"
#include "debug.h"


#include <memory>
#include <climits>
#include <iomanip> //setprecision
#include <string>
#include <iostream>
#include <boost/bind.hpp>

#include "AddressResolution.h"
#include "IPv6Datagram.h"
#include "ICMPv6NDMessage.h"
#include "ICMPv6MessageUtil.h"
#include "InterfaceTableAccess.h"
#include "IPv6InterfaceData.h"
#include "RoutingTable6Access.h"
#include "IPv6Forward.h"
#include "cTimerMessage.h"
#include "cSignalMessage.h"
#include "AddrResInfo_m.h"
#include "NDTimers.h"
#include "opp_utils.h"
#include "IPv6CDS.h"
#include "IPv6Output.h"
#include "LL6ControlInfo_m.h"

#include "stlwatch.h"

using std::string;
using IPv6NeighbourDiscovery::NDHOPLIMIT;
using IPv6NeighbourDiscovery::NDARTimer;

typedef  PendingPacketQ::iterator PPQI;

static const string pendingQueueIn("pendingQueueIn");
static const string ICMPIn("ICMPIn");

static const int Tmr_AddrReslnTimeout = 9000;
static const int Tmr_AddrReslnTimeoutFail = 9001;
static const size_t MAX_MULTICAST_SOLICIT = 3;

Define_Module( AddressResolution );

size_t AddressResolution::outputUnicastGate = UINT_MAX;

void AddressResolution::initialize(int stage)
{
  if (stage==1)
  {
    WATCH_PTRMULTIMAP(ppq);

    // we have to postpone initialization to stage 1, because interfaces
    // get registered in stage 0 and we need them.

    ift = InterfaceTableAccess().get();
    rt = RoutingTable6Access().get();

    //XXX Don't know why on OSF1 requires the global scope qualifier.  Is there
    //another class with exactly same name?
    fc = check_and_cast<IPv6Forward*> (
      OPP_Global::findModuleByTypeDepthFirst(this, "IPv6Forward")); // XXX try to get rid of pointers to other modules --AV
    assert(fc != 0);

    ctrIcmp6OutNgbrAdv = 0;
    ctrIcmp6OutNgbrSol = 0;

    if (ift->numInterfaceGates()==0)
    {
      outputUnicastGate = UINT_MAX;
      outputMod = NULL; //XXX is that enough? --AV
      return;
    }
    outputMod = new cModule*[ift->numInterfaceGates()];
    //nd->icmp->proc
    cModule* procMod = parentModule();
    assert(procMod);
    //ned gen sources don't have header files for incl.
    //assert(check_and_cast<IPv6Processing*>(procMod));
    for (unsigned int i = 0; i < ift->numInterfaceGates(); i++)
    {
      outputMod[i] = procMod->submodule("output", i);
      assert(outputMod[i]);
      assert(check_and_cast<IPv6Output*>(outputMod[i]));
      if (!outputMod[i])
        DoutFatal(dc::core|error_cf, "Cannot find IPv6OutputCore module");
    }

    // Initialise this magic number once only as its constant during runtime and
    // should be same for modules of same type
    if (outputMod[0] != 0 && outputUnicastGate == UINT_MAX)
    {
      outputUnicastGate = outputMod[0]->findGate("neighbourDiscoveryDirectIn");
      assert(outputUnicastGate != UINT_MAX);
      if (UINT_MAX == outputUnicastGate)
        DoutFatal(dc::core|error_cf, "Cannot find gate");
    }
  }
}

void AddressResolution::handleMessage(cMessage* msg)
{
  if (msg->isSelfMessage())
  {
    (static_cast<cTimerMessage *> (msg) )->callFunc();
    return;
  }

  if (ICMPIn == msg->arrivalGate()->name())
  {
    switch( (static_cast<ICMPv6Message*> (msg))->type() )
    {
      case ICMPv6_NEIGHBOUR_SOL:
        processNgbrSol(static_cast<IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol*>(msg));
        break;

      case ICMPv6_NEIGHBOUR_AD:
        processNgbrAd(static_cast<IPv6NeighbourDiscovery::ICMPv6NDMNgbrAd*>(msg));
        break;

      default:
        cerr<<rt->nodeName()<<" "<<simTime()<<" "<<className()<<" Unknown message "
            <<(static_cast<ICMPv6Message*> (msg))->type()
            << " forwarded from Neighbour Disc module"<<endl;
        Dout(dc::addr_resln|dc::warning|error_cf, rt->nodeName()<<" "<<simTime()<<" "<<className()
             <<" Unknown message "<<(static_cast<ICMPv6Message*> (msg))->type()
             << " forwarded from Neighbour Disc module");
        assert(false);
        delete msg;
        break;
    }
    return;

  }

  if (pendingQueueIn == msg->arrivalGate()->name())
  {
    IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(msg);
    AddrResInfo *addrResInfo = check_and_cast<AddrResInfo*>(dgram->removeControlInfo());

    assert(dgram->destAddress() != IPv6_ADDR_UNSPECIFIED &&
           addrResInfo->nextHop() != IPv6_ADDR_UNSPECIFIED);

    Dout(dc::addr_resln|flush_cf, rt->nodeName()<<" "<<dec<<setprecision(4)<<simTime()<<" "
         <<": packet queued dest="<<dgram->destAddress()<<" nexthop="
         <<addrResInfo->nextHop());

    //Already done at conceptual sending, necessary only for for promiscous
    //addr res when src addr unknown (handled by processNgbrAdv)
    //dgram->setSrcAddress(
    //  rt->determineSrcAddress(addrResInfo->nextHop(),
    //                          addrResInfo->ifIndex));

    //Store dgram into buffer
    //TODO Implement a limited queue later on that overflows
    ppq.insert(make_pair(addrResInfo->nextHop(), dgram));

    //Check if other dgrams are pending on this addr
    if (ppq.count(addrResInfo->nextHop()) > 1)
      return;

    NDARTimer* tmr = new NDARTimer();
    tmr->targetAddr = addrResInfo->nextHop();

    tmr->ifIndex = addrResInfo->ifIndex();

    boost::weak_ptr<NeighbourEntry> ngbr =
      rt->cds->neighbour(tmr->targetAddr);

    //For mobile nodes AddrRes failure will remove the DE so we need to recreate
    //DE that points to RE if neighbour is router. Can this happen for normal
    //nodes too? (Only if NUD is implemented would DE be removed)
    if (!ngbr.lock())
      if (rt->cds->defaultRouter().lock() &&
          rt->cds->defaultRouter().lock()->addr() == tmr->targetAddr)
      {
        (*rt->cds)[tmr->targetAddr].neighbour = rt->cds->defaultRouter();
        ngbr = rt->cds->defaultRouter();
      }
      else
      {
        //Can we resolve on another router besides the default and if so how to handle?
        assert(!rt->cds->router(tmr->targetAddr).lock());
        (*rt->cds)[tmr->targetAddr].neighbour = rt->cds->router(tmr->targetAddr);
      }

    //Routers in DRL usually not in NC (don't call insertNeighbourEntry) they
    //will be in the DC. However routers determined at run time from redirects
    //not in DRL can be in NC (see Sec. 7.3.3 page 70)
    if (!ngbr.lock())
      ///Prevent erroneous NC entries created for routers that already exist in IRouterList
      ngbr = rt->cds->router(tmr->targetAddr);

    if (!ngbr.lock())
    {
      //Create DE & NE and put to incomplete
      rt->cds->insertNeighbourEntry(new NeighbourEntry(tmr->targetAddr, tmr->ifIndex));
      ngbr = rt->cds->neighbour(tmr->targetAddr);
      Dout(dc::addr_resln, rt->nodeName()<<" "<<dec<<setprecision(4)<<simTime()<<" "
           <<": creating new NC entry for nexthop="<<tmr->targetAddr);
    }

    //Check that targetAddr really is a neighbour
    assert(ngbr.lock().get()->addr() == tmr->targetAddr);

    if (ngbr.lock().get()->state() != NeighbourEntry::INCOMPLETE)
    {
      Dout(dc::warning|flush_cf, rt->nodeName()<<setprecision(6)<<" "<<simTime()
           <<" "<<tmr->ifIndex<<" mismatched ND State="<<ngbr.lock().get()->state()
           <<" for "<<ngbr.lock().get()->addr()<< " "<<*(ngbr.lock().get()));

      //TODO Cancel impending or outstanding addrResln (see processNgbrAdv) but
      //how would ifIndex be set if incorrect?
      assert(false);
      return;
    }



    //Sending solicitation to solicited node multicast dest addr
    tmr->dgram = new IPv6Datagram(dgram->srcAddress(), IPv6Address::solNodeAddr(tmr->targetAddr));
    
    tmr->dgram->setHopLimit(NDHOPLIMIT);

    initiateNgbrSol(tmr);

    return;
  }

}



//The way tmr->ifIndex == UINT_MAX is used and an extra insertion of a timer
//with that marker that does not have a real timer is a hack REIMPLEMENT
void AddressResolution::initiateNgbrSol(NDARTimer* tmr)
{
  if (tmr->ifIndex == UINT_MAX)
  {
    //Don't know which link the node could possibly be on i.e. no router so
    //assume on link but which link? so try them all

    //Insert the tmr with UINT_MAX as a marker for a promiscous addr res (no
    //real timer msg associated with it though)
    tmrs.push_back(tmr);
    for (size_t ifIndex = 0; ifIndex < ift->numInterfaceGates(); ifIndex++)
    {
      NDARTimer* tmrCopy = tmr->dup(ifIndex);
      sendNgbrSol(tmrCopy);
    }
  }
  else
    sendNgbrSol(tmr);

}

void AddressResolution::sendNgbrSol(NDARTimer* tmr)
{
  InterfaceEntry *ie = ift->interfaceByPortNo(tmr->ifIndex);
  unsigned int ifIndex = tmr->ifIndex;
  if (rt->odad() && tmr->dgram->srcAddress() != IPv6_ADDR_UNSPECIFIED
      && ie->ipv6()->tentativeAddrAssigned(tmr->dgram->srcAddress()))
    Dout(dc::addr_resln|dc::warning|dc::custom|flush_cf, rt->nodeName()<<":"<<ifIndex
         <<" ODAD who sent a packet for addr res with a tentative srcAddr of "
         <<tmr->dgram->srcAddress());


  if (tmr->msg == 0)
  {
    if (!ie->ipv6()->addrAssigned(tmr->dgram->srcAddress()))
      if (ie->ipv6()->inetAddrs.size())
      {
        //Use link-local addr
        tmr->dgram->setSrcAddress(ie->ipv6()->inetAddrs[0]);
      }
      else
      {
        //happens when other nodes are ready but our addresses are not
        Dout(dc::warning|error_cf|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
             <<" Cannot find any suitable source address for AddrResln so dropping");
        fc->ctrIP6OutNoRoutes++;
        delete tmr;
        return;
      }

    NS* ns = new NS(tmr->targetAddr, ie->llAddrStr());

    tmr->dgram->encapsulate(ns);
    tmr->dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);
    tmr->dgram->setName(ns->name());

    tmr->msg = new cSignalMessage("AddrReslnTimeout", Tmr_AddrReslnTimeout);
    ((cSignalMessage*) (tmr->msg))->connect(boost::bind(&AddressResolution::sendNgbrSol, this, tmr));

    tmrs.push_back(tmr);

  }

  if (tmr->counter < MAX_MULTICAST_SOLICIT)
  {
    //This step should be done automatically for each call to callFunc
    tmr->counter++;
    send(tmr->dgram->dup(), "outputOut", ifIndex);
    Dout(dc::debug|dc::addr_resln, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
         <<" NS count="<<tmr->counter<<" max="<< MAX_MULTICAST_SOLICIT
         <<" for dest "<<tmr->targetAddr);
    ctrIcmp6OutNgbrSol++;
    rt->ctrIcmp6OutMsgs++;

    scheduleAt(simTime() + ie->ipv6()->retransTimer/1000.0, tmr->msg);
  }
  else
  {
    failedAddrRes(tmr);
  }
}


/**
 * Send ICMP dest unreachable for each queued packet if this is the last addr
 * res timeout (there are multiple if its a promiscous addr res)
 * Remove the Neighbour Entry to reinitiate next hop determination
 */

void AddressResolution::failedAddrRes(NDARTimer* tmr)
{
  ipv6_addr nextHop = tmr->targetAddr;
  bool finalTimer = true;
  NDARTI thisTmr = tmrs.end(), startRemove = tmrs.end();

  for (NDARTI it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ((*it)->targetAddr == tmr->targetAddr)
    {
      if (tmr == *it)
      {
        thisTmr = it;
      }
      else if ((*it)->msg == 0)
      {
        startRemove = it;
        continue;
      }
      else
      {
        finalTimer = false;
        //Found every thing we wanted since finding startRemove should always be
        //before thisTmr unless someones sorted/rearranged the list
        if (thisTmr != tmrs.end())
          break;
      }
    }
  }


  InterfaceEntry *ie = ift->interfaceByPortNo(tmr->ifIndex);

  Dout(dc::addr_resln|dc::notice, rt->nodeName()<<":"<<tmr->ifIndex
       <<" Address Res failed for "<<tmr->targetAddr);

  assert(tmr == *thisTmr);
  delete tmr;
  tmr = 0;
  tmrs.erase(thisTmr);

  //Remove datagrams only if we are the only timer object left for this addr.
  //Needed for promiscuous addr detection.
  if (finalTimer == true)
  {

    Dout(dc::addr_resln|dc::debug, rt->nodeName()<<" "<<dec<<setprecision(4)<<simTime()<<" "
         "Clearing datagrams to/via "<<nextHop<<" and sending ICMP dest unreachable");
    //Remove marker timerMessage only if this is a promiscous addr res As this
    //is before thisTmr the iterator should not be invalidated (hopefully)
    if (startRemove != tmrs.end())
      tmrs.erase(startRemove);



    //remove bad routes only after autoconfiguration period otherwise static routes
    //are deleted too!!
    if (simTime() > 5 ) 
    {
      //remove the DC entry so that subsequent transmissions reinitiate the   
      //nexthop determination 
      rt->cds->removeDestEntryByNeighbour(nextHop);
    }
    //Send the packets in map and remove entries from queue
    pair<PPQI, PPQI> rng = ppq.equal_range(nextHop);

    //rng.first->first has key > nextHop if src not found or if > nextHop
    //doesn't exist then rng.first == ppq.end()
    if (rng.first != ppq.end() && rng.first->first == nextHop)
    {
      for (PPQI p = rng.first; p != rng.second; ++p)
      {
        if (p->second->srcAddress() == IPv6_ADDR_UNSPECIFIED)
        {
          //This must be a local packet destined for unreachable dest on a
          //multihomed host (no src ip address determined yet)

          //Just use any ifaces link local address as src address to notify
          //local process of error

          p->second->setSrcAddress(ie->ipv6()->inetAddrs[0]);
        }

        ICMPv6Message* icmp = createICMPv6Message("destUnreachable",
                                                  ICMPv6_DESTINATION_UNREACHABLE,
                                                  ADDRESS_UNREACHABLE,
                                                  p->second->dup());
        delete p->second;
        send(icmp, "ICMPOut");
        fc->ctrIP6OutNoRoutes++;
      }
      ppq.erase(nextHop);
    }
  }

}

///Proxying for anycast/unicast and anycast addr delay not implemented refer to
///RFC 2461 Sec. 7.2.4
void AddressResolution::processNgbrSol(IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol* thengbrSol)
{
  std::auto_ptr<NS> ngbrSol(thengbrSol);

  //Retrieve details from packet and send off response
  IPv6Datagram* dgram = check_and_cast<IPv6Datagram *>(ngbrSol->encapsulatedMsg());

  assert(dgram->inputPort() > -1 && dgram->inputPort() < (int)ift->numInterfaceGates());
  //Send reply back on this interface
  size_t ifIndex = dgram->inputPort();

  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  bool dupDetectSource = dgram->srcAddress() == IPv6_ADDR_UNSPECIFIED;
  bool sendQueuedPacket = false;
  NeighbourEntry* ne = 0;

  if (!dupDetectSource)
  {
    //Update Ngbr LL addr if necessary
    if (ngbrSol->hasSrcLLAddr())
    {
      ne = rt->cds->neighbour(dgram->srcAddress()).lock().get();
      if (ne == 0)
      {
	if (rt->cds->router(dgram->srcAddress()).lock())
	{
	  (*rt->cds)[dgram->srcAddress()].neighbour = 
	    rt->cds->router(dgram->srcAddress());
	  (*rt->cds)[dgram->srcAddress()].neighbour.lock()->update(ngbrSol.get());
	  //If outstanding addrResln, will need to cancel?  Actually should not
	  //be required as ngbrSol cannot change NE state unless LLaddr
	  //differs (also does not set ifIndex)
	  assert(!ppq.count(dgram->srcAddress()));
	  ne = (*rt->cds)[dgram->srcAddress()].neighbour.lock().get();
	}
	else
	{
	  ne = new NeighbourEntry(ngbrSol.get());
	  rt->cds->insertNeighbourEntry(ne);
	}
      }
      else
      {
	if (ne->state() == NeighbourEntry::INCOMPLETE)
	  sendQueuedPacket = true;
        ne->update(ngbrSol.get());
      }
    }
  }


  //Reply with NA on interface that received NS
  //According to NDisc 7.2.3 reply only if address is assigned on receiving
  //interface
  IPv6Datagram* response = 0;
  if (ie->ipv6()->addrAssigned(ngbrSol->targetAddr()))
    response = new IPv6Datagram(ngbrSol->targetAddr(), dgram->srcAddress());

  bool optimistic = false;
  if (rt->odad() && !response && ie->ipv6()->tentativeAddrAssigned(ngbrSol->targetAddr()))
  {
    response = new IPv6Datagram(ngbrSol->targetAddr(), dgram->srcAddress());
    optimistic = true;
  }

  if (!response)
  {
    Dout(dc::addr_resln|dc::notice|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<dec
         <<setprecision(4)<<simTime()<<" Received an Addr Resolution NS targeted at "
         <<ngbrSol->targetAddr()<<" with no matching address assigned");
    return;
  }

    Debug(response->setName("addrRes2"));
    response->setHopLimit(NDHOPLIMIT);

    if (dupDetectSource)
      response->setDestAddress(IPv6Address(ALL_NODES_LINK_ADDRESS));

    bool override = true;

    if (rt->odad())
    {
      override = !optimistic;
      if (optimistic)
        Dout(dc::custom, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
             <<" ODAD optimistic true and override is "<<(override?"true":"false"));
    }
    else
    {
      Dout(dc::neighbour_disc|dc::addr_resln|dc::debug|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<dec
           <<setprecision(6)<<simTime()<<" sending NA in response to NS");
    }

    //Always send LL regardless of whether solication was unicast or multicast
    //In future if solication is not multicast can omit the lladdr and set
    //override to false
    NA* na = new NA(ngbrSol->targetAddr(), ie->llAddrStr(), rt->isRouter(),
                    !dupDetectSource, override);

    response->encapsulate(na);
    response->setTransportProtocol(IP_PROT_IPv6_ICMP);
    response->setName(na->name());

    if (dupDetectSource)
      send(response, "outputOut", ifIndex);
    else
    {
      //if targetAddr is anycast addr then delay by 0 and
      //MAX_ANYCAST_DELAY_TIME

      //Send unicast response using srcLLAddr option which must be included if
      //its a valid addr res request.
      assert(ngbrSol->hasSrcLLAddr());

      //Addr res can not use unicast.  These are most likely NUD messages
      assert(!ngbrSol->targetAddr().isMulticast());

      LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
      ctrlInfo->setDestLLAddr(ngbrSol->srcLLAddr().c_str());
      response->setControlInfo(ctrlInfo);
      sendDirect(response, 0, outputMod[ifIndex], outputUnicastGate); // XXX why sendDirect? -AV

      //Using direct sending for unicast ND messages just like FAST RA

      ctrIcmp6OutNgbrAdv++;
      rt->ctrIcmp6OutMsgs++;

    }

  if (sendQueuedPacket)
  {
    sendQueuedPackets(dgram->srcAddress(), ifIndex, ne);
  }
}

void AddressResolution::sendQueuedPackets(const ipv6_addr& src,
					  unsigned int ifIndex,
					  IPv6NeighbourDiscovery::NeighbourEntry* ne)
{
  //Send the packets in map and remove entries from queue
  pair<PPQI, PPQI> rng = ppq.equal_range(src);

  //rng.first->first has key > src if src not found or if > src doesn't
  //exist then rng.first = ppq.end()
  if (rng.first != ppq.end() && rng.first->first == src)
  {
    for (PPQI p = rng.first; p != rng.second; ++p)
    {
      p->second->setOutputPort(ifIndex);

      //This lookup may not be necessary for every pending packet if the
      //destination is the same as the next hop or if all packets are going
      //to the same destination. However these cases are not typical
      if (p->second->srcAddress() == IPv6_ADDR_UNSPECIFIED)
      {
	p->second->setSrcAddress(
				 fc->determineSrcAddress(
							 p->second->destAddress(), ifIndex));
	if (p->second->srcAddress() == IPv6_ADDR_UNSPECIFIED)
	{
	  cerr<<rt->nodeName()<<" "<<simTime()<<" "<<className()<<" No suitable src Address for destination "
	      <<p->second->destAddress()<<endl;
	  Dout(dc::addr_resln|dc::notice|flush_cf, rt->nodeName()<<":"<<ifIndex
	       <<" No suitable src Address for destination "<<p->second->destAddress());

	  //TODO probably send error message about -EADDRNOTAVAIL
	  delete p->second;
	  continue;
	}
	if (rt->odad())
	{
	  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
	  if (ie->ipv6()->tentativeAddrAssigned(p->second->srcAddress()))
	    Dout(dc::warning|dc::addr_resln|dc::custom|flush_cf, rt->nodeName()<<":"<<ifIndex
		 <<" ODAD we're not supposed to do AddrResln on packet"
		 <<" that would have been sent from tentative addr even though we do"
		 <<" addrResln using different srcAddr?");
	}
      }

      IPv6Datagram *dgram = p->second;
      AddrResInfo *info = new AddrResInfo;
      info->setNextHop(src);
      info->setIfIndex(ifIndex);
      assert(ne->linkLayerAddr() != "");
      info->setLinkLayerAddr(ne->linkLayerAddr().c_str());
      dgram->setControlInfo(info);

      send(dgram, "fragmentationOut");
    }
    Dout(dc::addr_resln|dc::notice, rt->nodeName()<<":"<<ifIndex
	 <<" Pending Packets sent to "<<src<<" as result of changing state from"
	 <<" INCOMPLETE to "<<ne->state());
    ppq.erase(src);
  }
}

void AddressResolution::processNgbrAd(IPv6NeighbourDiscovery::ICMPv6NDMNgbrAd* thengbrAdv)
{

  std::auto_ptr<NA> ngbrAdv(thengbrAdv);

  //Read in new LL addr and test if ifIndex
  IPv6Datagram* dgram = check_and_cast<IPv6Datagram *>(ngbrAdv->encapsulatedMsg());

  ipv6_addr src = dgram->srcAddress();
  int ifIndex = dgram->inputPort();  

  //Update the DC in RoutingTable6
  //Check if the NE exists already before creating a new one
  NeighbourEntry* ne = rt->cds->neighbour(src).lock().get();
  if (!ne)
  {
    Dout(dc::addr_resln|dc::notice, rt->nodeName()<<":"<<ifIndex
	 <<" Didn't initiate comm so don't care about NA from "<<src
	 <<" to "<< ngbrAdv->targetLLAddr());
    return;
  }
  else
    if (ne->update(ngbrAdv.get()))
    {
      Dout(dc::addr_resln|dc::notice, rt->nodeName()<<":"<<ifIndex
	   <<" NA from "<<src<<" in response to our NS "<<src);

      // Explcitely update the interface index in the ne
      ne->setIfIndex(ifIndex);


      sendQueuedPackets(src, ifIndex, ne);


      Dout(dc::addr_resln|flush_cf, rt->nodeName()<<":"<<ifIndex
           <<" searching timer for "<<src<<" size="<<tmrs.size());
      //Cancel the addr Res timer/s and remove from list
      bool continueSearch = false, deleted = false;
      NDARTI startRemove = tmrs.end();
      NDARTI it;
      for (it = tmrs.begin(); it != tmrs.end(); it++)
      {
        Dout(dc::addr_resln|flush_cf, "it->targetAddr="
             <<(*it?ipv6_addr_toString((*it)->targetAddr):"null"));
        assert(*it != 0);

        if (*it && (*it)->targetAddr == src)
        {
          ///NA was in response to Addr Res on all Ifaces - only the first tmr
          ///has UINT_MAX
          if ((*it)->ifIndex == UINT_MAX || continueSearch)
          {
            if (!continueSearch)
            {
              continueSearch = true;
              startRemove = it;
              assert((*it)->msg == 0);
              Dout(dc::addr_resln|flush_cf, " Start of promiscuous addrres");
            }
            else //Only ones not marked with UINT_MAX actually have a real timer
            {
              Dout(dc::addr_resln|flush_cf, " One timer deleted for prom case");
              cancelEvent((*it)->msg);
            }

            delete *it;
            *it = 0;
          }
          else if (!continueSearch)
          {
            Dout(dc::addr_resln|flush_cf, " Normal timer deleted removed");
            cancelEvent((*it)->msg);
            delete *it;
            tmrs.erase(it);
            return;
          }
          deleted = true;
        }
        else if (deleted)
          break;
      }

      //Addr res on all ifaces should be consecutive elements in list
      //Remove timers from list (deleted in the previous iterations already)
      tmrs.erase(startRemove, it);
      return;
    }
    else 
    {
      Dout(dc::addr_resln|dc::notice, rt->nodeName()<<":"<<ifIndex
	   <<" Not an addr Res response from "<<src);
      return;
    }

  //Msg has to exist in tmrs when addrRes was successful
  assert(false);

}

template <class ForwardIterator>
void sequence_delete(ForwardIterator first, ForwardIterator last)
{
  while (first != last)
    delete *first++;
}

template <class ForwardIterator>
void map_delete(ForwardIterator first, ForwardIterator last)
{
  //  for (;first != last; first++)
  //  delete first->second;
  sequence_delete( (*first).second.begin(), (*first).second.end());
}

void AddressResolution::finish()
{
  // XXX cleanup stuff must be moved to dtor!
  // finish() is NOT for cleanup -- that must be done in destructor, and
  // additionally, ptrs must be NULL'ed in constructor so that it won't crash
  // if initialize() hasn't run because of an error during startup! --AV
  map_delete(ppq.begin(), ppq.end());
  for (NDARTI it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ((*it)->msg->isScheduled())
      cancelEvent((*it)->msg);
    delete *it;
  }
  tmrs.clear();

  delete [] outputMod;
  outputMod = 0;

  recordScalar("Icmp6OutNgbrAdv", ctrIcmp6OutNgbrAdv);
  recordScalar("Icmp6OutNgbrSol", ctrIcmp6OutNgbrSol);

}

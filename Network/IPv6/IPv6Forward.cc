//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
    @file IPv6Forward.cc
    @brief Forwarding of packets based on next hop prescribed by RoutingTable6

    -Responsibilities:
        -Receive valid IPv6 datagram
        -send datagram with Multicast addr. to Multicast module
        -Forward to IPv6Encapsulation module if next hop is a tunnel entry point
        -Drop datagram away and notify ICMP if dest. addr not in routing table
        -send to local Deliver if dest. addr. = 127.0.0.1
         or dest. addr. = NetworkCardAddr.[]
         otherwise, send to Fragmentation module
        -Dec Hop Limit when forwarding

    @author Johnny Lai
    @date 28/08/01
*/


#include "sys.h"
#include "debug.h"
#include "config.h"

#include <iomanip> //setprecision
#include <iterator> //ostream_iterator
#include <omnetpp.h>
#include <boost/cast.hpp>
#include <boost/functional.hpp>

#include "IPv6Forward.h"
#include "RoutingTable6.h"
#include "IPv6Datagram.h"
#include "ICMPv6Message.h"
#include "opp_utils.h"
#include "NDEntry.h"
#include "AddrResInfo_m.h"
#include "HdrExtRteProc.h"
#include "Interface6Entry.h"
#include "IPv6CDS.h"

#ifdef USE_MOBILITY
#include "MIPv6CDSMobileNode.h"
#include "MIPv6Entry.h"
#include "HdrExtDestProc.h"
#include "MIPv6DestOptMessages.h"
#include "MIPv6MNEntry.h" //bu_entry
#include "IPv6Encapsulation.h"
#include "MIPv6MessageBase.h"
#ifdef USE_HMIP
#include "HMIPv6CDSMobileNode.h"
#endif //USE_HMIP
#endif //#ifdef USE_MOBILITY

///For redirect. Its overkill but sendDirect would still require many mods too
///in terms of thinking how to pass a dgram in when it expects all to be
///icmp messages
#include "NDStateRouter.h"
#include "NeighbourDiscovery.h"

#include "opp_utils.h"  // for int/double <==> string conversions
#ifdef CWDEBUG
#include <cassert>
#include <iostream>
#include <string>
#include <memory>
#endif //CWDEBUG

using boost::weak_ptr;

Define_Module( IPv6Forward );


void IPv6Forward::initialize(int stage)
{
  if (stage == 0)
  {
    QueueBase::initialize();
    rt = RoutingTable6Access().get();
    ctrIP6InAddrErrors = 0;
    ctrIP6OutNoRoutes = 0;

    routingInfoDisplay = par("routingInfoDisplay");
#ifdef USE_MOBILITY
    tunMod = check_and_cast<IPv6Encapsulation*>
      (OPP_Global::findModuleByName(this, "tunneling")); // XXX try to get rid of pointers to other modules --AV
    assert(tunMod != 0);
#endif //USE_MOBILITY
  }
  //numInitStages needs to be after creation of ND state in NeighbourDiscovery
  //which happens at 3rd stage
  else if (stage == numInitStages() - 1 && rt->isRouter())
  {
    //Get the nd pointer to ndstaterouter/host pointer
    cModule* procMod = parentModule();  // XXX try to get rid of pointers to other modules --AV
    cModule* icmp = procMod->submodule("ICMP");
    assert(icmp);
    cModule* ndMod = icmp->submodule("nd");
    assert(ndMod);
    NeighbourDiscovery* ndm = check_and_cast<NeighbourDiscovery*>(ndMod);
    assert(ndm);
    if (!ndm)
      DoutFatal(dc::core|error_cf, "Cannot find ND module");
    nd = boost::polymorphic_downcast<IPv6NeighbourDiscovery::NDStateRouter*>(ndm->getRouterState());
    assert(nd);
    if (!nd)
      DoutFatal(dc::core|error_cf, "Cannot find NDStateRouter module");
  }
}

/**
   @class printRoutingInfo

   @brief display some information when routingInfoDisplay parameter is set for
   the IPv6Forward module.
*/

struct printRoutingInfo
{
  printRoutingInfo(bool routingInfoDisplay, IPv6Datagram* dgram, const char* name):display(routingInfoDisplay),datagram(dgram),name(name)
    {}

  ///Display information only when IPv6Forward::handleMessage() returns
  ~printRoutingInfo()
    {
      if (display)
      {
        cout<<name<<" "<<simulation.simTime()<<" src="<<datagram->srcAddress()<<" dest="
            <<datagram->destAddress()<<" len="<<datagram->length()<<endl;
      }
    }
  bool display;
  IPv6Datagram* datagram;
  const char* name;
};

/**
   @brief huge function to handle forwarding of datagrams to lower layers

   @todo When src of outgoing dgram is a tentative address, ODAD would forward
   dgram to router for unknown neighbour LL addr.
   @todo Multicast module will have to handle MIPv6 11.3.4 forwarding of multicast packets
*/

void IPv6Forward::endService(cMessage* theMsg)
{
  std::auto_ptr<IPv6Datagram> datagram(check_and_cast<IPv6Datagram*> (theMsg));

  assert(datagram->inputPort() < (int)rt->interfaceCount());

  //Disabled for now. Don't know why some packets when they leave this function
  //still display 0 source address when they shouldn't. Recording at prerouting
  //and output
  //printRoutingInfo(routingInfoDisplay, datagram.get(), rt->nodeName());
  if (rt->localDeliver(datagram->destAddress()))
  {
    Dout(dc::forwarding|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()<<" "
         <<simTime()<<" received packet from "<<datagram->srcAddress()
         <<" dest="<<datagram->destAddress());

    //This condition can occur when upper layer originates packets without
    //specifying a src address destined for a multicast destination or local
    //destination so the packet is missing the src address. This is a bit
    //dodgy but the only other solution would be to enforce the app layer to
    //choose a src address.
    IPv6Datagram* copy = datagram->dup();
    if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED &&
        datagram->inputPort() == -1)
      copy->setSrcAddress(rt->getInterfaceByIndex(0).inetAddrs[0]);

    send(copy, "localOut");
    //Check if it is a multicast packet that we need to forward
    if (!datagram->destAddress().isMulticast())
      return;
  }

#if !defined NOINGRESSFILTERING
  //Cannot use the result of conceptual sending i.e. ifIndex because at that
  //stage dgrams with link local addresses are dropped.  Thus this disables
  //a form of "IP Masq" via tunneling at the gateways.  Otherwise this check
  //is not required here at all and a test for vIfIndex from result of
  //conceptual sending is sufficient to trigger a tunnel.
  IPv6NeighbourDiscovery::IPv6CDS::DCI it;
  // ifIndex is the virtual tunnel interface to send on.
  if (rt->cds->findDestEntry(datagram->destAddress(), it))
  {
    NeighbourEntry* ne = it->second.neighbour.lock().get();
    Dout(dc::debug|dc::encapsulation|dc::forwarding|flush_cf, rt->nodeName()
         <<" possible tunnel index="<<(ne?
                                       boost::lexical_cast<std::string>(ne->ifIndex()):
                                       "No neighbour")
         <<(ne?
            std::string(" state=") + boost::lexical_cast<std::string>(ne->state()):
            "")
         <<" dgram="<<*datagram);

    //-1 is used to indicate promiscuous addr res so INCOMPLETE test
    //required
    if (ne && ne->state() != NeighbourEntry::INCOMPLETE &&
        ne->ifIndex()  > rt->interfaceCount() )
    {
      Dout(dc::encapsulation|dc::forwarding|dc::debug|flush_cf, rt->nodeName()
           <<" Found tunnel vifIndex ="<<hex<<it->second.neighbour.lock().get()->ifIndex()
           <<dec);
      IPv6Datagram* copy = datagram->dup();
      copy->setOutputPort(ne->ifIndex());
      send(copy, "tunnelEntry");

      if (!datagram->destAddress().isMulticast())
        return;
    }
  }
#endif //!NOINGRESSFILTERING

  insertSourceRoute(*datagram);

  //Packet was received
  if (datagram->inputPort() != -1 )
  {
    if (!processReceived(*datagram))
      return;

    Dout(dc::debug|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
         <<" forwarding packet "<<*datagram);
  }//End if packet was received


  if (datagram->destAddress().isMulticast())
  {
    send(datagram.release(), "multicastOut");
    return;
  }

#ifdef USE_MOBILITY
  //If we have a binding for dest then swap dest==hoa to coa
  //Could possibly place this into sendCore
  using MobileIPv6::MIPv6DestinationEntry;
  using MobileIPv6::bc_entry;
  using MobileIPv6::MIPv6RteOpt;

  weak_ptr<bc_entry> bce;

  Dout(dc::mipv6, rt->nodeName()<<" datagram->inputPort = " << datagram->inputPort());

  if (rt->mobilitySupport() && datagram->inputPort() == -1)
  {
    // In simultaneous movement, it is possible that the mobility
    // messages are sent in reverse tunnel. Therefore, we need to
    // check if the packet is encapsulated with another IPv6 header.

    MobileIPv6::MIPv6MobilityHeaderBase* ms = 0;
    IPv6Datagram* tunPacket = false;
    if (datagram->transportProtocol() == IP_PROT_IPv6)
      tunPacket = check_and_cast<IPv6Datagram*>(datagram->encapsulatedMsg());

    if (tunPacket && tunPacket->transportProtocol() == IP_PROT_IPv6_MOBILITY)
        ms = check_and_cast<MobileIPv6::MIPv6MobilityHeaderBase*>(
          tunPacket->encapsulatedMsg());
    else if (datagram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
      ms = check_and_cast<MobileIPv6::MIPv6MobilityHeaderBase*>(datagram->encapsulatedMsg());

    // no mobility message is allowed to have type 2 rh except BA
    if (ms == 0 || (ms && ms->header_type() ==  MobileIPv6::MIPv6MHT_BA))
    {
      IPv6NeighbourDiscovery::IPv6CDS::DCI it;
      if (rt->cds->findDestEntry(datagram->destAddress(), it))
      {
        bce = it->second.bce;
        if (bce.lock().get() != 0)
        {
          assert(bce.lock()->home_addr == datagram->destAddress());
          datagram->setDestAddress(bce.lock()->care_of_addr);

          HdrExtRteProc* rtProc = datagram->acquireRoutingInterface();
          //Should only be one t2 header per datagram (except for inner
          //tunneled packets)
          assert(rtProc->routingHeader(IPv6_TYPE2_RT_HDR) == 0);
          MIPv6RteOpt* rt2 = new MIPv6RteOpt(bce.lock()->home_addr);
          rtProc->addRoutingHeader(rt2);
//#if defined TESTMIPv6 || defined DEBUG_BC
          Dout(dc::mipv6, rt->nodeName()<<" Found binding for destination "
               <<bce.lock()->home_addr<<" swapping to "<<bce.lock()->care_of_addr);
        }
      }
    }
  }
#endif //USE_MOBILITY

  Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<" "<<dec<<setprecision(4)<<simTime()
       <<" sending/forwarding dgram="<<*datagram);

  AddrResInfo *info = new AddrResInfo;
  int status = conceptualSending(datagram.get(), info); // fills in control info
  datagram->setControlInfo(info);

  // destination address does not exist in routing table:
  // Can this ever happen unless there is no default router?

  // If no router, Assume dest is onlink according to Sec 5.2 Conceptual
  // sending algorithm. Do address resolution on all ifaces.

  // or
  // packet awaits addr res so put into queue
  if (status == -1 || status == -2)
  {
    Dout(dc::forwarding|flush_cf," "<<rt->nodeName()<<":"<<info->ifIndex()<<
         simTime()<<" "<<className()<<": addrRes pending packet dest="
         <<datagram->destAddress()<<" nexthop="<<info->nextHop()<<" status"<<status);

    if (rt->odad())
    {
      bool foundTentative = false;
      for (unsigned int i = 0; i < rt->interfaceCount(); i++)
      {
        Interface6Entry& ie = rt->getInterfaceByIndex(i);
        if ((datagram->srcAddress() != IPv6_ADDR_UNSPECIFIED &&
             ie.tentativeAddrAssigned(datagram->srcAddress()))
#ifdef USE_MOBILITY
            || (rt->mobilitySupport() && rt->isMobileNode() && rt->awayFromHome() &&
                ie.tentativeAddrAssigned(
                  boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>
                  (rt->mipv6cds)->careOfAddr(true)))
#endif //USE_MOBILITY
            )
        {
          //forward via router
          Dout (dc::custom|dc::forwarding|flush_cf, rt->nodeName()<<":"<<simTime()
                <<" ODAD would forward to router for unknown neighbour LL addr. (once I get more time)");
          delete info;
          foundTentative = true;
          return;
        }
      }
    }

    send(datagram.release(), "pendingQueueOut");
    return;
  }

  //Required to enable triggering of prefix addresses instead of just a host
  //match as done via rt->cds->findDestEntry earlier in this function
  if ((unsigned int)status > rt->interfaceCount())
  {
    Dout(dc::encapsulation|dc::forwarding|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
         <<" vIfindex="<<hex<<status<<dec<<" dgram="<<*datagram);
    datagram->setOutputPort(info->ifIndex());
    delete datagram->removeControlInfo();
    send(datagram.release(), "tunnelEntry");
    return;
  }


  //Set src address here if upper layer protocols left it up
  //to network layer
  if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
    datagram->setSrcAddress(determineSrcAddress(
                              datagram->destAddress(), info->ifIndex()));
  }


#ifdef USE_MOBILITY
  //Check to see if we have sent a BU to the peer (bule exists) and if so use a hoa dest
  //opt and send packet via route optimised path. Otherwise send via reverse tunnel to HA

  //Perhaps this goes into send too as only for non forwarded packets. But would
  //need the ifIndex hint unless we assumed single interface (which some other places in code do)
  bool pcoa = false;
  //Process Datagram according to MIPv6 Sec. 11.3.1 while away from home
  if (rt->mobilitySupport() && rt->isMobileNode() &&
      boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
      ->awayFromHome() &&
      boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
      ->primaryHA().get() != 0 &&
      datagram->srcAddress() ==
      boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
      ->homeAddr())
  {
    MobileIPv6::MIPv6CDSMobileNode* mipv6cdsMN =
      boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds);

    bool coaAssigned = false;
    if (mipv6cdsMN->currentRouter().get() != 0 &&
        rt->getInterfaceByIndex(info->ifIndex()).addrAssigned(
          mipv6cdsMN->careOfAddr(pcoa))
        ||
        (rt->odad() &&
         //for tentative true is a misnomer because the newcoa should have been
         //assigned to tentative addr anyway
         rt->getInterfaceByIndex(info->ifIndex()).tentativeAddrAssigned(
           mipv6cdsMN->careOfAddr(pcoa))
         )
        )
      coaAssigned = true;

    // In simultaneous movment, there are times when the MN has the
    // care-of address of the CN in its binding cache entry. Since the
    // dest address of the packet got swapped to the care-of address
    // from the home address of the CN, it is important that we use
    // home address as a key to obtain the binding update

    MobileIPv6::bu_entry* bule = 0;

    if (bce.lock().get() != 0)
      bule = mipv6cdsMN->findBU(bce.lock()->home_addr);
    else
      bule = mipv6cdsMN->findBU(datagram->destAddress());

    if (bule)
      Dout(dc::debug|flush_cf, "bule "<<*bule);

    // The following section of the code only applies to data packet
    // sent from upper layer. The mobility messages do not contain the
    // home address option TODO: maybe an extra check of where the
    // message is sent should be done?

    MobileIPv6::MIPv6MobilityHeaderBase* ms = 0;
    if (datagram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
      ms = check_and_cast<MobileIPv6::MIPv6MobilityHeaderBase*>(datagram->encapsulatedMsg());
    if (ms == 0 && bule && !bule->isPerformingRR &&
        bule->homeAddr() == datagram->srcAddress() &&
        coaAssigned && mipv6cdsMN->careOfAddr(pcoa) == bule->careOfAddr() &&
        //state 0 means ba received or assumed to be received > 0 means
        //outstanding BUs
        (!bule->problem && bule->state == 0) && bule->expires() > 0)
    {
      //Do route optimisation when BU to cn exists already
      HdrExtDestProc* destProc = datagram->acquireDestInterface();
      //Should only ever have one home addr destOpt.
      assert(destProc->getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT)
             == 0);

      bool test = destProc->addOption(
        new MobileIPv6::MIPv6TLVOptHomeAddress(datagram->srcAddress()));
      assert(test);
      datagram->setSrcAddress(mipv6cdsMN->careOfAddr(pcoa));
//#if defined TESTMIPv6 || defined DEBUG_DESTHOMEOPT
      Dout(dc::mipv6, rt->nodeName()<<" Added homeAddress Option "
           <<check_and_cast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
           ->homeAddr()<<" src addr="<<
           check_and_cast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
           ->careOfAddr()<<" for destination "<<datagram->destAddress());

      bool docheck = false;
#ifdef USE_HMIP
      if (rt->mobilitySupport() && rt->hmipSupport())
      {
        docheck = true;
        HierarchicalMIPv6::HMIPv6CDSMobileNode* hmipv6cdsMN =
          boost::polymorphic_downcast<HierarchicalMIPv6::HMIPv6CDSMobileNode*>(rt->mipv6cds);

        Interface6Entry& ie = rt->getInterfaceByIndex(0);

        if (hmipv6cdsMN->isMAPValid())
        {
            //This is too restrictive especially when EdgeHandover is on no
            //packets go out until we update HA
            //mipv6cdsMN->careOfAddr(pcoa) != hmipv6cdsMN->remoteCareOfAddr())
          if (!ie.addrAssigned(hmipv6cdsMN->remoteCareOfAddr()))
          {
            Dout(dc::hmip, " rcoa "<<hmipv6cdsMN->remoteCareOfAddr()
                 <<" from new MAP not ready yet(awaiting BA from MAP), dropping packet");
            delete info;
            return;
          }

          if (hmipv6cdsMN->remoteCareOfAddr() == datagram->srcAddress())
          {
            docheck = false;

            //Reverse tunnel to map always in latest hmip draft. reverse tunnels
            //when not doing RO because we set a dest trigger on HA so that it
            //tunnels automatically from MN->MAP.
            if (hmipv6cdsMN->currentMap().v())
            {
              size_t vIfIndex = tunMod->findTunnel(hmipv6cdsMN->localCareOfAddr(), hmipv6cdsMN->currentMap().addr());
              assert(vIfIndex);

              Dout(dc::hmip|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()
                   <<" reverse tunnelling to MAP vIfIndex="<<hex<<vIfIndex<<dec
                   <<" for dest="<<datagram->destAddress());
              IPv6Datagram* copy = datagram->dup();
              copy->setOutputPort(vIfIndex);
              send(copy, "tunnelEntry");
              delete info;
              return;
            }
          }
          else
          {
            unsigned int ifIndexTest;
            //outgoing interface (ifIndex) MUST have src addr (care of Addr) as on
            //link prefix
            if (!rt->cds->lookupAddress(datagram->srcAddress(), ifIndexTest))
            {
              Dout(dc::eh, rt->nodeName()<<" "<<simTime()<<" dgram dropped as src="
                   <<datagram->srcAddress()<<" is no longer onlink (BU to HA not sent "
                   <<"yet so coa is not current rcoa)");
              delete info;
              return;
            }
          }
        }



      }
#endif
      if (docheck)
      {
        Dout(dc::forwarding|flush_cf, rt->nodeName()<<" "<<simTime()
             <<" checking coa "<<datagram->srcAddress()
             <<" is onlink at correct ifIndex "<<info->ifIndex());
        unsigned int ifIndexTest;
        //outgoing interface (ifIndex) MUST have src addr (care of Addr) as on
        //link prefix
        assert(rt->cds->lookupAddress(datagram->srcAddress(), ifIndexTest));
        assert(ifIndexTest == info->ifIndex());
      }

    }
    else if (coaAssigned)
    {
      //Reverse Tunnel
      size_t vIfIndex = tunMod->findTunnel(mipv6cdsMN->careOfAddr(pcoa),
                                           mipv6cdsMN->primaryHA()->prefix().prefix);
#ifndef USE_HMIP
      assert(vIfIndex);
#else
      if (rt->hmipSupport() && !vIfIndex)
      {
        Dout(dc::mipv6, " reverse tunnel to HA not found as coa="<<mipv6cdsMN->careOfAddr(pcoa)
             <<" is the old one and we have handed to new MAP, awaiting BA "
             <<"from HA to use rcoa, dropping packet");
        delete info;
        return;
      }
#endif //USE_HMIP

      Dout(dc::mipv6|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()
           <<" reverse tunnelling vIfIndex="<<hex<<vIfIndex<<dec
           <<" for dest="<<datagram->destAddress());
      IPv6Datagram* copy = datagram->dup();
      copy->setOutputPort(vIfIndex);
      send(copy, "tunnelEntry");
      delete info; // XXX CRASH CRASH CRASH
      return;
    }
    else
    {
      if (boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
          ->currentRouter().get() == 0)
        //FMIP just set to PCoA here and hope for the best right.
        Dout(dc::forwarding|dc::mipv6, rt->nodeName()<<" "<<simTime()
             <<" No suitable src address available on foreign network as no "
             <<"routers recorded so far to form coa");
      else
        Dout(dc::forwarding|dc::mipv6, rt->nodeName()<<" "<<simTime()
             <<" No suitable src address available on foreign network as "
             <<"ncoa in dad "<<
             check_and_cast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
             ->careOfAddr(false)
             );
      datagram->setSrcAddress(IPv6_ADDR_UNSPECIFIED);
    }

  }
#endif //USE_MOBILITY

  if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
#ifndef USE_MOBILITY
    Dout(dc::warning, rt->nodeName()<<" "<<className()<<" No suitable src Address for destination "
         <<datagram->destAddress());
#endif //USE_MOBILITY
    ctrIP6OutNoRoutes++;
    //TODO probably send error message about -EADDRNOTAVAIL

    delete info;
    return;
  }


  //Unicast dest addr with dest LL addr available in DC -> NE
  assert(datagram->srcAddress() != IPv6_ADDR_UNSPECIFIED &&
         !datagram->srcAddress().isMulticast());

  // default: send datagram to fragmentation
  datagram->setOutputPort(info->ifIndex());

  if (rt->odad())
  {
    ///Check for forwarding to on link neighbour and send back a redirect to host
    if (datagram->inputPort() != IMPL_INPUT_PORT_LOCAL_PACKET && info->ifIndex() == (unsigned int)datagram->inputPort() &&
        datagram->destAddress() == info->nextHop())
    {
      bool redirected = false;
      //Enter_Method does not work here :(
      nd->sendRedirect(datagram.release(), info->nextHop(), redirected);
      Dout(dc::custom|dc::forwarding|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
           <<" "<<simTime()<<" sent redirect to ODAD node "<<datagram->srcAddress());
      if (redirected)
      {
        delete info;
        return;
      }
    }
  }

  send(datagram.release(), "fragmentationOut");
}

void IPv6Forward::finish()
{
  recordScalar("IP6InAddrErrors", ctrIP6InAddrErrors);
  recordScalar("IP6OutNoRoutes", ctrIP6OutNoRoutes);
}


std::ostream& operator<<(std::ostream & os,  IPv6Forward& routeMod)
{
  for (IPv6Forward::SRI it = routeMod.routes.begin();
       it != routeMod.routes.end(); it++)
  {
    os << "Source route to dest "<<it->first<<" via intermediate hop"<<endl;
    copy(it->second->begin(), it->second->end(), ostream_iterator<
         _SrcRoute::value_type>(os, " >>\n"));
//    os << *(it->second.end()-1)<<endl;
  }
  return os;
}

///Route is searched according to final dest (last address in SrcRoute).  The
///src address of packet is used to determine outgoing iface.
void IPv6Forward::addSrcRoute(const SrcRoute& route)
{
  routes[*route->rbegin()] = route;
}

///Thought I almost had to do this.  If there is no supports for traits on
///compiler will have to resort to using this
struct addAddress : public unary_function<ipv6_addr, void>
{
  addAddress(HdrExtRte* obj) : mem(obj) {}
  void operator() (const ipv6_addr& x) { mem->addAddress(x); }
  HdrExtRte* mem;
};

/**
   Based on Conceptual Sending Algorithm described in Sec. 5.2 of RFC2461

   info.outputPort = UINT_MAX;
   Notify address resolution to occur on all interfaces i.e. don't know
   which link this address could be on
*/

int IPv6Forward::conceptualSending(IPv6Datagram *dgram, AddrResInfo *info)
{
  // Conceptual Sending Algorithm

  weak_ptr<NeighbourEntry> ne;

  if (rt->isRouter())
    ne = rt->cds->lookupDestination(dgram->destAddress());
  else
    ne = rt->cds->neighbour(dgram->destAddress());

  if (ne.lock().get() == 0)
  {
    // Next Hop determination
    unsigned int tmpIfIndex = info->ifIndex();
    if (rt->cds->lookupAddress(dgram->destAddress(),tmpIfIndex))
    {
      info->setIfIndex(tmpIfIndex);
      // destination address of the packet is onlink

      //Do Address Resolution from this interface (info->ifIndex())
      info->setNextHop(dgram->destAddress());
      return -2;
    }
    // destination address of the packet is offlink
    else
    {
      info->setIfIndex(tmpIfIndex);
      ne = rt->cds->defaultRouter();

      // check to see if the router entry exists
      if(ne.lock().get() != 0)
      {
        //Save this route to dest in DC
        (*rt->cds)[dgram->destAddress()].neighbour = ne;
        Dout(dc::forwarding, " Using default router addr="<<ne.lock()->addr()<<" for dest="
             <<dgram->destAddress());
        if (ne.lock()->addr() == dgram->srcAddress())
        {
          cout<< rt->nodeName()<<" default router of destination points back to source! "
              <<*(static_cast<IRouterList*>(rt->cds))<<endl;
          Dout(dc::warning, rt->nodeName()<<" default router of destination points back to source! "
               <<*(static_cast<IRouterList*>(rt->cds)));
        }
      }
      else
      {

        if (rt->interfaceCount() > 1)
          //Signify to addr res to occur on all interfaces
          info->setIfIndex(UINT_MAX);
        else
          info->setIfIndex(0);

        info->setNextHop(dgram->destAddress());

        Dout(dc::forwarding, "No default router assuming dest="<<dgram->destAddress()
             <<" is on link."<<"Performing "
             <<(info->ifIndex() != 0?
                "promiscuous addr res":
                "plain addr res on single iface=")<<info->ifIndex());

        // no route to dest -1 (promiscuous addr res) or do plain addr res -2
        return info->ifIndex() != 0?-1:-2;
      }

    }
  }
  else if (!rt->isRouter())
    Dout(dc::forwarding, " Found dest "<<dgram->destAddress()<<" in Dest Cache next hop="
         <<ne.lock()->addr());

  //Assume neighbour is reachable and use precached info
  info->setNextHop(ne.lock().get()->addr());
  info->setIfIndex(ne.lock().get()->ifIndex());

  //Neighbour exists check state that neighbour is in
  if (ne.lock().get()->state() == NeighbourEntry::INCOMPLETE)
    //Pass dgram to addr resln to queue pending packet
    return -2;

  //TODO
  if (ne.lock().get()->state() == NeighbourEntry::STALE)
  {
    //Initiate NUD timer to go to DELAY state & subsequently PROBE if
    //no indication of reachability (refer to RFC 2461 Sec. 7.3.2

    //Probably best to return an indication of this so RoutingCore starts the
    //timer or send a message to ND to initiate NUD

    Dout(dc::debug, rt->nodeName()<<":"<<info->ifIndex()<<" -- Reachability to "
         << info->nextHop() <<" STALE");
  }

  info->setLinkLayerAddr(ne.lock().get()->linkLayerAddr().c_str());

  return info->ifIndex();
} //end conceptualSending

/**
 *    Choose an apropriate source address
 *    should do:
 *    i)   get an address with an apropriate scope
 *    ii)  see if there is a specific route for the destination and use
 *         an address of the attached interface
 *    iii) don't use deprecated addresses or Expired Addresses TODO
 */

ipv6_addr IPv6Forward::determineSrcAddress(const ipv6_addr& dest, size_t ifIndex)
{
  ipv6_addr::SCOPE destScope = ipv6_addr_scope(dest);

#ifdef USE_MOBILITY
  //We don't care which outgoing iface dest is on because it is determined by
  //default router ifIndex anyway (preferably iface on link with Internet
  //connection after translation to care of addr)
  if (rt->mobilitySupport() && rt->isMobileNode() &&
      boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
      ->primaryHA().get() != 0 && destScope == ipv6_addr::Scope_Global)
  {
    Dout(dc::mipv6, rt->nodeName()<<" using homeAddress "
         <<boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
         ->homeAddr()<<" for destination "<<dest);
    return boost::polymorphic_downcast<MobileIPv6::MIPv6CDSMobileNode*>(rt->mipv6cds)
      ->homeAddr();
  }

#endif //USE_MOBILITY

  assert(dest != IPv6_ADDR_UNSPECIFIED);
  if (dest == IPv6_ADDR_UNSPECIFIED)
    return IPv6_ADDR_UNSPECIFIED;

  //ifIndex == UINT_MAX can only mean No default Router so assume dest is onlink
  //and return any link local address on the default Interface.  Address Res
  //will find the correct iface and the subsequent source Address
  if (ifIndex == UINT_MAX && rt->cds->defaultRouter().lock().get() == 0)
  {
    if (rt->getInterfaceByIndex(0).inetAddrs.size() == 0)
    {
      cerr <<rt->nodeId()<<" "<<rt->getInterfaceByIndex(ifIndex).iface_name
           <<" is not ready (no addresses assigned"<<endl;
      Dout(dc::mipv6, rt->nodeName()<<" "<<rt->getInterfaceByIndex(ifIndex).iface_name
           <<" is not ready (no addresses assigned");

      if (rt->odad())
      {
        if (rt->getInterfaceByIndex(0).tentativeAddrs.size())
        {
          Dout(dc::custom, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
               <<" determineSrcAddress no default rtr case ODAD is on using tentative addr="
               << rt->getInterfaceByIndex(0).tentativeAddrs[0]);
          return rt->getInterfaceByIndex(0).tentativeAddrs[0];
        }
      }

      return IPv6_ADDR_UNSPECIFIED;
    }
    else
      return rt->getInterfaceByIndex(0).inetAddrs[0];
  }

  Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);

  for (size_t i = 0; i < ie.inetAddrs.size(); i++)
    if (ie.inetAddrs[i].scope() == destScope)
      return ie.inetAddrs[i];

  if (rt->odad())
  {
    for (size_t i = 0; i < ie.tentativeAddrs.size(); i++)
      if (ie.tentativeAddrs[i].scope() == destScope)
      {
        Dout(dc::custom, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
             <<" determineSrcAddress using ODAD addr="
             << rt->getInterfaceByIndex(0).tentativeAddrs[0]);
        return ie.tentativeAddrs[i];
      }
  }

#if !defined NOIPMASQ
  //Perhaps allow return of src addresses with diff scope to allow for case of
  //"IP MASQ".
  return ie.inetAddrs[ie.inetAddrs.size() - 1];
#else
  return IPv6_ADDR_UNSPECIFIED;
#endif //!NOIPMASQ
} //end determineSrcAddress

/*
  @brief Test for a preconfigured source route to a destination and insert an
  appropriate routing header
 */
bool IPv6Forward::insertSourceRoute(IPv6Datagram& datagram)
{
  SRI srit = routes.find(datagram.destAddress());
  if (srit != routes.end() &&
      rt->localDeliver(datagram.srcAddress()) &&
      !datagram.findHeader(EXTHDR_ROUTING))
  {
    HdrExtRteProc* proc = datagram.acquireRoutingInterface();
    HdrExtRte* rt0 = proc->routingHeader();
    if(!rt0)
    {
      rt0 = new HdrExtRte;
      proc->addRoutingHeader(rt0);
    }

    const SrcRoute& route = srit->second;
    for_each(route->begin()+1, route->end(),
             boost::bind1st(mem_fun(&HdrExtRte::addAddress), rt0));
    //addAddress(rt0));
    assert(datagram.destAddress() == *(route->rbegin()));
    datagram.setDestAddress(*(route->begin()));
    return true;
  }
  return false;
}

bool IPv6Forward::processReceived(IPv6Datagram& datagram)
{
  // delete datagram and continue when datagram arrives from Network
  // for another node
  // IP_FORWARD is off (host)
  // All ND messages are 255 i.e. are not forwarded.
  // Link local address on either source or destination address
  // srcAddress is multicast
  if (!rt->isRouter() || datagram.hopLimit() >= 255 ||
      ipv6_addr_scope(datagram.srcAddress()) == ipv6_addr::Scope_Link ||
      ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Link||
      ipv6_addr_scope(datagram.srcAddress()) == ipv6_addr::Scope_Node ||
      ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Node||
      datagram.srcAddress().isMulticast() ||
      (!rt->routeSitePackets() &&
       (ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Site ||
        ipv6_addr_scope(datagram.srcAddress())  == ipv6_addr::Scope_Site)))
  {
    ctrIP6InAddrErrors++;
    Debug(

      if (!rt->isRouter())
      Dout(dc::debug, rt->nodeName()<<":"<<datagram.inputPort()<<" Packet discarded as this is a host");

      else if (datagram.hopLimit() >= 255)
    {
      // TODO: The datagram could be a proxy NS sent by mobile node
      // and the router could be CN's HA. Should we send proxy NA?
      Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()<<" Packet discarded as it is a ND packet");
    }
      else if (ipv6_addr_scope(datagram.srcAddress()) == ipv6_addr::Scope_Link ||

               ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Link)
      Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()
           <<" Packet discarded because the addresses src="<<datagram.srcAddress()<<" dest="
           <<datagram.destAddress()<< " have link local scope "<< dec <<datagram.hopLimit());

      else if (!rt->routeSitePackets() &&
               (ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Site ||
                ipv6_addr_scope(datagram.srcAddress())  == ipv6_addr::Scope_Site))
      Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()<<" Packet discarded because the addresses have site scope and forward site packets is "<<(rt->routeSitePackets()?"true":"false"));

      );

    return false;
  }

  //decrement hop Limit only when forwarding
  datagram.setHopLimit(datagram.hopLimit() - 1);
  if (datagram.hopLimit() == 0)
  {
    sendErrorMessage(new ICMPv6Message(ICMPv6_TIME_EXCEEDED,
                                       ND_HOP_LIMIT_EXCEEDED,
                                       datagram.dup()));
    Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()
         <<" hop limit exceeded"<<datagram);
    return false;
  }

  return true;
}

/*     ----------------------------------------------------------
        Private Functions
    ----------------------------------------------------------    */

// send error message to ICMP Module
void IPv6Forward::sendErrorMessage(ICMPv6Message* err)
{
    send(err, "errorOut");
}

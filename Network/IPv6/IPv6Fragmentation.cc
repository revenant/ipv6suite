//
// Copyright (C) 2004 CTIE, Monash University
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
   @file IPv6Fragmentation.cc
   @brief Implementation for IPv6Fragmentation

   Responsibilities:
   receive valid IP datagram from Routing or Multicast
   Fragment datagram if size > MTU [output port] && src_addr == node_addr
   send fragments to IPOutput[output port]

   @author Johnny Lai

   Based on IPFragmentation by Jochen Reber
*/
#include "sys.h"
#include "debug.h"

#include <cassert>

#include "IPv6Fragmentation.h"
#include "RoutingTable6.h"
#include "Constants.h"
#include "HdrExtFragProc.h"
#include "IPv6Datagram.h"
#include "ICMPv6Message.h"
#include "IPv6InterfaceData.h"
#include "Messages.h"

typedef cTypedMessage<LLInterfaceInfo> LLInterfacePkt;

Define_Module( IPv6Fragmentation );

/*  ----------------------------------------------------------
        Public Functions
    ----------------------------------------------------------  */

void IPv6Fragmentation::initialize()
{
  RoutingTable6Access::initialize();
  numOfPorts = par("numOfPorts");
  delay = par("procdelay");
  ctrIP6InTooBig = 0;
  ctrIP6FragCreates = 0;
  ctrIP6FragFails = 0;
  ctrIP6FragOKs = 0;

  curPacket = 0;
  waitTmr = new cMessage("IPv6FragmentationWait");
  waitQueue.setName("fragmentWaitQ");

  string display(displayString());
  display += ";q=fragmentWaitQ";
  setDisplayString(display.c_str());
}

/**
 * @todo Use Path MTU in Destination Entry once that's implemented instead of
 * just the device MTU
 * TODO delay per fragmented datagram sent
 */
void IPv6Fragmentation::handleMessage(cMessage* msg)
{
  IPv6Datagram* datagram = 0;

  if (!msg->isSelfMessage())
  {
    AddrResMsg*  newAddrResMsg = boost::polymorphic_downcast<AddrResMsg*> (msg);
    assert(newAddrResMsg);

    if (waitTmr->isScheduled())
    {
      //cerr<<fullPath()<<" "<<simTime()<<" received new packet "<<*newAddrResMsg->data().dgram
      //<<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime();
      Dout(dc::custom, fullPath()<<" "<<simTime()<<" received new packet "<<*newAddrResMsg->data().dgram
           <<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime());
      //delete newAddrResMsg->data().dgram;
      //delete newAddrResMsg;
      waitQueue.insert(newAddrResMsg);
      return;
    } else if (!waitTmr->isScheduled() && 0 == curPacket)
    {
      curPacket = newAddrResMsg;
      AddrResInfo& info = curPacket->data();
      datagram = info.dgram;
      Interface6Entry& ie = rt->getInterfaceByIndex(info.ifIndex);
      mtu = ie.mtu;

      assert(mtu >= IPv6_MIN_MTU); //All IPv6 links must conform


      if (datagram->inputPort() == -1 && datagram->hopLimit() == 0)
      {
        if (!rt->isRouter())
          datagram->setHopLimit(ie.curHopLimit);
        else
          datagram->setHopLimit(DEFAULT_ROUTER_HOPLIMIT);
      }

      scheduleAt(delay + simTime(), waitTmr);
      return;
    }
    assert(false);
    return;
  }

  AddrResMsg* addrResMsg = curPacket;
  assert(addrResMsg);
  datagram = addrResMsg->data().dgram;
  if (waitQueue.empty())
    curPacket = 0;
  else
  {
    curPacket = boost::polymorphic_downcast<AddrResMsg*>(waitQueue.pop());
    scheduleAt(delay + simTime(), waitTmr);
  }

  /*
    ev << "totalLength / MTU: " << datagram->totalLength() << " / "
    << mtu << "\n";
  */

  // check if datagram does not require fragmentation
  if (datagram->totalLength() <= mtu)
  {
    sendOutput(addrResMsg);
    addrResMsg = 0;
    return;
  }

  //Source fragmentation only in IPv6
  if (datagram->inputPort() != -1 ||
      //Do not fragment ICMP
      datagram->transportProtocol() == IP_PROT_IPv6_ICMP)
  {
    //ICMP packets do come through here however they should be limited
    //in size during creation time. Either that or we drop them
    assert(datagram->transportProtocol() != IP_PROT_IPv6_ICMP);
    ICMPv6Message* err = new ICMPv6Message(ICMPv6_PACKET_TOO_BIG);
    err->encapsulate(datagram->dup());
    err->setName("ICMPv6_ERROR:PACKET_TOO_BIG");
    err->setOptInfo(mtu);
    sendErrorMessage(err);
    if (datagram->inputPort() != -1) //Tried to forward a big packet
      ctrIP6InTooBig++;
    delete datagram;
    delete addrResMsg;
    addrResMsg = 0;
    return;
  }


  HdrExtFragProc* fragProc = datagram->acquireFragInterface();
  assert(fragProc != 0);
  if (!fragProc)
    ctrIP6FragFails++;

  unsigned int noOfFragments = 0;
  IPv6Datagram** fragment = fragProc->fragmentPacket(datagram, mtu, noOfFragments);
  for (size_t i=0; i<noOfFragments; i++)
  {
    AddrResMsg* duplicate = addrResMsg->dup();
    duplicate->data().dgram = fragment[i];
    //We never had fragmentation anyway so don't worry about implementing wait
    //within loops or use a delay proportional to number of frag packets and
    //send them all out at once after delay.
//      wait(delay);
    ctrIP6FragCreates++;
    sendOutput(duplicate);
  }
  delete [] fragment;
  delete addrResMsg;
  delete datagram;
  addrResMsg = 0;
  ctrIP6FragOKs++;

    /*
        headerLength = datagram->headerLength();
        payload = datagram->totalLength() - headerLength;

        noOfFragments=
            int(ceil((float(payload)/mtu) /
            (1-float(headerLength)/mtu) ) );

        ev << "No of Fragments: " << noOfFragments << "\n";


           // if "don't Fragment"-bit is set, throw
            datagram away and send ICMP error message
        if (datagram->dontFragment() && noOfFragments > 1)
        {
            sendErrorMessage (datagram,
                ICMP_DESTINATION_UNREACHABLE,
                ICMP_FRAGMENTATION_ERROR_CODE);
            continue;
        }

        for (i=0; i<noOfFragments; i++)
        {

            IPv6Datagram *fragment;
            fragment = datagram->dup();

                // total_length equal to mtu, except for
                last fragment

                // "more Fragments"-bit is unchanged
                in the last fragment, otherwise
                true
            if (i != noOfFragments-1)
            {

                fragment->setMoreFragments (true);
                fragment->setTotalLength(mtu);
            } else
            {
                    // size of last fragment
                fragment->setTotalLength
                    (datagram->totalLength() -
                     (noOfFragments-1) *
                     (mtu - datagram->headerLength()));
            }
            fragment->setFragmentOffset(
                    i*(mtu - datagram->headerLength()) );


            wait(delay);
            sendDatagramToOutput(fragment);
        } // end for to noOfFragments
        delete( datagram );
*/

}


void IPv6Fragmentation::finish()
{
  recordScalar("IP6InTooBigErrors", ctrIP6InTooBig++);
  recordScalar("IP6FragCreates", ctrIP6FragCreates);
  recordScalar("IP6FragFails", ctrIP6FragFails);
  recordScalar("IP6FragOKs", ctrIP6FragOKs);

}


/*  ----------------------------------------------------------
        Private Functions
    ----------------------------------------------------------  */

//  send error message to ICMP Module
void IPv6Fragmentation::sendErrorMessage(ICMPv6Message* err)
{
  send(err, "errorOut");
}

void IPv6Fragmentation::sendOutput(AddrResMsg* msg )
{
  int outputPort = msg->data().ifIndex;

  if (!(outputPort < numOfPorts))
  {
    ev << "Error in IPv6Fragmentation: "
       << "illegal output port: " << outputPort << "\n";

    delete msg->data().dgram;
    delete msg;
    return;
  }
  LLInterfaceInfo info = { msg->data().dgram, msg->data().linkLayerAddr };
  LLInterfacePkt* ifpkt = new LLInterfacePkt(info);
  ifpkt->setName(info.dgram->name());

  // XXX This needs to be done in llpkt itself as take is a protected function,
  //except that's a template class.  We do not know the type of the template
  //parameter can contain members that are cobjects. Guess cTypedMessage needs
  //refactoring too

  //ifpkt->take(info.dgram);

  delete msg;
  send(ifpkt, "outputOut", outputPort);
}


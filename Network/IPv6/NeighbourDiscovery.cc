//
// Copyright (C) 2001, 2004 Johnny Lai
// Monash University, Melbourne, Australia

/**
   @file NeighbourDiscovery.cc
   @brief Implementation of Simple module NeighbourDiscovery.
*/
#include "NeighbourDiscovery.h"
#include "ICMPv6NDMessage.h"
#include "opp_utils.h"
#include "NDStates.h"
#include "RoutingTable6.h"
#include "RoutingTable6Access.h"
#include "cTimerMessage.h"


Define_Module( NeighbourDiscovery );

using namespace IPv6NeighbourDiscovery;

namespace IPv6NeighbourDiscovery
{
  const int Ctrl_NodeInitialise = 7001;
  const int Tmr_DupAddrSolTimeout = 8001;
  const int Tmr_NextUnsolRtrAd = 8002;
  const int Tmr_RtrSolTimeout = 8003;
  const int Tmr_AddrConfLifetime = 8004;
  const int Tmr_RouterLifetime = 8005;
  const int Tmr_PrefixLifetime = 8006;
}

namespace MobileIPv6
{
  //MIPv6 ND timers
  const int Tmr_RtrAdvMissed = 8010;
  const int Sched_SendRtrSol = 9000;
  //Sent once only per returning home
  const int Sched_SendUnsolNgbrAd = 10000;
}


int NeighbourDiscovery::numInitStages() const
{
  return 5;
}

IPv6NeighbourDiscovery::NDState* NeighbourDiscovery::getRouterState()
{
  assert(rt->isRouter());
  return nd;
}

/**
 * @brief determines when to create the Host/Router states
 *
 * Very important to get this done after all the info has been parsed in XML file
 *
 */

void NeighbourDiscovery::initialize(int stageNo)
{
  if (stageNo == 0)
  {
#ifdef USE_MOBILITY
    missedRtrAdvLatency = new cOutVector("Movement Detection Latency");
    missedRtrDuration = 0;
    l2LinkupTime = 0;
    rsLatency = new cOutVector("RS - RA Latency");;
    rsSentTime = 0;
#endif // USE_MOBILITY

    rt = RoutingTable6Access().get();

    ctrIcmp6OutRtrSol = 0;
    ctrIcmp6OutRtrAdv = 0;
    ctrIcmp6OutRedirect = 0;
    ctrIcmp6InRtrSol = ctrIcmp6InRtrAdv = 0;
    ctrIcmp6InNgbrSol = ctrIcmp6InNgbrAdv = 0;
    ctrIcmp6InRedirect = 0;

  }
  //Need to be at stage three as stage two collects important info on whether this
  //node is a router, MIPv6 support and such
  else if (stageNo == 2)
    nd = NDState::startND(this);
  else if (stageNo == 4)
    nd->print();
}

void NeighbourDiscovery::handleMessage(cMessage* theMsg)
{
  std::auto_ptr<cMessage> msg(theMsg);

  if (msg->isSelfMessage())
    (static_cast<cTimerMessage *> (msg.release()) )->callFunc();
  else
  {
    nd->processMessage(OPP_Global::auto_downcast<ICMPv6Message>(msg));
// #if !defined TESTIPv6
//     nd->processMessage(static_cast<ICMPv6Message*>(msg));
// #else
//     assert(dynamic_cast<ICMPv6Message*>(msg) != 0);
//     nd->processMessage(static_cast<ICMPv6Message*>(msg));
// #endif //TESTIPv6
  }

}

void NeighbourDiscovery::finish()
{
  // XXX cleanup stuff must be moved to dtor!
  delete nd;
  nd = 0;

  recordScalar("Icmp6OutRtrSol", ctrIcmp6OutRtrSol);
  recordScalar("Icmp6OutRtrAdv", ctrIcmp6OutRtrAdv);
  recordScalar("Icmp6OutRedirect", ctrIcmp6OutRedirect);
  recordScalar("Icmp6InRtrSol", ctrIcmp6InRtrSol);
  recordScalar("Icmp6InRtrAdv", ctrIcmp6InRtrAdv);
  recordScalar("Icmp6InNgbrSol", ctrIcmp6InNgbrSol);
  recordScalar("Icmp6InNgbrAdv", ctrIcmp6InNgbrAdv);
  recordScalar("Icmp6InRedirect", ctrIcmp6InRedirect);

}

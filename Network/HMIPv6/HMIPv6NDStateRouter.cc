//
// Copyright (C) 2002 CTIE, Monash University
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
 * @file   HMIPv6NDStateRouter.cc
 * @author Johnny Lai
 * @date   05 Sep 2002
 *
 * @brief  Implementation of HMIPv6NDStateRouter class
 *
 */


#include "HMIPv6NDStateRouter.h"
#include "HMIPv6ICMPv6NDMessage.h"
#include "RoutingTable6.h"

namespace HierarchicalMIPv6
{
  HMIPv6NDStateRouter::HMIPv6NDStateRouter(NeighbourDiscovery* mod)
    :MIPv6NDStateRouter(mod)
  {}


  HMIPv6NDStateRouter::~HMIPv6NDStateRouter()
  {}

  void HMIPv6NDStateRouter::print(void)
  {
    cout << "=============== MAP Specific Node Level Configuration Variables ============" <<endl;
    if ( mode == HMIPv6NDStateRouter::modeBasic )
      cout << "Modes of operation: BASIC" << endl;
    else
      cout << "Modes of operation: EXTENDED" << endl;

    if ( mnMUSTSetRoCAAsSource )
      cout << "MN operation: MN MUST use its RCoA as source address of its outgoing packets "<< endl;
    else
    {
      if ( mnMAYSetRoCAAsSource )
        cout << "MN operation: MN MAY use its RCoA as source address of its outgoing packets "<< endl;
    }

    if ( reverseTunnel )
      cout << "Reverse tunnelling outbound traffic to map: "<< endl;

    cout << "===================  MAP Option Information  ===============================" <<endl;
    int mapIndex = 0;
    for (MAPIt mapIt = mapOptions.begin(); mapIt != mapOptions.end(); mapIt++)
    {
      cout << "MAP["<<mapIndex<<"]: "<<(*mapIt).addr()<<endl;
      cout << "interface: "<< (*mapIt).ifaceIdx() <<"\t | ";
      cout << "AdvMAPValidLifetime: " <<(*mapIt).lifetime()<<endl;
      mapIndex++;
    }
    cout << "============================================================================" << endl;
  }
/*
  HMIPv6ICMPv6NDOptMAP* HMIPv6NDStateRouter::getMAPbyInterface(size_t iface_Idx)
  {

    return 0;
  }
*/

  ICMPv6NDMRtrAd* HMIPv6NDStateRouter
  ::createRA(const IPv6InterfaceData::RouterVariables& rtrVar, size_t ifidx)
  {
    ICMPv6NDMRtrAd* rtrAd =
      MobileIPv6::MIPv6NDStateRouter::createRA(rtrVar, ifidx);

    if (rt->isMAP())
    {
      for (MAPIt mapIt = mapOptions.begin(); mapIt != mapOptions.end(); mapIt++)
      {
        bool r, m;

        if ( mode == modeBasic )
        {
          r = true;
          m = false;
        }
        else if ( mode == modeExtended )
        {
          r = false;
          m = true;
        }

        if ((*mapIt).ifaceIdx() == ifidx)
        {
          (*mapIt).setR(r);
          (*mapIt).setM(m);
          (*mapIt).setI(mnMAYSetRoCAAsSource);
          (*mapIt).setP(mnMUSTSetRoCAAsSource);
          (*mapIt).setV(reverseTunnel);

          rtrAd->addOption((*mapIt));
        }
      }
    }

    return rtrAd;
  }

} //namespace HierarchicalMIPv6

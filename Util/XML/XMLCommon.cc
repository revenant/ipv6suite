// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   XMLCommon.cc
 * @author Johnny Lai
 * @date   09 Sep 2004
 *
 * @brief  Implementation of XMLCommon
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "XMLCommon.h"
#include "RoutingTable6.h"
#include "opp_utils.h"
#include "InterfaceTableAccess.h"
#include "IPv6InterfaceData.h"


namespace XMLConfiguration
{
  const std::string XML_ON = "on";

void checkValidData(InterfaceTable *ift, RoutingTable6* rt)
{
    bool isError = false;

    const char* nodeName = OPP_Global::findNetNodeModule(rt)->name();

    for(size_t i = 0; i < ift->numInterfaceGates(); i++)
    {
      InterfaceEntry *ie = ift->interfaceByPortNo(i);

      //Todo Max/MinRtrAdvInt checks are clumsy as the DTD can have another set
      //i.e. MIPv6MaxRtrAdvInterval attr for MIPv6 defaults they are not parsed
      //unless actual interface element exists. Need to really do the default
      //parsing of elements into objects even for elements that don't exist but
      //the objects do.
#ifdef USE_MOBILITY
        // mobility support
      if ((rt->mobilitySupport() && ie->ipv6()->rtrVar.advSendAds &&
          (ie->ipv6()->rtrVar.maxRtrAdvInt > 1.5 ||
           ie->ipv6()->rtrVar.maxRtrAdvInt < MIN_MIPV6_MAX_RTR_ADV_INT)) ||
          // without mobility support
          (!rt->mobilitySupport() &&
#else
      if
#endif
           (ie->ipv6()->rtrVar.maxRtrAdvInt < 4 ||
            ie->ipv6()->rtrVar.maxRtrAdvInt > 1800)
#ifdef USE_MOBILITY
           ))
#endif //USE_MOBILITY
      {
        cerr<<nodeName << ", interface[" << i
            <<"], MaxRtrAdvInterval not set correctly "
            <<ie->ipv6()->rtrVar.maxRtrAdvInt<<endl;
        Dout(dc::warning, nodeName << ", interface[" << i
             <<"], MaxRtrAdvInterval not set correctly "
             <<ie->ipv6()->rtrVar.maxRtrAdvInt);
        isError = true;
        break;
      }

#ifdef USE_MOBILITY
      if((rt->mobilitySupport() && ie->ipv6()->rtrVar.advSendAds &&
          ie->ipv6()->rtrVar.minRtrAdvInt < MIN_MIPV6_MIN_RTR_ADV_INT) ||
         (ie->ipv6()->rtrVar.minRtrAdvInt >
          ie->ipv6()->rtrVar.maxRtrAdvInt) ||
         // without mobility support
         (!rt->mobilitySupport() &&
#else
      // minRrtAdvInterval must be no less than 3 seconds and no greater
      // than .75 * MaxRtrAdvInterval Sec. 6.2.1 of RFC 2461
      if
#endif
          (ie->ipv6()->rtrVar.minRtrAdvInt < 3 ||
         ie->ipv6()->rtrVar.minRtrAdvInt > 0.75 *
           ie->ipv6()->rtrVar.maxRtrAdvInt)
#ifdef USE_MOBILITY
          ))
#endif //USE_MOBILITY
      {
        cerr<<nodeName << ", interface[" << i
            <<"], MinRtrAdvInterval="<<ie->ipv6()->rtrVar.minRtrAdvInt
            <<" not set correctly. MaxRtrAdvInterval="
            <<ie->ipv6()->rtrVar.maxRtrAdvInt
            <<endl;
        Dout(dc::warning, nodeName << ", interface[" << i
             <<"], MinRtrAdvInterval="<<ie->ipv6()->rtrVar.minRtrAdvInt
             <<" not set correctly. MaxRtrAdvInterval="
             <<ie->ipv6()->rtrVar.maxRtrAdvInt);
        isError = true;
        break;
      }

      if(ie->ipv6()->rtrVar.advReachableTime > 3600000)
      {
        cerr<<nodeName << ", interface[" << i
            <<"], AdvReachableTime not set correctly" << endl;
        isError = true;
        break;
      }

      // AdvDefaultLifetime MUST be either 0 or between
      // MaxRtrAdvInterval and 9000 seconds
      if(ie->ipv6()->rtrVar.advDefaultLifetime != 0 &&
         (ie->ipv6()->rtrVar.advDefaultLifetime <
          ie->ipv6()->rtrVar.maxRtrAdvInt ||
          ie->ipv6()->rtrVar.advDefaultLifetime > 9000 ))
      {
        cerr<<nodeName << ", interface[" << i
            <<"], AdvDefaultLifetime not set correctly "
            <<ie->ipv6()->rtrVar.advDefaultLifetime<< endl;
        isError = true;
        break;
      }
    }

    if(isError)
      exit(0);

}

}


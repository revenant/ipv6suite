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

namespace XMLConfiguration
{
  const std::string XML_ON = "on";

void checkValidData(RoutingTable6* rt)
{
    bool isError = false;

    const char* nodeName = OPP_Global::findNetNodeModule(rt)->name();

    for(size_t i = 0; i < rt->interfaceCount(); i++)
    {
      Interface6Entry *ie = rt->getInterfaceByIndex(i);

      //Todo Max/MinRtrAdvInt checks are clumsy as the DTD can have another set
      //i.e. MIPv6MaxRtrAdvInterval attr for MIPv6 defaults they are not parsed
      //unless actual interface element exists. Need to really do the default
      //parsing of elements into objects even for elements that don't exist but
      //the objects do.
#ifdef USE_MOBILITY
        // mobility support
      if ((rt->mobilitySupport() && ie->rtrVar.advSendAds &&
          (ie->rtrVar.maxRtrAdvInt > 1.5 ||
           ie->rtrVar.maxRtrAdvInt < MIN_MIPV6_MAX_RTR_ADV_INT)) ||
          // without mobility support
          (!rt->mobilitySupport() &&
#else
      if
#endif
           (ie->rtrVar.maxRtrAdvInt < 4 ||
            ie->rtrVar.maxRtrAdvInt > 1800)
#ifdef USE_MOBILITY
           ))
#endif //USE_MOBILITY
      {
        cerr<<nodeName << ", interface[" << i
            <<"], MaxRtrAdvInterval not set correctly "
            <<ie->rtrVar.maxRtrAdvInt<<endl;
        Dout(dc::warning, nodeName << ", interface[" << i
             <<"], MaxRtrAdvInterval not set correctly "
             <<ie->rtrVar.maxRtrAdvInt);
        isError = true;
        break;
      }

#ifdef USE_MOBILITY
      if((rt->mobilitySupport() && ie->rtrVar.advSendAds &&
          ie->rtrVar.minRtrAdvInt < MIN_MIPV6_MIN_RTR_ADV_INT) ||
         (ie->rtrVar.minRtrAdvInt >
          ie->rtrVar.maxRtrAdvInt) ||
         // without mobility support
         (!rt->mobilitySupport() &&
#else
      // minRrtAdvInterval must be no less than 3 seconds and no greater
      // than .75 * MaxRtrAdvInterval Sec. 6.2.1 of RFC 2461
      if
#endif
          (ie->rtrVar.minRtrAdvInt < 3 ||
         ie->rtrVar.minRtrAdvInt > 0.75 *
           ie->rtrVar.maxRtrAdvInt)
#ifdef USE_MOBILITY
          ))
#endif //USE_MOBILITY
      {
        cerr<<nodeName << ", interface[" << i
            <<"], MinRtrAdvInterval="<<ie->rtrVar.minRtrAdvInt
            <<" not set correctly. MaxRtrAdvInterval="
            <<ie->rtrVar.maxRtrAdvInt
            <<endl;
        Dout(dc::warning, nodeName << ", interface[" << i
             <<"], MinRtrAdvInterval="<<ie->rtrVar.minRtrAdvInt
             <<" not set correctly. MaxRtrAdvInterval="
             <<ie->rtrVar.maxRtrAdvInt);
        isError = true;
        break;
      }

      if(ie->rtrVar.advReachableTime > 3600000)
      {
        cerr<<nodeName << ", interface[" << i
            <<"], AdvReachableTime not set correctly" << endl;
        isError = true;
        break;
      }

      // AdvDefaultLifetime MUST be either 0 or between
      // MaxRtrAdvInterval and 9000 seconds
      if(ie->rtrVar.advDefaultLifetime != 0 &&
         (ie->rtrVar.advDefaultLifetime <
          ie->rtrVar.maxRtrAdvInt ||
          ie->rtrVar.advDefaultLifetime > 9000 ))
      {
        cerr<<nodeName << ", interface[" << i
            <<"], AdvDefaultLifetime not set correctly "
            <<ie->rtrVar.advDefaultLifetime<< endl;
        isError = true;
        break;
      }
    }

    if(isError)
      exit(0);

}

}


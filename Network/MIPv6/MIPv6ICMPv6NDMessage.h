// -*- C++ -*-
// Copyright (C) 2001 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// -*- C++ -*-
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
    @file MIPv6ICMPv6NDMessage.h

    @brief Modification to IPv6 ND messages and prefix information option
             as well as new specific MIPv6 options
             (draft-ietf-mobileip-ipv6-16), section 6.1

    @author Eric Wu
    @date 7/4/2002

*/

#ifndef MIPv6ICMPV6NDMESSAGE_H
#define MIPv6ICMPV6NDMESSAGE_H

#ifndef ICMPV6NDMESSAGE_H
#include "ICMPv6NDMessage.h"
#endif //ICMPV6NDMESSAGE_H

#if defined USE_HMIP
#ifndef HMIPV6ICMPV6NDMESSAGE_H
#include "HMIPv6ICMPv6NDMessage.h"
#endif //HMIPV6ICMPV6NDMESSAGE_H
#endif //defined USE_HMIP

namespace MobileIPv6
{

// class MIPv6ICMPv6NDOptHomeAgentInfo
//
// New Home Agent Information Option

class MIPv6ICMPv6NDOptHomeAgentInfo: public ICMPv6_NDOptionBase
{
public:
  MIPv6ICMPv6NDOptHomeAgentInfo(const int haPref = 0, const int haLifeTime = 0);

  virtual MIPv6ICMPv6NDOptHomeAgentInfo* dup() const
    {
      return new MIPv6ICMPv6NDOptHomeAgentInfo(*this);
    }

  int haPref;
  int haLifetime;
};

// class MIPv6ICMPv6NDMRtrAd
//
// Implementation of modified Neighbour Dicovery message, router
// advertisement (RA) for Mobile IPv6. This RA includes two extra
// possible options, Advertisement Interval Option and Home Agent
// Information Option as well modified Prefix option;

class MIPv6ICMPv6NDMRtrAd : public IPv6NeighbourDiscovery::ICMPv6NDMRtrAd
{
 public:
  MIPv6ICMPv6NDMRtrAd(int lifetime, int hopLimit, unsigned int reach,
                           unsigned int retrans,
                           const AdvPrefixList& prefixList,
                           bool managed = false, bool other = false,
                           // ... specific attributes
                           bool homeagent = false
                           );

  MIPv6ICMPv6NDMRtrAd(int lifetime, int hopLimit = 0,
                           unsigned int reach = 0, unsigned int retrans = 0,
                           bool managed = false, bool other = false,
                           // ... specific attributes
                           bool homeagent = false
                           );

  MIPv6ICMPv6NDMRtrAd(const MIPv6ICMPv6NDMRtrAd& src);

  ~MIPv6ICMPv6NDMRtrAd(void)
    {
      if (_haInfo)
        delete _haInfo;
    }

  const MIPv6ICMPv6NDMRtrAd& operator=
    (const MIPv6ICMPv6NDMRtrAd& src);

  bool operator==(const MIPv6ICMPv6NDMRtrAd& rhs) const;

  virtual MIPv6ICMPv6NDMRtrAd* dup() const
    { return new MIPv6ICMPv6NDMRtrAd(*this); }

  void setHomeAgent(bool b_ha)
    {
#ifdef USE_MOBILITY // for HOMEAGENT_MASK
      if ( b_ha )
        setOptInfo(optInfo() | HOMEAGENT_MASK);
      else
        setOptInfo(optInfo() & ~HOMEAGENT_MASK);
#endif
    }

  bool hasHomeAgentInfo() const
    {
      return _haInfo != 0;
    }

  void setHomeAgentInfo(int haPref, int haLifetime)
    {
      bool changeLength = true;
      if (_haInfo)
      {
        changeLength = false;
        _haInfo->haPref = haPref;
        _haInfo->haLifetime = haLifetime;
      }
      else
        _haInfo = new MIPv6ICMPv6NDOptHomeAgentInfo(haPref, haLifetime);

      if (changeLength)
        setLength(length() + _haInfo->length());
    }

  const MIPv6ICMPv6NDOptHomeAgentInfo& homeAgentInfo()
    {
      return *(static_cast<MIPv6ICMPv6NDOptHomeAgentInfo*> (_haInfo));
    }

 protected:
  MIPv6ICMPv6NDOptHomeAgentInfo* _haInfo;
};

} // end namespace MobileIPv6

#endif // MIPv6ICMPV6NDMESSAGE_H

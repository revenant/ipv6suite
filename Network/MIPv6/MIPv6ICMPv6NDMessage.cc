//
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
	@file MIPv6ICMPv6NDMessage.cc
    
	@brief Modification to IPv6 ND messages and prefix information option
             as well as new specific MIPv6 options
             (draft-ietf-mobileip-ipv6-16), section 6.1
    
	@author Eric Wu
    @date 9/4/2002
 
*/

#include "MIPv6ICMPv6NDMessage.h"

namespace MobileIPv6
{

// Definition of class MIPv6ICMPv6NDMRtrAd

MIPv6ICMPv6NDMRtrAd::
MIPv6ICMPv6NDMRtrAd(int lifetime, int hopLimit, unsigned int reach, 
                     unsigned int retrans, const AdvPrefixList& prefixList,
                     bool managed, bool other, bool homeagent)
  :ICMPv6NDMRtrAd(lifetime, hopLimit, reach, retrans, prefixList,
                  managed, other) 
{
  _haInfo = 0;
  setHomeAgent(homeagent);
}

MIPv6ICMPv6NDMRtrAd::
MIPv6ICMPv6NDMRtrAd(int lifetime, int hopLimit, unsigned int reach,
                         unsigned int retrans, bool managed,
                         bool other, bool homeagent)
  :ICMPv6NDMRtrAd(lifetime, hopLimit, reach, retrans, managed, other)
{
  _haInfo = 0;  
  setHomeAgent(homeagent);; 
}

MIPv6ICMPv6NDMRtrAd::
MIPv6ICMPv6NDMRtrAd(const MIPv6ICMPv6NDMRtrAd& src)
// there is no default constructor of ICMPv6NDMsgBaseRtrAd hence
// entering some "default" values into existing constructor
  :ICMPv6NDMRtrAd(0, 0, 0, 0, false, false)
{
  _haInfo = 0;  
  setName(src.name());
  // operator=() will overwrite the values of attributes that the
  // constructor with "default" values took
  operator=(src);  
}

const MIPv6ICMPv6NDMRtrAd& MIPv6ICMPv6NDMRtrAd::
operator=(const MIPv6ICMPv6NDMRtrAd& src)
{
  if (this != &src)
  {
    ICMPv6NDMRtrAd::operator=(src);
    if (_haInfo && src._haInfo)
    {
      _haInfo->haPref = src._haInfo->haPref;
      _haInfo->haLifetime = src._haInfo->haLifetime;
    }
    else if (!_haInfo && src._haInfo)
      _haInfo = src._haInfo->dup();
  }
  
  return *this;  
}

bool MIPv6ICMPv6NDMRtrAd::
operator==(const MIPv6ICMPv6NDMRtrAd& rhs) const
{
  if (this == &rhs)
    return true;
  
  bool success = ICMPv6NDMRtrAd::operator==(rhs);

  if( !success)
    return false;

  return (_haInfo->haPref == rhs._haInfo->haPref &&
          _haInfo->haLifetime == rhs._haInfo->haLifetime);
}

// class MIPv6ICMPv6NDOptHomeAgentInfo 

MIPv6ICMPv6NDOptHomeAgentInfo::
MIPv6ICMPv6NDOptHomeAgentInfo(const int haPref, const int haLifeTime)
  : ICMPv6_NDOptionBase(NDO_HOME_AGENT_INFO, 1),
    haPref(haPref), haLifetime(haLifeTime)
{}

} // end namespace MobileIPv6

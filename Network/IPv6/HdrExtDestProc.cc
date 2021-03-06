//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
 * @file   HdrExtDestProc.cc
 * @author Johnny Lai
 * @date   Sun Apr  7 17:01:17 2002
 *
 * @brief  Implementation of IPv6 Destination Extension Header
 *
 *
 */


#include <boost/cast.hpp>
#include "IPv6LocalDeliver.h"
#include "HdrExtDestProc.h"


HdrExtDestProc::HdrExtDestProc()
  :HdrExtProc(EXTHDR_DEST), opt_hdr(*boost::polymorphic_downcast<ipv6_ext_opts_hdr*>(ext_hdr))
{
}

HdrExtDestProc::HdrExtDestProc(const HdrExtDestProc& src)
  :HdrExtProc(EXTHDR_DEST), opt_hdr(*boost::polymorphic_downcast<ipv6_ext_opts_hdr*>(ext_hdr))
{
  operator=(src);
}

HdrExtDestProc::~HdrExtDestProc()
{
  for ( size_t i = 0; i < destOpts.size(); i ++)
    delete destOpts[i];
}

bool HdrExtDestProc::operator==(const HdrExtDestProc& rhs)
{
  if (!HdrExtProc::operator==(rhs))
    return false;

  if (!(opt_hdr == rhs.opt_hdr && destOpts.size() == rhs.destOpts.size()))
    return false;

  for ( size_t i = 0; i < rhs.destOpts.size(); i ++)
  {
    if (destOpts[i] != rhs.destOpts[i])
      return false;
  }
  return true;
}

HdrExtDestProc& HdrExtDestProc::operator=(const HdrExtDestProc& rhs)
{
  if (this != &rhs)
  {
    HdrExtProc::operator=(rhs);

    opt_hdr = rhs.opt_hdr;
    destOpts.clear();
    for ( size_t i = 0; i < rhs.destOpts.size(); i ++)
      destOpts.push_back(rhs.destOpts[i]->dup());
  }

  return *this;
}

ostream& HdrExtDestProc::operator<<(std::ostream& os)
{
  IPv6TLVOptionBase* opt = getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT);
  if (opt)
    return opt->operator<<(os);
  else
  {
    return os<<" Destination Extension Hdr type ="<<type();
  }
}

bool HdrExtDestProc::processHeader(cSimpleModule* mod, IPv6Datagram* pdu)
{
  bool success = true;

  IPv6LocalDeliver* core = static_cast<IPv6LocalDeliver*>(mod);

  for ( size_t i = 0; i < destOpts.size(); i++ )
  {
    success = destOpts[i]->processOption(core, pdu);

    if(!success)
      return false;
  }

  return success;
}

/**
 * Assuming that we can only have one type of tlv option per destination option
 * header.
 * @return false if opt exists already or true if opt was added
 */

bool HdrExtDestProc::addOption(const IPv6TLVOptionBase* opt)
{
  for ( size_t i = 0; i < destOpts.size(); i ++)
  {
    if ( destOpts[i]->type() == const_cast<IPv6TLVOptionBase*>(opt)->type())
      return false;
  }
  destOpts.push_back(const_cast<IPv6TLVOptionBase*>(opt));
  return true;
}

IPv6TLVOptionBase* HdrExtDestProc::
getOption(IPv6TLVOptionBase::TLVOptType type)
{
  for ( size_t i = 0; i < destOpts.size(); i ++)
  {
    if ( destOpts[i]->type() == type)
      return destOpts[i];
  }
  return 0;
}

IPv6TLVOptionBase::IPv6TLVOptionBase(TLVOptType otype, unsigned char len)
  : _type(otype), _len(len)
{}


IPv6TLVOptionBase::~IPv6TLVOptionBase()
{}

//
// Copyright (C) 2002 CTIE, Monash University
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
    @file MIPv6MessageBase.cc
    @brief Abstract classes for Mobility Support IPv6 Messages
    @see draft-ietf-mobileip-ipv6-16.txt, section 5
    @author Eric Wu
    @date 4/4/2002
*/

#include "MIPv6MessageBase.h"

namespace MobileIPv6
{

// MIPv6MobilityHeaderBase definition

MIPv6MobilityHeaderBase::
MIPv6MobilityHeaderBase(MIPv6MobilityHeaderType headertype, int len)
  : cMessage("", IP_PROT_IPv6_MOBILITY),
    _payloadprot(IP_PROT_NONE),
    _headertype(headertype),
    _checksum(0)
{
  setLength(len);
}

MIPv6MobilityHeaderBase::
MIPv6MobilityHeaderBase(const MIPv6MobilityHeaderBase& src)
{
  setName(src.name());
  operator= (src);
}

MIPv6MobilityHeaderBase::~MIPv6MobilityHeaderBase(void)
{
  for (size_t i = 0; i < _parameters.size(); i++)
    delete _parameters[i];

  _parameters.empty();
}

MIPv6MobilityHeaderBase& MIPv6MobilityHeaderBase::
operator=(const MIPv6MobilityHeaderBase& rhs)
{
  if (this != &rhs)
  {
    cMessage::operator=(rhs);
    _checksum = rhs._checksum;
    _payloadprot = rhs._payloadprot;
    _headertype = rhs._headertype;
    _parameters = rhs._parameters;
  }

  return* this;
}

void MIPv6MobilityHeaderBase::info(char* buf)
{}

bool MIPv6MobilityHeaderBase::addMPar(MIPv6MHParameterBase* param)
{
  // check if the same parameter has already existed
  for (size_t i = 0; i < _parameters.size(); i++)
  {
    if (param->type() == _parameters[i]->type())
        return false;
  }

  _parameters.push_back(param);
  return true;
}

// MIPv6MHParameterBase definition

MIPv6MHParameterBase::MIPv6MHParameterBase(MIPv6MHParameterType type)
  : _type(type),
    _len(0)
{}

MIPv6MHParameterBase::~MIPv6MHParameterBase(void)
{}

MIPv6MHParameterBase& MIPv6MHParameterBase::operator=(MIPv6MHParameterBase& rhs)
{
  if (this != &rhs)
  {
    _type = rhs._type;
    _len = rhs._len;
  }

  return *this;
}

} // end namespace MobileIPv6

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
    @file MIPv6MHParameters.cc
	@brief MIPv6 Mobility Header Parameters
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.2.2 to 5.2.7
	@author Eric Wu
	@date 4/4/2002
*/

#include "MIPv6MHParameters.h"

namespace MobileIPv6
{

const Authenticator UNSPECIFIED_AUTHENTICATOR = { 0, 0, 0, 0 };

// Unique Identifier

MIPv6MHUniqueIdentifier::
MIPv6MHUniqueIdentifier(const int uniqueID)
  : MIPv6MHParameterBase(MIPv6MHPT_UI),
    _uniqueID(uniqueID)
{ 
  _len = 4;  
}

MIPv6MHUniqueIdentifier::~MIPv6MHUniqueIdentifier(void)
{}

MIPv6MHUniqueIdentifier& MIPv6MHUniqueIdentifier::
operator=(MIPv6MHUniqueIdentifier& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MHParameterBase::operator=(rhs);
    _uniqueID = rhs._uniqueID;
  }
  return *this;
}

// Alternate Care-of Address

MIPv6MHAlternateCareofAddress::MIPv6MHAlternateCareofAddress(const ipv6_addr& addr)
  : MIPv6MHParameterBase(MIPv6MHPT_ACoA),
    _acoa(addr)
{
  _len = 18;
}

MIPv6MHAlternateCareofAddress::~MIPv6MHAlternateCareofAddress(void)
{}

MIPv6MHAlternateCareofAddress& MIPv6MHAlternateCareofAddress::
operator=(MIPv6MHAlternateCareofAddress& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MHParameterBase::operator=(rhs);
    _acoa = rhs._acoa;
  }
  return *this;
}

// Nonce Indices

MIPv6MHNonceIndices::MIPv6MHNonceIndices(const int hni, const int coni)
  : MIPv6MHParameterBase(MIPv6MHPT_NI),
    _hni(hni),
    _coni(coni)
{
  _len = 6; 
}

MIPv6MHNonceIndices::~MIPv6MHNonceIndices(void)
{}

MIPv6MHNonceIndices& MIPv6MHNonceIndices::operator=(MIPv6MHNonceIndices& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MHParameterBase::operator=(rhs);
    _hni = rhs._hni;
    _coni = rhs._coni;
  }
  return *this;
}

// Authentication Data

MIPv6MHAuthenticationData::
MIPv6MHAuthenticationData(const int spi, Authenticator auth)
  : MIPv6MHParameterBase(MIPv6MHPT_Auth),
    _spi(spi),
    _auth(auth)
{
  _len = 18; 
}

MIPv6MHAuthenticationData::~MIPv6MHAuthenticationData(void)
{}

MIPv6MHAuthenticationData& MIPv6MHAuthenticationData::
operator=(MIPv6MHAuthenticationData& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MHParameterBase::operator=(rhs);
    _spi = rhs._spi;
    _auth = rhs._auth;
  }
  return *this;
}
 
} // end namespace MobileIPv6

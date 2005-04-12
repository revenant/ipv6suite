// -*- C++ -*-
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
    @file MIPv6MHParameters.h
    @brief MIPv6 Mobility Header Parameters
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.2.2 to 5.2.7
    @author Eric Wu
    @date 4/4/2002
*/

#ifndef __MIPv6MHPARAMETERS_H__
#define __MIPv6MHPARAMETERS_H__

#include "MIPv6MessageBase.h"
#include "ipv6_addr.h"

namespace MobileIPv6
{

// 96-bit crypotgraphic hash used in sec 5.2.7 (NOTE: manipulation of
// authenticator may well be in a seperate class)
struct Authenticator
{
  // each of the followings is "assumed" 24 bit long and therefore
  // bitmasking may be neccessary
  unsigned long low;
  unsigned long medium;
  unsigned long high;
  unsigned long extreme;
};

// initial value of authenticator
extern const Authenticator UNSPECIFIED_AUTHENTICATOR;

// class forward declaration

class MIPv6MHUniqueIdentifier;
class MIPv6MHAlternateCareofAddress;
class MIPv6MHNonceIndices;
class MIPv6MHAuthenticationData;

// typedef

typedef MIPv6MHUniqueIdentifier       UI;
typedef MIPv6MHAlternateCareofAddress ACoA;
typedef MIPv6MHNonceIndices           NI;
typedef MIPv6MHAuthenticationData     AuthD;

class MIPv6MHUniqueIdentifier : public MIPv6MHParameterBase
{
 public:
  MIPv6MHUniqueIdentifier(const int ui = 0);
  virtual ~MIPv6MHUniqueIdentifier(void);

  MIPv6MHUniqueIdentifier& operator=(MIPv6MHUniqueIdentifier&);
  operator int(void) const { return _uniqueID; }

  void setUniqueID(const int uniqueID) { _uniqueID = uniqueID; }
  int uniqueID(void) const { return _uniqueID; }

 private:
  int _uniqueID;
};

class MIPv6MHAlternateCareofAddress : public MIPv6MHParameterBase
{
 public:
  MIPv6MHAlternateCareofAddress(const ipv6_addr& addr = IPv6_ADDR_UNSPECIFIED);
  virtual ~MIPv6MHAlternateCareofAddress(void);

  MIPv6MHAlternateCareofAddress& operator=(MIPv6MHAlternateCareofAddress&);
  operator ipv6_addr(void) const { return _acoa; }

  void setAddress(ipv6_addr& acoa) { _acoa = acoa; }
  ipv6_addr address(void) const { return _acoa; }

 private:
  ipv6_addr _acoa;
};

class MIPv6MHNonceIndices : public MIPv6MHParameterBase
{
 public:
  MIPv6MHNonceIndices(const int hni = 0, const int coni = 0);
  virtual ~MIPv6MHNonceIndices(void);

  MIPv6MHNonceIndices& operator=(MIPv6MHNonceIndices&);

  void setHomeNI(const int hni) { _hni = hni; }
  int homeNI(void) const { return _hni; }

  void setCareofNI(const int coni) { _coni = coni; }
  int careofNI(void) const { return _coni; }

 private:
  int _hni;
  int _coni;
};

class MIPv6MHAuthenticationData : public MIPv6MHParameterBase
{
 public:
  MIPv6MHAuthenticationData(const int spi = 0,
                           Authenticator auth = UNSPECIFIED_AUTHENTICATOR);
  virtual ~MIPv6MHAuthenticationData(void);

  MIPv6MHAuthenticationData& operator=(MIPv6MHAuthenticationData&);

  void setSPI(const int spi) { _spi = spi; }
  int spi(void) const { return _spi; }

  void setAuthenticator(const Authenticator& auth) { _auth = auth; }
  const Authenticator& authenticator(void) const { return _auth; }

 private:
  int _spi;
  Authenticator _auth;
};

} // namespace

#endif // __MIPv6MHPARAMETERS_H__

// -*- C++ -*-
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
    @file MIPv6MessageBase.h
    @brief Abstract classes for Mobility Support IPv6 Messages
    @see draft-ietf-mobileip-ipv6-16.txt, section 5
    @author Eric Wu
    @date 2/4/2002
*/

#ifndef __MIPv6MESSAGEBASE_H__
#define __MIPv6MESSAGEBASE_H__

#include <list>
#include <vector>
#include <omnetpp.h>

#include "IPv6Datagram.h"
#include "HdrExtProc.h"

using namespace std;

namespace MobileIPv6
{

class MIPv6MobilityHeaderBase;
class MIPv6MHParameterBase;
class MIPv6DestOptionBase;
class MIPv6DestOptSubOptionBase;

typedef vector<MIPv6MHParameterBase*> Parameters;

// enumerated types for mobility header

enum MIPv6MobilityHeaderType
{
  MIPv6MHT_NONE = 0,

  MIPv6MHT_BR = 1, // binding request
  MIPv6MHT_HoTI = 2, // home test init
  MIPv6MHT_CoTI = 3, // care-of test init
  MIPv6MHT_HoT = 4, // home test message
  MIPv6MHT_CoT = 5, // care-of test message
  MIPv6MHT_BU = 6, // binding update
  MIPv6MHT_BA = 7, // binding acknowledgement
  MIPv6MHT_BM = 8 // biniding missing
};

// enumerated types for parameters of which are "encapsulated" in the
// mobility header

enum MIPv6MHParameterType
{
  MIPv6MHPT_NONE = 8888,

  MIPv6MHPT_Pad1 = 0, // pad1 (NOT IMPLEMENTED YET!)
  MIPv6MHPT_PadN = 1, // padN (NOT IMPLEMENTED YET!)
  MIPv6MHPT_UI = 2, // unique identifier
  MIPv6MHPT_ACoA = 3, // alternate care-of address
  MIPv6MHPT_NI = 4, // nonce indices
  MIPv6MHPT_Auth = 5 // authentication data
};

/**
   class@ MIPv6MHParameterBase

   This is a template base class which holds common functionality for
   all MIPv6 parameters
 */

class MIPv6MHParameterBase
{
 public:
  virtual ~MIPv6MHParameterBase(void);

  MIPv6MHParameterType type(void) const { return _type; }
  int length() const { return _len; }

 protected:
  MIPv6MHParameterBase(MIPv6MHParameterType type = MIPv6MHPT_NONE);
  MIPv6MHParameterBase& operator=(MIPv6MHParameterBase&);

 protected:
  MIPv6MHParameterType _type;
  int _len;
};

/**
   class@ MIPv6MobilityHeaderBase

   This is a template base class which holds common functionality for
   all MIPv6 mobility headers
 */

class MIPv6MobilityHeaderBase : public cMessage
{
 public:
  virtual ~MIPv6MobilityHeaderBase(void);

  MIPv6MobilityHeaderBase& operator=(const MIPv6MobilityHeaderBase& );
  virtual MIPv6MobilityHeaderBase* dup() const = 0;
  virtual const char* className() const { return "MIPv6MobilityHeaderBase"; }
  virtual void info(char* buf);

  // add an instance of mobility header parameter into the list
  virtual bool addMPar(MIPv6MHParameterBase*);

  void calculateChecksum(void)
    {
      // TODO: leave it for future implementation
    }
  int checksum(void) const { return _checksum; }

  // According to sec 5.1.1, the payload protocol MUST be set to
  // NO_NXTHDR (59)
  IPProtocolId payload_prot(void) const { return _payloadprot; }

  // NOT ALLOWED to call this function yet
  void setPayloadProt(IPProtocolId prot)
    {
      _payloadprot = prot;
    }

  MIPv6MobilityHeaderType header_type(void) const { return _headertype; }

  MIPv6MHParameterBase* parameter(MIPv6MHParameterType t) const
    {
      for ( size_t i = 0; i < _parameters.size(); i++)
        if ( _parameters[i]->type() == t )
          return _parameters[i];

      return 0;
    }

  // TODO: add parameter
//  void addParameter(MIPv6MHParameterBase* parameter)
//    {}

  // return physical length in unit of octets of packet that is from real data
//  size_t physicalLen(void) const { return _physicalLen; }

  // set the physical length by calculating the length of packet that is
  // from real data in unit of octets
//  void setPhysicalLen(size_t l) { _physicalLen = l; }

  // return phyical length in unit of 8 octets of packet that is from
  // real data
//  size_t physicalLenInOctet(void) const { return _physicalLen / 8; }

 protected:
  MIPv6MobilityHeaderBase(MIPv6MobilityHeaderType headertype = MIPv6MHT_NONE,
                         int len = 0);
  MIPv6MobilityHeaderBase(const MIPv6MobilityHeaderBase& src);

 protected:
  IPProtocolId _payloadprot;

  // mobility header (MH) Type
  MIPv6MobilityHeaderType _headertype;

  int _checksum;

  // mobility header parameters
  Parameters _parameters;

// private:
//  // actual length calculated from real data
//  size_t _physicalLen;
};

} // end namespace

#endif // __MIPv6MESSAGEBASE_H__

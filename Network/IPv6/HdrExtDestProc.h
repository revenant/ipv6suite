// -*- C++ -*-
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
    @file HdrExtDestProc.h
    @brief  Destination Options header processing
    @author Johnny Lai
    @date 04.4.02
*/

#ifndef HDREXTDESTPROC_H
#define HDREXTDESTPROC_H

#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR

#ifndef IPV6HEADERS_H
#include "IPv6Headers.h"
#endif //IPV6HEADERS_H

#ifndef HDREXTPROC_H
#include "HdrExtProc.h"
#endif //HDREXTPROC_H

#ifndef IPV6DATAGRAM_H
#include "IPv6Datagram.h"
#endif //IPV6DATAGRAM_H

using namespace std;

/**
   @class IPv6TLVOptionBase

   @brief Abstract base class which holds common functionality for all TLV
   encoded options
*/

class IPv6TLVOptionBase
{

public:

  enum TLVOptType
  {
    MIPv6_HOME_ADDRESS_OPT = 201,

    // sub option types
    MIPv6Sub_Pad1 = 0, // (NOT IMPLEMENTED YET!)
    MIPv6Sub_PadN = 1 // (NOT IMPLEMENTED YET!)
  };

  virtual ~IPv6TLVOptionBase();

  virtual IPv6TLVOptionBase* dup() const = 0;

  // force a "real" destination option to override this function
  virtual bool processOption(cSimpleModule* mod, IPv6Datagram* pdu) = 0;

  TLVOptType type() { return _type; }

  /**
     a unit of 8 octets for the length of the option including the sub-options
  */
  int length()
    {
      int sLen = 0;

      for ( size_t i = 0; i < _subOptions.size(); i ++ )
        sLen += _subOptions[i]->length();

      return static_cast<int>((_len+sLen) / 8);
    }

protected:
  IPv6TLVOptionBase(TLVOptType otype, unsigned char len);

  TLVOptType  _type;

  typedef vector<IPv6TLVOptionBase*> Options;
  Options _subOptions;

  //Units of octets
  int _len;
};

typedef vector<IPv6TLVOptionBase*> DestOptions;

/**
 * @class HdrExtDestProc
 * @brief Destination Options Extension header
 *
 * Acts as a container for the various options and the processing of options in
 * a abstract manner by providing the interface below.
 */
class HdrExtDestProc:public HdrExtProc
{

public:

  //@name constructors and operators
  //@{
  explicit HdrExtDestProc();

  explicit HdrExtDestProc(const HdrExtDestProc& src);

  ~HdrExtDestProc();

  virtual const char* className() const { return "HdrExtDestProc"; }

  bool operator==(const HdrExtDestProc& rhs);

  HdrExtDestProc& operator=(const HdrExtDestProc& rhs);

  virtual std::ostream& operator<<(std::ostream& os);
  //@}

  virtual HdrExtDestProc* dup() const
    {
      return new HdrExtDestProc(*this);
    }

  //@name Overridden HdrExtProc functions
  //@{
  ///processed at every dest in routing header
  virtual bool processHeader(cSimpleModule* mod, IPv6Datagram* pdu);
  //@}

  /// We will take ownership of this pointer
  bool addOption(const IPv6TLVOptionBase* opt);

  // return a pointer to a destination option
  IPv6TLVOptionBase* getOption(IPv6TLVOptionBase::TLVOptType type);

protected:
  ipv6_ext_opts_hdr& opt_hdr;
  DestOptions destOpts;
};


/**
 * @class IPv6SubOptionBase
 *
 * @brief Currently only Pad1 and PadN defined (unimplemented)
 */

class IPv6SubOptionBase
{
  // enumerated types for destination option sub-options

  enum SubOptType
  {
    DOSOT_Pad1 = 0,
    DOSOT_PadN = 1
  };

public:
  bool operator==(const IPv6SubOptionBase& rhs) const
    {
      return _type == rhs._type;
    }

  SubOptType type() { return _type; }

protected:
  SubOptType _type;
};

#endif //HDREXTDESTPROC_H

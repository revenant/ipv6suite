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
    @file MIPv6MobilityHeaders.h
    @brief MIPv6 Mobility Headers
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.1.2 to 5.1.8
    @author Eric Wu
    @date 4/4/2002
*/

#ifndef MIPv6MOBILITYHEADERS_H
#define MIPv6MOBILITYHEADERS_H

#include "MIPv6MessageBase.h"
#include "MIPv6MHParameters.h"
#include "ipv6_addr.h"

namespace MobileIPv6
{

// constants

// use these constants accordingly in the fields of BA when BU was rejected
extern const unsigned int UNDEFINED_REFRESH;
extern const unsigned int UNDEFINED_EXPIRES;
extern const unsigned int UNDEFINED_SEQ;

struct bit_64
{
  unsigned int high;
  unsigned int low;
};

extern bool operator==(const bit_64& lhs, const bit_64& rhs);
extern bool operator!=(const bit_64& lhs, const bit_64& rhs);

extern const bit_64 UNSPECIFIED_BIT_64;

class MIPv6MHBindingRequest : public MIPv6MobilityHeaderBase
{
 public:
  MIPv6MHBindingRequest(void);
  MIPv6MHBindingRequest(const MIPv6MHBindingRequest& src);
  virtual ~MIPv6MHBindingRequest(void);
  virtual MIPv6MHBindingRequest& operator=(const MIPv6MHBindingRequest& rhs);

  virtual MIPv6MHBindingRequest* dup() const
    { return new MIPv6MHBindingRequest(*this); }
  virtual const char* className() const
    { return "MIPv6MHBindingRequest"; }
  virtual void info(char* buf);

  virtual bool addMPar(MIPv6MHParameterBase* param);

 private:
  // reserved
};

class MIPv6MHTestInit : public MIPv6MobilityHeaderBase
{
 public:
  MIPv6MHTestInit(MIPv6MobilityHeaderType _headertype, const bit_64& __cookie);
  MIPv6MHTestInit(const MIPv6MHTestInit& src);
  virtual ~MIPv6MHTestInit(void);
  virtual MIPv6MHTestInit& operator=(const MIPv6MHTestInit& rhs);

  virtual MIPv6MHTestInit* dup() const
    { return new MIPv6MHTestInit(*this); }
  virtual const char* className() const
    {
      if (_headertype==MIPv6MHT_HoTI)
        return "HoTI";
      else if (_headertype==MIPv6MHT_CoTI)
        return "CoTI";
      else
        assert(false);
    }
  virtual void info(char* buf);

  virtual bool addMPar(MIPv6MHParameterBase* param);

  bit_64 cookie;

 private:
  // reserved
};

class MIPv6MHTest : public MIPv6MobilityHeaderBase
{
 public:
  MIPv6MHTest(MIPv6MobilityHeaderType _headertype, int hni,
              const bit_64& __cookie = UNSPECIFIED_BIT_64,
              const bit_64& __token = UNSPECIFIED_BIT_64);
  MIPv6MHTest(const MIPv6MHTest& src);
  virtual ~MIPv6MHTest(void);
  virtual MIPv6MHTest& operator=(const MIPv6MHTest& rhs);

  virtual MIPv6MHTest* dup() const
    { return new MIPv6MHTest(*this); }
  virtual const char* className() const
    {
      if (_headertype==MIPv6MHT_HoT)
        return "HoT";
      else if (_headertype==MIPv6MHT_CoT)
        return "CoT";
      else
        assert(false);
    }
  virtual void info(char* buf);

  virtual bool addMPar(MIPv6MHParameterBase* param);

  void setHomeNI(const int hni) { _hni = hni; }
  int homeNI(void) const { return _hni; }

 private:
  int _hni;

 public:
  bit_64 cookie;
  bit_64 token;

};

class MIPv6MHBindingUpdate : public MIPv6MobilityHeaderBase
{
 public:
  MIPv6MHBindingUpdate(bool ack = false, bool homereg = false,
                       bool saonly = false, bool dad = false,
                       unsigned int seq = 0,
                       unsigned int expires = 0,
                       const ipv6_addr& ha = IPv6_ADDR_UNSPECIFIED
#ifdef USE_HMIP
                       , bool map = false
#endif
                       , bool cellSignaling = false
                       ,cModule* senderMod = 0);
  MIPv6MHBindingUpdate(const MIPv6MHBindingUpdate& src);
  virtual ~MIPv6MHBindingUpdate(void);
  virtual MIPv6MHBindingUpdate& operator=(const MIPv6MHBindingUpdate& rhs);

  virtual MIPv6MHBindingUpdate* dup() const
    { return new MIPv6MHBindingUpdate(*this); }
  virtual const char* className() const
    { return "MIPv6MHBindingUpdate"; }
  virtual void info(char* buf);

  virtual bool addMPar(MIPv6MHParameterBase* param);

  bool cellSignaling(void) const  { return _cellSignaling; }
  bool ack(void) const  { return _ack; }
  bool homereg(void) const  { return _homereg; }
  bool saonly(void) const  { return _saonly; }
  bool dad(void) const { return _dad; }

#ifdef USE_HMIP
  bool mapreg(void) const { return _map; }
#endif

  unsigned int sequence(void) const  { return _seq; }
  unsigned int expires(void) const  { return _expires; }
  const ipv6_addr& ha(void) const  { return _ha; }

  void setSequence(unsigned int sequence) { _seq = sequence; }

  cModule* senderModule(void) const { return _senderMod; }

 private:
  bool _ack, _homereg, _saonly, _dad, _cellSignaling;
  unsigned int _seq;
  unsigned int _expires;
  ipv6_addr _ha; // home address
#ifdef USE_HMIP
  bool _map;
#endif
  cModule* _senderMod;

};

class MIPv6MHBindingAcknowledgement : public MIPv6MobilityHeaderBase
{
 public:
    // binding acknowledgement status; see sec 5.1.8
  enum BAStatus
    {
      BAS_ACCEPTED = 0,
      BAS_PREFIX_DISC = 1,
      BAS_REASON_UNSPECIFIED = 128,
      //BAS_ADMIN_PROHIBITED = 130,
      BAS_INSUFF_RESOURCE = 130,
      BAS_HR_NOT_SUPPORTED = 131,
      BAS_NOT_HOME_SUBNET=132,
      BAS_NOT_HA_FOR_MN = 133,
      BAS_DAD_FAILED = 134,
      BAS_SEQ_OUT_OF_WINDOW = 135,
      BAS_UNREC_HONI = 136,
      BAS_UNREC_CONI= 137,
      BAS_UNREC_BOTHNI = 138,
      BAS_REG_TYPE_CHANGE_DIS = 139,
      BAS_ROU_DUE_TO_LOW_TRAFFIC = 142,
      BAS_INVALID_AUTH = 143,
      BAS_TOO_OLD_HONI = 144,
      BAS_TOO_OLD_CONI = 145
    };

  MIPv6MHBindingAcknowledgement(const BAStatus status = BAS_REASON_UNSPECIFIED,
                                const unsigned int seq = 0,
                                const unsigned int expires = 0,
                                const unsigned int refresh = 0);
  MIPv6MHBindingAcknowledgement(const MIPv6MHBindingAcknowledgement& src);
  virtual ~MIPv6MHBindingAcknowledgement(void);
  virtual MIPv6MHBindingAcknowledgement& operator=(const MIPv6MHBindingAcknowledgement& rhs);

  virtual MIPv6MHBindingAcknowledgement* dup() const
    { return new MIPv6MHBindingAcknowledgement(*this); }
  virtual const char* className() const
    { return "MIPv6MHBindingAcknowledgement"; }
  virtual void info(char* buf);

  virtual bool addMPar(MIPv6MHParameterBase* param);

  BAStatus status(void) const { return _status; }
  unsigned int sequence(void) const { return _seq; }
  unsigned int lifetime(void) const { return _expires; }
  unsigned int refresh(void) const { return _refresh; }

  // I don't know if I should create functions to set explicitely all
  // of BA's attributes yet so I leave it blank for the moment being

  // ...

 private:
  BAStatus _status;
  unsigned int _seq;
  unsigned int _expires;
  unsigned int _refresh;
};

class MIPv6MHBindingMissing : public MIPv6MobilityHeaderBase
{
 public:
  MIPv6MHBindingMissing(const ipv6_addr& ha = IPv6_ADDR_UNSPECIFIED);
  MIPv6MHBindingMissing(const MIPv6MHBindingMissing& src);
  virtual ~MIPv6MHBindingMissing(void);
  virtual MIPv6MHBindingMissing& operator=(const MIPv6MHBindingMissing& rhs);

  virtual MIPv6MHBindingMissing* dup() const
    { return new MIPv6MHBindingMissing(*this); }
  virtual const char* className() const
    { return "MIPv6MHBindingMissing"; }
  virtual void info(char* buf);

  virtual bool addMPar(MIPv6MHParameterBase* param);

  const ipv6_addr& ha(void) const { return _ha; }

  // should I explicitely set the home addrss of BM?

  // ...

 private:
  ipv6_addr _ha; // home address
};

} // end namespace

#endif // MIPv6MOBILITYHEADERS_H

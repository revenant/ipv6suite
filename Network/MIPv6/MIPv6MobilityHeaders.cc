//
// Copyright (C) 2002, 2004 CTIE, Monash University 
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
    @file MIPv6MobilityHeaders.cc
	@brief MIPv6 Mobility Headers
    @see draft-ietf-mobileip-ipv6-16.txt, section 5.1.2 to 5.1.8
	@author Eric Wu
	@date 4/4/2002

    @todo Implement info functions (define info in terms of writeContents so
    that operator<< can use that too)
*/

#include "MIPv6MobilityHeaders.h"

using std::list;

namespace MobileIPv6
{

const unsigned int UNDEFINED_REFRESH = 0xFFFFFFFF;
const unsigned int UNDEFINED_EXPIRES = 0xFFFFFFFF;
const unsigned int UNDEFINED_SEQ = 0xFFFF;

bool operator==(const bit_64& lhs, const bit_64& rhs)
{
  return (lhs.high == rhs.high && lhs.low == rhs.low);
}

bool operator!=(const bit_64& lhs, const bit_64& rhs)
{
  return !(lhs.high == rhs.high && lhs.low == rhs.low);
}
  
const bit_64 UNSPECIFIED_BIT_64 = {0 ,0};

// Binding Request

MIPv6MHBindingRequest::MIPv6MHBindingRequest(void)
  : MIPv6MobilityHeaderBase(MIPv6MHT_BR, 2)
{
  setName("BR");
}

MIPv6MHBindingRequest::MIPv6MHBindingRequest(const MIPv6MHBindingRequest& src)
{
  setName(src.name());
  operator=(src);
}

MIPv6MHBindingRequest::~MIPv6MHBindingRequest(void)
{}

MIPv6MHBindingRequest& MIPv6MHBindingRequest::
operator=(const MIPv6MHBindingRequest& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MobilityHeaderBase::operator=(rhs);
  }
  
  return *this;   
}

bool MIPv6MHBindingRequest::addMPar(MIPv6MHParameterBase* param)
{
  if (param->type() != MIPv6MHPT_UI && param->type() != MIPv6MHPT_Auth)
    return false;

  return MIPv6MobilityHeaderBase::addMPar(param);
}

void MIPv6MHBindingRequest::info(char* buf)
{}

// Test Init Message

MIPv6MHTestInit::MIPv6MHTestInit(MIPv6MobilityHeaderType _headertype, const bit_64& __cookie)
  : MIPv6MobilityHeaderBase(_headertype, 16), cookie(__cookie)
{
  assert(_headertype == MIPv6MHT_HoTI || _headertype == MIPv6MHT_CoTI);
  if ( _headertype == MIPv6MHT_HoTI)
    setName("HoTI");
  else
    setName("CoTI");
}

MIPv6MHTestInit::MIPv6MHTestInit(const MIPv6MHTestInit& src)
{
  operator=(src);
}

MIPv6MHTestInit::~MIPv6MHTestInit(void)
{}

MIPv6MHTestInit& MIPv6MHTestInit::
operator=(const MIPv6MHTestInit& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MobilityHeaderBase::operator=(rhs);
    cookie = rhs.cookie;
  }
  
  return *this;   
}

bool MIPv6MHTestInit::addMPar(MIPv6MHParameterBase* param)
{
  if (param->type() != MIPv6MHPT_UI)
    return false;

  return MIPv6MobilityHeaderBase::addMPar(param);
}

void MIPv6MHTestInit::info(char* buf)
{}

// Test Message

MIPv6MHTest::MIPv6MHTest(MIPv6MobilityHeaderType _headertype, int hni, const bit_64& __cookie, const bit_64& __token)
  : MIPv6MobilityHeaderBase(_headertype, 24),
    _hni(hni),
    cookie(__cookie),
    token(__token)
{
  assert(_headertype == MIPv6MHT_HoT || _headertype == MIPv6MHT_CoT);
  if ( _headertype == MIPv6MHT_HoT)
    setName("HoT");
  else
    setName("CoT");
}

MIPv6MHTest::MIPv6MHTest(const MIPv6MHTest& src)
{
  operator=(src);
}

MIPv6MHTest::~MIPv6MHTest(void)
{}

MIPv6MHTest& MIPv6MHTest::
operator=(const MIPv6MHTest& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MobilityHeaderBase::operator=(rhs);
    _hni = rhs.homeNI();
    cookie = rhs.cookie;
    token = rhs.token;
  }
  
  return *this;   
}

bool MIPv6MHTest::addMPar(MIPv6MHParameterBase* param)
{
  // DON'T KNOW YET
  return false;
}

void MIPv6MHTest::info(char* buf)
{}

// Binding Update

MIPv6MHBindingUpdate::MIPv6MHBindingUpdate(bool ack, bool homereg, 
                                           bool saonly, bool dad,
                                           unsigned int seq, 
                                           unsigned int expires,
                                           const ipv6_addr& ha
#ifdef USE_HMIP
                                           , bool map
#endif
                                           ,cModule* senderMod)
  : MIPv6MobilityHeaderBase(MIPv6MHT_BU, 24),
    _ack(ack), _homereg(homereg), _saonly(saonly), _dad(dad),
    _seq(seq), _expires(expires), _ha(ha)
#ifdef USE_HMIP
    ,_map(map)
#endif
    ,_senderMod(senderMod)
{
    setName("BU");
}

MIPv6MHBindingUpdate::MIPv6MHBindingUpdate(const MIPv6MHBindingUpdate& src)
{
  operator=(src);
}

MIPv6MHBindingUpdate::~MIPv6MHBindingUpdate(void)
{}

MIPv6MHBindingUpdate& MIPv6MHBindingUpdate::
operator=(const MIPv6MHBindingUpdate& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MobilityHeaderBase::operator=(rhs);
    _ack = rhs.ack();
    _homereg = rhs.homereg();
    _saonly = rhs.saonly();
    _dad = rhs.dad();
    _seq = rhs.sequence();
    _expires = rhs.expires();
    _ha = rhs.ha();
#ifdef USE_HMIP
    _map = rhs._map;
#endif //USE_HMIP
    _senderMod = rhs._senderMod;
  }
  
  return *this;   
}

bool MIPv6MHBindingUpdate::addMPar(MIPv6MHParameterBase* param)
{
  if (param->type() != MIPv6MHPT_NI && param->type() != MIPv6MHPT_Auth &&
    param->type() != MIPv6MHPT_UI && param->type() != MIPv6MHPT_ACoA)
    return false;

  return MIPv6MobilityHeaderBase::addMPar(param);
}

void MIPv6MHBindingUpdate::info(char* buf)
{}

// Binding Acknowledgement

MIPv6MHBindingAcknowledgement::
MIPv6MHBindingAcknowledgement(const BAStatus status,
                              const unsigned int seq, 
                              const unsigned int expires,
                              const unsigned int refresh)
  : MIPv6MobilityHeaderBase(MIPv6MHT_BA, 14),
    _status(status),
    _seq(seq),
    _expires(expires),
    _refresh(refresh)
{
  setName("BA");
}

MIPv6MHBindingAcknowledgement::MIPv6MHBindingAcknowledgement(const MIPv6MHBindingAcknowledgement& src)
{
  operator=(src);
}

MIPv6MHBindingAcknowledgement::~MIPv6MHBindingAcknowledgement(void)
{}

MIPv6MHBindingAcknowledgement& MIPv6MHBindingAcknowledgement::
operator=(const MIPv6MHBindingAcknowledgement& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MobilityHeaderBase::operator=(rhs);
    _status = rhs._status;
    _seq = rhs.sequence();
    _expires = rhs.lifetime();
    _refresh = rhs.refresh();
  }
  
  return *this;   
}

bool MIPv6MHBindingAcknowledgement::addMPar(MIPv6MHParameterBase* param)
{
  if (param->type() != MIPv6MHPT_Auth)
    return false;

  return MIPv6MobilityHeaderBase::addMPar(param);
}

void MIPv6MHBindingAcknowledgement::info(char* buf)
{}

// Binding Missing

MIPv6MHBindingMissing::MIPv6MHBindingMissing(const ipv6_addr& ha)
  : MIPv6MobilityHeaderBase(MIPv6MHT_BM, 22),
    _ha(ha)
{
  setName("BM");
}

MIPv6MHBindingMissing::MIPv6MHBindingMissing(const MIPv6MHBindingMissing& src)
{
  operator=(src);
}

MIPv6MHBindingMissing::~MIPv6MHBindingMissing(void)
{}

MIPv6MHBindingMissing& MIPv6MHBindingMissing::
operator=(const MIPv6MHBindingMissing& rhs)
{
  if (this != &rhs)
  {    
    MIPv6MobilityHeaderBase::operator=(rhs);
    _ha = rhs.ha();
  }
  
  return *this;   
}

bool MIPv6MHBindingMissing::addMPar(MIPv6MHParameterBase* param)
{
  // don't know yet
  return false;
}

void MIPv6MHBindingMissing::info(char* buf)
{}
 
} // end namespace MobileIPv6

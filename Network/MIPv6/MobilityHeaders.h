// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file MobilityHeaders.h
 * @author Johnny Lai
 * @date 15 Oct 2006
 *
 * @brief Definition of Mobile IPv6 Headers acc. to Sec. 6.1 of RFC3775
 *
 */

#ifndef MOBILITYHEADERS_H
#define MOBILITYHEADERS_H

#ifndef _MOBILITYHEADER_M_H_
#include "MobilityHeader_m.h"
#endif //_MOBILITYHEADERBASE_H_

#ifndef _MOBILITYOPTIONS_M_H_
#include "MobilityOptions_m.h"
#endif //_MOBILITYOPTIONS_M_H_

class HOTI : public HOTI_Base
{
public:
  HOTI(const char *name="HOTI", int kind=MIPv6MHT_HOTI) : HOTI_Base(name,kind) 
  {
    setByteLength(8 + byteLength());
    setHomeCookie(rand());
  }
  HOTI(const HOTI& other) : HOTI_Base(other.name()) {operator=(other);}
  HOTI& operator=(const HOTI& other) {HOTI_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new HOTI(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from HOTI_Base
  
};

class COTI : public COTI_Base
{
public:
  COTI(const char *name="COTI", int kind=MIPv6MHT_COTI) : COTI_Base(name,kind) 
  {
    setByteLength(byteLength() + 8);
    setCareOfCookie(rand());
  }
  COTI(const COTI& other) : COTI_Base(other.name()) {operator=(other);}
  COTI& operator=(const COTI& other) {COTI_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new COTI(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from COTI_Base
};

class HOT : public HOT_Base
{
public:
  HOT(int cookie, u_int16 hni, const char *name="HOT", int kind=MIPv6MHT_HOT) :
    HOT_Base(name,kind) 
  {
    setHni(hni);
    setHomeCookie(cookie);
    setByteLength(byteLength() + 16);
  }
  HOT(const HOT& other) : HOT_Base(other.name()) {operator=(other);}
  HOT& operator=(const HOT& other) {HOT_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new HOT(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from HOT_Base
};

class COT : public COT_Base
{
public:
  COT(int cookie, u_int16 coni, const char *name="COT", int kind=MIPv6MHT_COT) : 
    COT_Base(name,kind) 
  {
    setConi(coni);
    setCareOfCookie(cookie);
    setByteLength(byteLength() + 16);
  }
  COT(const COT& other) : COT_Base(other.name()) {operator=(other);}
  COT& operator=(const COT& other) {COT_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new COT(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from COT_Base
};

class BU : public BU_Base
{
public:
  BU(bool ack, bool homereg, bool mapreg, u_int16 sequence, u_int16 expires, const char *name="BU", int kind=MIPv6MHT_BU) : BU_Base(name,kind) 
  {
    setAck(ack);
    setHomereg(homereg);
    setMapreg(mapreg);
    setSequence(sequence);
    setExpires(expires);
    setByteLength(4 + byteLength());
  }
  BU(const BU& other) : BU_Base(other.name()) {operator=(other);}
  BU& operator=(const BU& other) {BU_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new BU(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from BU_Base

  void addOption(MobilityOptionBase* op) 
  {
    assert(op->kind() == MOPT_NI || op->kind() == MOPT_AUTH ||
	   op->kind() == MOPT_ACoA);
    MobilityHeaderBase::addOption(op);
  }
};

class BA : public BA_Base
{
public:
  BA(BAStatus status = BAS_ACCEPTED, u_int16 sequence = 0, u_int16 lifetime = 0, const char *name="BA", int kind= MIPv6MHT_BA  ) : BA_Base(name,kind) 
  {
    setByteLength(4 + byteLength());
    setStatus(status);
    setSequence(sequence);
    setLifetime(lifetime);
  }
  BA(const BA& other) : BA_Base(other.name()) {operator=(other);}
  BA& operator=(const BA& other) {BA_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new BA(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from BA_Base

  void addOption(MobilityOptionBase* op) 
  {
    assert(op->kind() == MOPT_BRA || op->kind() == MOPT_AUTH);
    MobilityHeaderBase::addOption(op);
  }
};

class BE : public BE_Base
{
public:
  
/**
   @param hoa address in home address dest option or can be unsepcified 
   @param status true (2) means uncrecognized Mobility Header and false (1) is unknown binding for hoa dest opt
*/

  BE(int status = 1, ipv6_addr hoa = IPv6_ADDR_UNSPECIFIED,
     const char *name="BE", int kind= MIPv6MHT_BE) : BE_Base(name,kind) 
  {
    setStatus(status == 2);
    setByteLength(16 + byteLength());
  }
  BE(const BE& other) : BE_Base(other.name()) {operator=(other);}
  BE& operator=(const BE& other) {BE_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new BE(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from BE_Base  
};


#endif /* MOBILITYHEADERS_H */


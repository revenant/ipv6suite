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
 * @file MIPv6MobilityOptions.h
 * @author Johnny Lai
 * @date 14 Oct 2006
 *
 * @brief Definition of class Mobility Options as defined in RFC3775 Sec. 6.2
 *
 */

#ifndef MIPV6MOBILITYOPTIONS_H
#define MIPV6MOBILITYOPTIONS_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#include "MobilityOptions_m.h"

class MIPv6OptBRA : public MIPv6OptBRA_Base
{
  public:
  MIPv6OptBRA(const char *name="Binding Request Advice", int kind=2):
    MIPv6OptBRA_Base(name,kind), interval_var(0)
  {
    setByteLength(6);
  }
  MIPv6OptBRA(const MIPv6OptBRA& other) : MIPv6OptBRA_Base(other.name()) {operator=(other);}
  MIPv6OptBRA& operator=(const MIPv6OptBRA& other) {MIPv6OptBRA_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new MIPv6OptBRA(*this);}
  virtual void setInterval(const u_int16& interval_var)
  {
    assert(interval_var > 0);
    MIPv6OptBRA_Base::setInterval(interval_var);
  }
};

class MIPv6OptACoA : public MIPv6OptACoA_Base
{
  public:
  MIPv6OptACoA(const char *name="Alternate CoA", int kind=3):
    MIPv6OptACoA_Base(name,kind)
  {
    setByteLength(20);
    setAcoa(IPv6_ADDR_UNSPECIFIED);
  }
  MIPv6OptACoA(const MIPv6OptACoA& other) : MIPv6OptACoA_Base(other.name()) {operator=(other);}
  MIPv6OptACoA& operator=(const MIPv6OptACoA& other) {MIPv6OptACoA_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new MIPv6OptACoA(*this);}
};

class MIPv6OptNI : public MIPv6OptNI_Base
{
  public:
  MIPv6OptNI(const char *name="Nonce Indices", int kind=4):
    MIPv6OptNI_Base(name,kind)
  {
    setByteLength(8);
    setHni(0);
    setConi(0);
  }
  MIPv6OptNI(const MIPv6OptNI& other) : MIPv6OptNI_Base(other.name()) {operator=(other);}
  MIPv6OptNI& operator=(const MIPv6OptNI& other) {MIPv6OptNI_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new MIPv6OptNI(*this);}
};

class MIPv6OptBAD : public MIPv6OptBAD_Base
{
  public:
  MIPv6OptBAD(const char *name="Binding Authorization Data", int kind=5):
    MIPv6OptBAD_Base(name,kind)
  {
    setByteLength(20);
  }
  MIPv6OptBAD(const MIPv6OptBAD& other) : MIPv6OptBAD_Base(other.name()) {operator=(other);}
  MIPv6OptBAD& operator=(const MIPv6OptBAD& other) {MIPv6OptBAD_Base::operator=(other); return *this;}
  virtual cPolymorphic *dup() const {return new MIPv6OptBAD(*this);}
};

#endif /* MIPV6MOBILITYOPTIONS_H */


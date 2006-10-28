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
 * @file MobilityHeaderBase.h
 * @author Johnny Lai
 * @date 14 Oct 2006
 *
 * @brief Definition of class MobilityHeaderBase
 *
 */

#ifndef MOBILITYHEADERBASE_H
#define MOBILITYHEADERBASE_H


#include "MobilityHeaderBase_m.h"

#ifndef VECTOR
#define VECTOR
#include <vector>
#endif

class MobilityOptionBase;

/**
 * @class MobilityHeaderBase
 *
 * @brief 
 *
 * detailed description
 */

class MobilityHeaderBase: public MobilityHeaderBase_Base
{
public:
  //@name constructors, destructors and operators
  //@{
  MobilityHeaderBase& operator=(const MobilityHeaderBase& other) {MobilityHeaderBase_Base::operator=(other); return *this;}

protected:
  MobilityHeaderBase(const char *name=NULL, int kind=0) : MobilityHeaderBase_Base(name,kind) 
  {
    setByteLength(8);
  }
  MobilityHeaderBase(const MobilityHeaderBase& other) : MobilityHeaderBase_Base(other.name()) {operator=(other);}

  //@}

  ///@name Overidden cMessage functions
  //@{
public:
  virtual cPolymorphic *dup() const {return new MobilityHeaderBase(*this);}
  // ADD CODE HERE to redefine and implement pure virtual functions from MobilityHeaderBase_Base  

  //Cater for alignment on multiple of 8 octets
  void setByteLength(long l)  {
    while (l % 8 != 0)
    {
      l++;
    }
    cMessage::setByteLength(l);
  }
  //@}

  virtual void addMobilityOption(MobilityOptionBase* op);
  
  MobilityOptionBase* mobilityOption(int t) const;


  
 protected:
  std::vector<MobilityOptionBase* > mobilityOptions;
 private:

};


#endif /* MOBILITYHEADERBASE_H */


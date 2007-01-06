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
 * @file   MobilityHeaderBase.cc
 * @author Johnny Lai
 * @date   15 Oct 2006
 *
 * @brief  Implementation of MobilityHeaderBase
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "MobilityHeaderBase.h"
#include "MobilityHeaders.h"

MobilityHeaderBase::MobilityHeaderBase(const char *name, int kind) : 
  MobilityHeaderBase_Base(name,kind) 
{
  setByteLength(8);
  WATCH_VECTOR(mobilityOptions);
}


MobilityHeaderBase::~MobilityHeaderBase()
{
  //TODO delete options but take release from ownership
  for ( size_t i = 0; i < mobilityOptions.size(); i++)
    delete mobilityOptions[i];
  mobilityOptions.clear();
}

MobilityHeaderBase& MobilityHeaderBase::operator=(const MobilityHeaderBase& other) 
{
  MobilityHeaderBase_Base::operator=(other);
  for ( size_t i = 0; i < other.mobilityOptions.size(); i++)
    addOption(dynamic_cast<MobilityOptionBase*>(other.mobilityOptions[i]->dup()));
  return *this;
}

void MobilityHeaderBase::padHeader()  
{
  unsigned int l = this->byteLength();
  while (l % 8 != 0)
    {
      l++;
    }
  this->setByteLength(l);
}

void  MobilityHeaderBase::addOption(MobilityOptionBase* op)
{
  if (kind() == MIPv6MHT_BE || kind() == MIPv6MHT_COT || kind() == MIPv6MHT_HOT
      || kind() == MIPv6MHT_COTI || kind() == MIPv6MHT_HOTI 
      || kind() == MIPv6MHT_BRR)
    assert(false);
  mobilityOptions.push_back(op);
  if (kind() == MIPv6MHT_BA && op->kind() == MOPT_AUTH)
    assert(op->byteLength() > 0);
  assert(op->byteLength() > 0);
  setByteLength(byteLength() + op->byteLength());
}

MobilityOptionBase*  MobilityHeaderBase::mobilityOption(MobilityOptType type) const
{
  for ( size_t i = 0; i < mobilityOptions.size(); i++)
    if ( mobilityOptions[i]->kind() == type )
      return mobilityOptions[i];
    
  return 0;
}

std::ostream& operator<<(std::ostream& os, const MobilityHeaderBase& mhb)
{
  os<<" mobheader "; 
  if (mhb.kind() == MIPv6MHT_HOTI)
  {
    os<<"hc="<<((HOTI&)mhb).homeCookie();
  }
  else if (mhb.kind() == MIPv6MHT_COTI)
  {
    os<<"coc="<<((COTI&)mhb).careOfCookie();
  }
  else if (mhb.kind() == MIPv6MHT_HOT)
  {
    HOT& hot = (HOT&)mhb;
    os<<"hni="<<hot.hni()<<" hc="<<hot.homeCookie();
  }
  else if (mhb.kind() == MIPv6MHT_COT)
  {
    COT& cot = (COT&)mhb;
    os<<"cni="<<cot.coni()<<" coc="<<cot.careOfCookie();
  }
  else if (mhb.kind() == MIPv6MHT_BU)
  {
    BU& bu = (BU&)mhb;
    os<<"ack="<<bu.ack()<<" home="<<bu.homereg()<<" map="<<bu.mapreg()<<" seq="
      <<bu.sequence()<<" exp="<<bu.expires();
  }
  else if (mhb.kind() == MIPv6MHT_BA)
  {
    BA& ba = (BA&)mhb;
    os<<"status="<<ba.status()<<" seq="<<ba.sequence()<<" lifetime="<<ba.lifetime();    
  }
  else if (mhb.kind() == MIPv6MHT_BE)
  {
    BE& be = (BE&)mhb;
    os<<(be.status()?"unrecognised mob header":"unknown binding")
      <<" for hoa="<<be.hoa();
  }

  for ( size_t i = 0; i < mhb.mobilityOptions.size(); i++)
  {
    if (i == 0)
      os<<" mobopt";
    MobilityOptionBase& opt = *(mhb.mobilityOptions[i]);
    os<<" len="<<opt.byteLength();
    switch(opt.kind())
    {
    case MOPT_BRA:
	os<<" refresh interval="<<((MIPv6OptBRA&)opt).interval();
	break;
    case MOPT_ACoA:
      os<<" acoa="<<((MIPv6OptACoA&)opt).acoa();
      break;
    case MOPT_NI:
      os<<" hni="<<((MIPv6OptNI&)opt).hni()<<" coni="<<((MIPv6OptNI&)opt).coni();
      break;
    case MOPT_AUTH:
      os<<" Auth";
      break;
    default:
      os<<" unknown kind="<<opt.kind();
      break;
    }
    os<<"|";
  }
  return os;
}

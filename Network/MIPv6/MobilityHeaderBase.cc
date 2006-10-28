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
 * @todo
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "MobilityHeaderBase.h"
#include "MobilityHeaders.h"


void  MobilityHeaderBase::addMobilityOption(MobilityOptionBase* op)
{
  if (kind() == MIPv6MHT_BE || kind() == MIPv6MHT_COT || kind() == MIPv6MHT_HOT
      || kind() == MIPv6MHT_COTI || kind() == MIPv6MHT_HOTI 
      || kind() == MIPv6MHT_BRR)
    assert(false);
  mobilityOptions.push_back(op);
  //byteLength() may be bigger as it includes padding but when adding options
  //we should remove the padding and recalculate it.
  setByteLength(byteLength() + op->byteLength());
}

MobilityOptionBase*  MobilityHeaderBase::mobilityOption(int t) const
{
  for ( size_t i = 0; i < mobilityOptions.size(); i++)
    if ( mobilityOptions[i]->kind() == t )
      return mobilityOptions[i];
    
  return 0;
}

// -*- C++ -*-
//
// Copyright (C) 2001, 2005 CTIE, Monash University
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

#include "MIPv6Entry.h"

namespace MobileIPv6
{

  std::ostream& operator<< (std::ostream& os, const bc_entry& bce)
  {
    os<<" hoa="<<bce.home_addr<<" coa="<<bce.care_of_addr<<(bce.is_home_reg?" home_reg ":"")
      <<" expires="<<bce.expires<<" seq.no="<<bce.seq_no;
    return os;
  }

} //end namespace MobileIPv6

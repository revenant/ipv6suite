// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file EHTimers.h
 * @author Johnny Lai
 * @date 30 Aug 2004
 *
 * @brief Some typedefs that are shared by two loosely coupled classes
 * MobileIPv6::MIPv6MStateMobileNode and EdgeHandover::EHNDStateHost
 *
 */

#ifndef EHTIMERS_H
#define EHTIMERS_H

#if !defined BOOST_FUNCTION_HPP
#include <boost/function.hpp>
#endif

#if !defined CSIGNALMESSAGE_H
#include "cSignalMessage.h"
#endif

namespace HierarchicalMIPv6
{
  class HMIPv6MAPEntry;
};

namespace EdgeHandover
{

  typedef cSignalMessage EHCallback;
  typedef boost::function<ipv6_addr (const HierarchicalMIPv6::HMIPv6MAPEntry&,
				     unsigned int)> BoundMapChangedCB;
};

#endif //EHTIMERS_H

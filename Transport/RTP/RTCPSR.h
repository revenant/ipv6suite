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
 * @file RTCPSR.h
 * @author Johnny Lai
 * @date 10 Aug 2006
 *
 * @brief Definition of class RTCPSR
 *
 * @test see RTCPSRTest
 *
 * @todo Remove template text
 */

#ifndef RTCPSR_H
#define RTCPSR_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#include "RTCPSR_m.h"

/**
   @class RTCPSR_
   @brief RTCP Sender Reports
   @ingroup RTCP
*/

class RTCPSR : public RTCPSR_Base
{
  public:
    RTCPSR(const char *name="RTCPSR", int kind=200) : RTCPSR_Base(name,kind)
    {
      setByteLength(5*4  + byteLength());
    }
    RTCPSR(const RTCPSR& other) : RTCPSR_Base(other.name()) {operator=(other);}
    RTCPSR& operator=(const RTCPSR& other) {RTCPSR_Base::operator=(other); return *this;}
    virtual cPolymorphic *dup() const {return new RTCPSR(*this);}
    // ADD CODE HERE to redefine and implement pure virtual functions from RTCPSR_Base
};

#endif /* RTCPSR_H */


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
 * @file RTCPPacket.h
 * @author 
 * @date 29 Jul 2006
 *
 * @brief Definition of class RTCPPacket
 *
 * @test see RTCPPacketTest
 *
 * @todo Remove template text
 */

#ifndef RTCPPACKET_H
#define RTCPPACKET_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#include "RTCPPacket_m.h"

#include <vector>
#include <string>

/**
   @class RTCPRR
   @brief RTCP Receiver Report 	
   @ingroup RTCP
*/


class RTCPRR : public RTCPRR_Base
{
  public:
    RTCPRR(const char *name="RTCPRR", int kind=201) : RTCPRR_Base(name,kind) {}
    RTCPRR(const RTCPRR& other) : RTCPRR_Base(other.name()) {operator=(other);}
    RTCPRR& operator=(const RTCPRR& other) {RTCPRR_Base::operator=(other); return *this;}
    virtual cPolymorphic *dup() const {return new RTCPRR(*this);}
    // ADD CODE HERE to redefine and implement pure virtual functions from RTCPRR_Base
    virtual void setReportBlocksArraySize(unsigned int size){}
    virtual unsigned int reportBlocksArraySize() const;
    virtual RTCPReportBlock& reportBlocks(unsigned int k);
    virtual void setReportBlocks(unsigned int k, const RTCPReportBlock& reportBlocks_var);
    virtual void addBlock(const RTCPReportBlock& b);
protected:
      std::vector<RTCPReportBlock> blocks;
};

class RTCPGoodBye : public RTCPGoodBye_Base
{
  public:
    RTCPGoodBye(const char *name="RTCPBYE", int kind=203) : RTCPGoodBye_Base(name,kind) 
    {
      setByteLength(4 + byteLength());
    }
    RTCPGoodBye(const RTCPGoodBye& other) : RTCPGoodBye_Base(other.name()) {operator=(other);}
    RTCPGoodBye& operator=(const RTCPGoodBye& other) {RTCPGoodBye_Base::operator=(other); return *this;}
    virtual cPolymorphic *dup() const {return new RTCPGoodBye(*this);}
    // ADD CODE HERE to redefine and implement pure virtual functions from RTCPGoodBye_Base
};

class RTCPSDES : public RTCPSDES_Base
{
  public:
    RTCPSDES(const char *name="RTCPSDES", int kind=202) : RTCPSDES_Base(name,kind) 
    {
      setByteLength(2 + byteLength());
    }
    RTCPSDES(const RTCPSDES& other) : RTCPSDES_Base(other.name()) {operator=(other);}
    RTCPSDES& operator=(const RTCPSDES& other) {RTCPSDES_Base::operator=(other); return *this;}
    virtual cPolymorphic *dup() const {return new RTCPSDES(*this);}
    // ADD CODE HERE to redefine and implement pure virtual functions from RTCPSDES_Base
    // field getter/setter methods
    virtual const char * cname() const;
    virtual void setCname(const char * cname_var);
protected:
  std::string _cname;
};

#endif /* RTCPPACKET_H */


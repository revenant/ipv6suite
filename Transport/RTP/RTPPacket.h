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
 * @file RTPPacket.h
 * @author 
 * @date 29 Jul 2006
 *
 * @brief Definition of class RTPPacket
 *
 * @test see RTPPacketTest
 *
 * @todo Remove template text
 */

#ifndef RTPPACKET_H
#define RTPPACKET_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#include "RTPPacket_m.h"

/**
 * @class RTPPacket
 *
 * @brief 
 *
 * detailed description
 */

 class RTPPacket : public RTPPacket_Base
 {
   public:
  //@name constructors, destructors and operators
  //@{
   RTPPacket(unsigned int ssrc, unsigned int seqNo, const char *name="RTP", int kind=0) : RTPPacket_Base(name,kind) 
   {
     setByteLength(12);
     setSsrc(ssrc);
     setSeqNo(seqNo);
   }
   RTPPacket(const RTPPacket& other) : RTPPacket_Base(other.name()) {operator=(other);}
   RTPPacket& operator=(const RTPPacket& other) {RTPPacket_Base::operator=(other); return *this;}

   ///@name Overidden cObject functions and pure virtual functions from RTPPacket_Base
   //@{

   virtual cPolymorphic *dup() const {return new RTPPacket(*this);}

   virtual void setPayloadLength(unsigned int payloadLength_var)
   {
     // adjust message length
     setByteLength(byteLength()-payloadLength() + payloadLength_var);

     // set the new length
     RTPPacket_Base::setPayloadLength(payloadLength_var);
   }

   //*}

 };




/**
 * @class RTCPPacket
 *
 * @brief 
 *
 * detailed description
 */

class RTCPPacket: public RTCPPacket_Base
{
 public:

  //@name constructors, destructors and operators
  //@{
    RTCPPacket(const char *name="RTCP", int kind=0) : RTCPPacket_Base(name,kind) 
    {
      setByteLength(8);
    }
    RTCPPacket(const RTCPPacket& other) : RTCPPacket_Base(other.name()) {operator=(other);}
    RTCPPacket& operator=(const RTCPPacket& other) {RTCPPacket_Base::operator=(other); return *this;}
  //@}

    virtual cPolymorphic *dup() const {return new RTCPPacket(*this);}

 protected:

 private:

};

#endif /* RTPPACKET_H */


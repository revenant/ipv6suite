// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/HdrExtFragProc.h,v 1.2 2005/02/10 05:27:42 andras Exp $
//
// Copyright (C) 2001 CTIE, Monash University
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

/**
   @file HdrExtFragProc.h
   @brief interface for the HdrExtFragProc class

   Processing of ipv6_frag_hdr, fragmentation/reassembly
   @author Johnny Lai
*/


#if !defined(AFX_HDREXTFRAGPROC_H__F1261A16_3253_429D_8AB8_F3866EDC4741__INCLUDED_)
#define AFX_HDREXTFRAGPROC_H__F1261A16_3253_429D_8AB8_F3866EDC4741__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <omnetpp.h>
#include <cassert>

#include "IPv6Headers.h"
#include "HdrExtProc.h"

class IPv6Datagram;

/**
 * @class HdrExtFragProc
 * @brief Processing of Fragmentation Extension header
 *
 * Unimplemented
 */

class HdrExtFragProc: public HdrExtProc
{
public:

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

  explicit HdrExtFragProc();

  explicit HdrExtFragProc(const HdrExtFragProc& src);

  virtual ~HdrExtFragProc();


  HdrExtFragProc& operator=(const HdrExtFragProc& rhs);

  virtual HdrExtFragProc* dup() const
    {
      return new HdrExtFragProc(*this);
    }

  virtual bool processHeader(cSimpleModule* mod, IPv6Datagram* pdu);

  /**
     @Overridden cObject functions
  */
  //@{
//   virtual cObject* dup() const { return new HdrExtFragProc(*this); }
//   virtual const char *className() const { return "HdrExtFragProc"; }
//   virtual std::string info();
//   virtual void writeContents(ostream& os);
  //@}


  /**
     Assemble packets. The assembled packet is stored in this instance.
     @arg nfrags count of fragments in array frags
     @arg frags array of fragments with count given in nfrags
     @return true if packet was assembled successfully, false otherwise
     @note This destroys whatever packet was in here previously.
  */
  bool assemblePacket(const int& nfrags, IPv6Datagram* frags,
                      IPv6Datagram*& defragPdu);

  /**
     Fragment this packet.
     @arg path_mtu specifies the MTU to fragment for
     @arg nfrags count of fragments returned
     @return the fragment array sorted by frag_off with count in nfrags
  */
  IPv6Datagram** fragmentPacket(IPv6Datagram* pdu, const size_t& path_mtu,
                                size_t& nfrags) const;

  unsigned int fragmentId() const
    {
      return frag_hdr.frag_id;
    }

  int fragmentOffset() const
    {
      assert(frag_hdr.frag_off > 0);
      return frag_hdr.frag_off>>3; //Only the top 13 bits
    }

  /**
     The next datagram disassembled will use this frag_id
     Note: You can not change the fragment id of a packet
     after it has been fragmented
  */
  void setFragmentId(unsigned int frag_id)
    {
      this->frag_id = frag_id;
    }

  /*
	bool dontFragment() { return dont_fragment; }
	void setDontFragment(bool dontFragment)
    { dont_fragment = dontFragment; }
  */

  //Don't need this as this class will take care of all frag details
//	void setFragmentOffset(int offset);

  /**
     Test if fragments are outstanding for this packet id.
     @returns true if fragments are outstanding
     false if this is the last fragment or unfragmented packet
  */
  bool moreFragments() const;

  /*
	bool lastFragment()
	{
    return moreFragments()?false:true;
	}
  */

protected:

  /**
     Common frag_counter for whole simulation.  As long as there are not
     too many nodes this should not be a problem.
  */
  static unsigned int frag_counter;

  ipv6_ext_frag_hdr& frag_hdr;

  /**
     The fragment ID to used when this packet is fragmented.
  */
  unsigned int frag_id;
};

#endif // !defined(AFX_HDREXTFRAGPROC_H__F1261A16_3253_429D_8AB8_F3866EDC4741__INCLUDED_)

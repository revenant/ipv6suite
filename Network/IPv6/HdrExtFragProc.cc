// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/HdrExtFragProc.cc,v 1.3 2005/02/10 05:59:32 andras Exp $
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
// HdrExtFragProc.cc: implementation of the HdrExtFragProc class.
//
//////////////////////////////////////////////////////////////////////

/**
   @file HdrExtFragProc.cc
   @brief interface for the HdrExtFragProc class

   Processing of ipv6_frag_hdr, fragmentation/reassembly
   @author Johnny Lai
*/

#include "HdrExtFragProc.h"
#include "IPv6Datagram.h"
#include "HdrExtRteProc.h"

#include <boost/cast.hpp>

using boost::polymorphic_downcast;

unsigned int HdrExtFragProc::frag_counter = 0;


HdrExtFragProc::HdrExtFragProc():HdrExtProc(EXTHDR_FRAGMENT),
                                 frag_hdr(*polymorphic_downcast<ipv6_ext_frag_hdr*>(ext_hdr)),
                                 frag_id(HdrExtFragProc::frag_counter)
{}

HdrExtFragProc::HdrExtFragProc(const HdrExtFragProc& src)
  :HdrExtProc(EXTHDR_FRAGMENT), frag_hdr(*polymorphic_downcast<ipv6_ext_frag_hdr*>(ext_hdr)),
   frag_id(HdrExtFragProc::frag_counter)
{
  //setName(src.name());
  operator=(src);
}

HdrExtFragProc::~HdrExtFragProc()
{}

/**
   @todo not completed yet.
 */
HdrExtFragProc&  HdrExtFragProc::operator=(const HdrExtFragProc& rhs)
{
  if (this != &rhs)
  {
    //assign(rhs);
    frag_id = rhs.frag_id;
  }

  return *this;
}

bool HdrExtFragProc::processHeader(cSimpleModule* mod, IPv6Datagram* pdu)
{
  cerr<<"Frag header encountered.  Not implemented fully"<<endl;
  return false;
}

// std::string  HdrExtFragProc::info()
// {
//   int bufPos = strlen(buf);
//   sprintf(&buf[bufPos], " FragHdr id=%d, off=%d, len=%d, more=%d",
//           fragmentId(), fragmentOffset(), length(), moreFragments()?1:0);
// }

bool HdrExtFragProc::moreFragments() const
{
  return frag_hdr.frag_off & IPv6_FRAG_MASK?true:false;
  return false;
}

/**
    Assemble packets. The assembled packet is stored in this instance.
    @nfrags count of fragments in array frags
    @frags array of fragments with count given in nfrags
    @return true if packet was assembled successfully, false otherwise
*/
bool HdrExtFragProc::assemblePacket(const int& nfrags, IPv6Datagram* frags,
                                    IPv6Datagram*& defragPdu)
{
  return false;
}

/**
    Fragment this packet.
    @path_mtu specifies the MTU to fragment for
    @nfrags count of fragments returned
    @returns the fragment array sorted by frag_off with count in nfrags
*/
//TODO This function is not correct
IPv6Datagram** HdrExtFragProc::fragmentPacket(IPv6Datagram* pdu,
                                              const size_t& path_mtu,
                                              size_t& nfrags) const
{
  //Use the current frag_id
  assert(frag_id == 0); //Original packet was not a fragment itself?
  //Search for Unfragmentable part i.e. iff Routing Header else iff Hop-by-Hop Options.
  size_t unfrag_len = IPv6_HEADER_LENGTH;
  int pos = -1;

  unfrag_len += cumul_len(*pdu) + length();

#ifdef DEBUG
  //Compare with direct computation of the ext_hdrs

#endif

  //Then calculate the fragmentable length
  int frag_len = pdu->totalLength() - unfrag_len;

  unsigned int frag_pdu_len = 0;
  int offset = 0;
  int remainder = 0;

  for (int i = 1;; i++)
    {
      nfrags = 8*i;
      offset = frag_len/nfrags;
      frag_pdu_len = unfrag_len + length() + offset;
      if (frag_pdu_len <= path_mtu)
        {
          remainder = frag_len%nfrags;
        }
    }

  //Create a new pdu (the first one which will be duplicated)
  IPv6Datagram* first_frag = new IPv6Datagram();
  first_frag->header = pdu->header;

  //Iterate through all wrappers upto pos and get them to dup the raw
  //ext_hdr struct and add these to first_frag's ext_hdrs.

  //  Change the unfraggable part's next header value to point to frag_hdr (44)

  HdrExtProc* proc = 0;
  //iterate through headers till we reach one before this header
  if (proc)
    {

      proc->setNextHeader(NEXTHDR_FRAGMENT);
    }
  else
    {
      pdu->header.next_header = NEXTHDR_FRAGMENT;
      pos = 0;

    }

  //Append the fragment header taking note of changing the offset

  //Copy the rest of the ext_hdrs directly from pdu also making note
  //of the original ext_hdr's next_header value and setting it to
  //frag_hdr's next_header to that.

  //Replicate this arrangement to the other fragments along with
  //copying the actual upper layer packet chunk by chunk. (don't know
  //if cPacket has this func.
  IPv6Datagram** frags = new IPv6Datagram* [nfrags];
  frags[0] = first_frag;
  for (size_t i = 1; i < nfrags; i++)
    frags[i] = first_frag->dup();


  // Append a frag header with the chosen frag_id and the correct
  // offset from beginning of fraggable part and then append the
  // fragment itself finally.




  //After frag increment global counter
  frag_counter++;

  return frags;
}

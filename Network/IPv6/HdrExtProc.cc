// -*- C++ -*-
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
   @file HdrExtProc.cc
   @brief Implementation of base class for IPv6 extension headers

   Have a general ExtHdr class with virtual length functions that deal with
   traversing the headers and obtaining the lengths from them.  All other exthdr
   classes extend from that.
*/


#include <boost/cast.hpp>
#include <omnetpp.h>

#include "IPv6Headers.h"
#include "IPv6Datagram.h"
#include "HdrExtProc.h"
#include "HdrExtRteProc.h"
#include "HdrExtFragProc.h"



HdrExtProc::~HdrExtProc()
{
  delete ext_hdr;
}

int HdrExtProc::cumul_len(const IPv6Datagram& pdu) const
{
  int len = 0;
  HdrExtProc* proc = pdu.getNextHeader(0); //Get the first header

  //There has to be an extension header after all we are an extension header
  assert(proc != 0);

  for(;;)
  {
    if (proc == 0)
    {

      cerr << "Error in calculating cumulative length. Wrapper not found"<<endl;
      return len;
    }

    len += proc->length();
    //Find cumulative length until it reaches us.
    if (proc == this)
      return len + length();
    proc = pdu.getNextHeader(proc);
  }


}


//In units of octets.
size_t HdrExtProc::length() const
{
  size_t len = 0;

  switch (_type)
  {
    //Detect Pad1/PadN and figure out length
    case EXTHDR_HOP:
      cerr << "Hop extension header not implemented "<<endl;
      break;
    case EXTHDR_ROUTING:
      assert(false); //overridden
      break;
    case EXTHDR_FRAGMENT:
      len = IPv6_FRAG_HDR_LEN;
      break;
      //Not supported i.e. is ignored completely. (log warning msg)
    case EXTHDR_ESP: case EXTHDR_AUTH:
      cerr << "Don't know length for EXTHDR_ESP/EXTHDR_AUTH"<<endl;
      break;
    case NEXTHDR_DEST:
      //Currently only home addr option so this should be fine.
      len = IPv6_RT_HDR_LEN + 16;
      break;
    default:
      assert(false);
      cerr << "Unrecoverable error: Unknown ext hdr type: "<<_type<<endl;

      break;
  }
  return len;
}

ipv6_ext_hdr* HdrExtProc::construct(const IPv6ExtHeader& hdr_type,
                                    int opt_type) const
{
  ipv6_ext_hdr* ret = 0;

  switch (hdr_type)
  {
    case EXTHDR_DEST:
      ret = new ipv6_ext_opts_hdr;
      break;
    case EXTHDR_ROUTING:
      ret = new ipv6_ext_rt_hdr;
      break;
    case EXTHDR_FRAGMENT:
      ret = new ipv6_ext_frag_hdr;
      break;

    case EXTHDR_ESP: case EXTHDR_AUTH:
      cerr << "Don't know about authentication header types "<<_type<<endl;
      break;
    default:
      assert(false);
      cerr << "Unrecoverable error: Unknown ext hdr type: "<<_type<<endl;
      break;
  }
  assert(ret);
  return ret;
}

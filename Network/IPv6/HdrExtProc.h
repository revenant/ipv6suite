// -*- C++ -*-
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
   @file HdrExtProc.h: interface for the HdrExtProc class.
   @brief Base class for IPv6 extension headers

   Have a general ExtHdr class with virtual length functions that deal with
   traversing the headers and obtaining the lengths from them.  All other exthdr
   classes extend from that.
   @author Johnny Lai
*/


#if !defined(HDREXTPROC_H)
#define HDREXTPROC_H

#include "IPProtocolId_m.h"

#ifndef IPV6HEADERS_H
#include "IPv6Headers.h"
#endif //IPV6HEADERS_H

class IPv6Datagram;

/**
   @class HdrExtProc

   @brief Abstract base class for IPv6 Extension headers.

   Defines an interface to process extension headers.
 */

class HdrExtProc
{
public:
  //  friend class IPv6Datagram;

  virtual ~HdrExtProc() = 0;

  virtual const char* className() const { return "HdrExtProc"; }

  bool operator==(const HdrExtProc& rhs)
    {
      return _type == rhs._type;
    }

  virtual std::ostream& operator<< (std::ostream& os)
    {
      return os<<"ExtHdr "<<_type<<" nh="<<nextHeader()<<" ";
    }

  /**
   * length of this extension header in octects.
   *
   */

  virtual size_t length() const;

  /**
   * accumulate length of pdu headers up to and including this header
   * @param pdu has to contain this extension header (loop terminate condition)
   */
  virtual int cumul_len(const IPv6Datagram& pdu) const;

  /**
     Process extension header according to IPv6 Spec
     Relative ordering of header processing is determined by app and not by this
     function (this was the original mistaken intent)
  */
  virtual bool processHeader(cSimpleModule* mod, IPv6Datagram* pdu) = 0;

  virtual HdrExtProc* dup() const = 0;

  //@name Extension Header attributes
  //@{
  IPv6NextHeader nextHeader() const
    {
      //Todo macro to test authenticity of next_header
      return static_cast<IPv6NextHeader> ( ext_hdr->next_header);

    }
  void setNextHeader(const IPv6NextHeader& hdr_type)
    {
      ext_hdr->next_header = hdr_type;
    }

  void setNextHeader(const IPProtocolId& prot_type)
    {
      ext_hdr->next_header = (int) prot_type;
    }

  void setNextHeader(const IPv6ExtHeader& ext_hdr_type)
    {
      ext_hdr->next_header = (int) ext_hdr_type;
    }

  IPv6ExtHeader type() const
    {
      return _type;
    }

  ///Length of Extension header in units of 8 octets, excluding the first 8
  ///octets.
  unsigned int hdrExtLen() const
    {
      return ext_hdr->hdr_ext_len;
    }

  void setHdrExtLen(unsigned char hdr_len) const
    {
      ext_hdr->hdr_ext_len = hdr_len;
    }

  //@}
protected:


  //@name Constructors, Destructors and operators
  //@{
  explicit HdrExtProc(const IPv6ExtHeader& hdr_type, int opt_type = 0)
    :ext_hdr(construct(hdr_type, opt_type)), _type(hdr_type)
    {}

  /**
     The copy constructor for ext hdrs needs to take care of copying the values
  */
//   explicit HdrExtProc(const HdrExtProc& src)
//     :ext_hdr(0), _type(src._type)
//     {
//       operator=(src);
//     }

  HdrExtProc& operator=(const HdrExtProc& rhs)
    {
      if (this == &rhs)
        return *this;

      if (_type != rhs._type)
      {
        delete ext_hdr;
        _type = rhs._type;
        //Get the subclass to allocate the correct ext header
        ext_hdr = rhs.construct(_type, 0);
      }

      return *this;
    }
  //@}

  virtual ipv6_ext_hdr* construct(const IPv6ExtHeader& hdr_type,
                                  int opt_type) const;
protected:
  /**
     ext_hdr is owned by this object
  */
  ipv6_ext_hdr* ext_hdr;

  /**
     Identifies the current type of ext hdr.  Although derived classes
     will know what they are we want a quick way to calculate length
     without RTTI type inquiry of this
  */
  IPv6ExtHeader _type;
private:
  //Unimplemented. subclasses must implement
  HdrExtProc(const HdrExtProc& src);
};

#endif // !defined(HDREXTPROC_H)

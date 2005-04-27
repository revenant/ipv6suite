// -*- C++ -*-
//
// Eric Wu
// Copyright (C) 2001 Monash University, Melbourne, Australia
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

/*
    @file EtherFrame6.h
    @brief Ethernet frame format definition
    @author Eric Wu
*/


#ifndef __ETHER_FRAME_H
#define __EHTER_FRAME_H

#include <omnetpp.h>
#include "MACAddress6.h"

/*  -------------------------------------------------
        Constants
    -------------------------------------------------   */

extern const int PREAMBLE;
extern const int DEST_ADDR_LEN;
extern const int SRC_ADDR_LEN;
extern const int TYPE_LEN;
extern const int CRC;
extern const int POSTAMBLE;

/*  -------------------------------------------------
        Main class: EtherFrame6
    -------------------------------------------------
    field simulated:
        protocol
    constant fields not simulated:
        flag (0x7e), control (0x03), RC (biterror)
*/

class EtherFrame6: public cMessage
{
public:
  // constructors
  EtherFrame6(const char* name = NULL);
  EtherFrame6(const EtherFrame6 &p);

  // assignment operator
  virtual EtherFrame6& operator=(const EtherFrame6& p);
  virtual EtherFrame6 *dup() const { return new EtherFrame6(*this); }

  // info functions
  virtual const char *className() const { return "EtherFrame6"; }
  virtual std::string info();
  virtual void writeContents(std::ostream& os);
  const char  *dumpContents(void);

  // set functions
  void setProtocol(int prot);
  void setSrcAddress(const MACAddress6& src);
  void setDestAddress( const  MACAddress6& dest);
  void setDataLength(int dataLen);

  // get functions
  int protocol() const;
  const MACAddress6& srcAddress(void) const;
  const MACAddress6& destAddress(void) const;
  const char *destAddrString(void) const;
  const char *srcAddrString(void) const;

  #if defined __CN_PAYLOAD_H
  //send packets in network order;
  struct network_payload *networkOrder(void) const ;
  #endif //defined __CN_PAYLOAD_H

private:
  int _protocol;
  MACAddress6 _srcAddr;
  MACAddress6 _destAddr;
  int pack_mac_addr(const char *straddr, unsigned char *pack) const;

  // header length functions
  int headerByteLength() const;
};

#endif



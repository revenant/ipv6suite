// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/Ethernet6/Attic/EtherFrame.h,v 1.4 2005/02/11 10:46:37 andras Exp $
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
    @file EtherFrame.h
    @brief Ethernet frame format definition
    @author Eric Wu
*/


#ifndef __ETHER_FRAME_H
#define __EHTER_FRAME_H

#include <omnetpp.h>
#include <MACAddress.h>

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
        Main class: EtherFrame
    -------------------------------------------------
    field simulated:
        protocol
    constant fields not simulated:
        flag (0x7e), control (0x03), RC (biterror)
*/

class EtherFrame: public cMessage
{
public:
  // constructors
  EtherFrame(const char* name = NULL);
  EtherFrame(const EtherFrame &p);

  // assignment operator
  virtual EtherFrame& operator=(const EtherFrame& p);
  virtual EtherFrame *dup() const { return new EtherFrame(*this); }

  // info functions
  virtual const char *className() const { return "EtherFrame"; }
  virtual std::string info();
  virtual void writeContents(std::ostream& os);
  const char  *dumpContents(void);

  // set functions
  void setSrcAddress(const MACAddress& src);
  void setDestAddress( const  MACAddress& dest);
  void setDataLength(int dataLen);

  // get functions
  const MACAddress& srcAddress(void) const;
  const MACAddress& destAddress(void) const;
  const char *destAddrString(void) const;
  const char *srcAddrString(void) const;

  // return the total length of the packet including the header
  // the packet is in octets
//  const int packetLength(void);

/* XXX seems like this is not strictly necessary --AV
  // encapsulation/decapsulation of the IP datagram
* virtual void encapsulate(cPacket *);
  cPacket* decapsulate();
*/

  #if defined __CN_PAYLOAD_H
  //send packets in network order;
  struct network_payload *networkOrder(void) const ;
  #endif //defined __CN_PAYLOAD_H

private:
  MACAddress _srcAddr;
  MACAddress _destAddr;
  int pack_mac_addr(const char *straddr, unsigned char *pack) const;
//  int _dataLen;

  // header length functions
  int headerByteLength() const;
};

#endif



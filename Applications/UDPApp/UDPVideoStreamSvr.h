// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
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
 * @file UDPVideoStreamSvr.h
 * @author Johnny Lai
 * @date 01 Jun 2004
 *
 * @brief Definitition of class UDPVideoStreamSvr
 *
 * Video stream server capable of VBR streaming
 *
 */

#ifndef UDPVIDEOSTREAMSVR_H
#define UDPVIDEOSTREAMSVR_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR

#ifndef MAP
#define MAP
#include <map>
#endif //MAP

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

#ifndef UDPAPPLICATION_H
#include "UDPApplication.h"
#endif //UDPAPPLICATION_H

#ifndef EXPIRYENTRYLIST_H
#include "ExpiryEntryList.h"
#endif //EXPIRYENTRYLIST_H

/**
 * @class UDPVideoStreamSvr
 *
 * @brief Stream VBR video streams to clients
 *
 * Implements the UDPApplication interface and cooperates with UDPVideoStream.
 * UDPVideoStream requests a stream and UDPVideoStreamSvr starts streaming to
 * them. Capable of handling streaming to multiple clients.
 * 
 */

class UDPVideoStreamSvr: public UDPApplication
{
public:
  
  Module_Class_Members(UDPVideoStreamSvr, UDPApplication, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  virtual void initialize();

  virtual void finish();

  virtual void handleMessage(cMessage* msg);
  //@}

protected:

private:
  double minWaitInt;
  double maxWaitInt;
  unsigned int minPacketLen;
  unsigned int maxPacketLen;
  unsigned int videoSize;
  unsigned int speed;

  /**
     @struct ClientStreamData

     @brief Stores information on the client that has requested video streaming

     Compiler generated copy ctor is fine as long as no pointers are used
   */
  struct ClientStreamData
  {
    ///Satisfy default constructible req. by providing defaults
    ClientStreamData(std::string host = std::string(""), unsigned int port = 0,
                     unsigned int bytesLeft = 0, double txTime = 0)
      :host(host), port(port), bytesLeft(bytesLeft), txTime(txTime),
       packetCount(0)
      {}

    ///peer host ID (IPv6 address) 
    std::string host;
    ///peer port
    unsigned int port;
    unsigned int bytesLeft;
    ///Next scheduled stream transmission time
    double txTime;
    ///number of pkt tx (not constant as each packet size is random)
    unsigned int packetCount;
    double expiryTime() const { return txTime; }
    bool operator==(const ClientStreamData& rhs) const
      {
        return this->port == rhs.port && this->host == rhs.host;
      }    
  };

  //  std::map<std::string, ClientStreamData> cs;

  ///List of clients that have requested streams from me
  typedef ExpiryEntryList<
    ClientStreamData, Loki::cTimerMessageCB<void, TYPELIST_1(ClientStreamData)>
  > CSEL;

  CSEL* cs;

  ///Callback invoked to send stream to client c when client is scheduled to
  ///receive a stream as determined by cs.
  void transmitClientStream(ClientStreamData& c);

};


#endif /* UDPVIDEOSTREAMSVR_H */

// -*- C++ -*-
//
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
 * @file UDPVideoStream.h
 * @author Johnny Lai
 * @date 25 May 2004
 *
 * @brief Definitition of class UDPVideoStream
 *
 * @test see UDPVideoStreamTest
 *
 * @todo Remove template text
 */

#ifndef UDPVIDEOSTREAM_H
#define UDPVIDEOSTREAM_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef UDPAPPLICATION_H
#include "UDPApplication.h"
#endif //UDPAPPLICATION_H

/**
 * @class UDPVideoStream
 *
 * @brief "Realtime" VideoStream client application
 *
 * Basic video stream application. Clients connect to server and get a stream of
 * video back.
 */

class UDPVideoStream: public UDPApplication
{
public:
  friend class UDPVideoStreamTest;

  Module_Class_Members(UDPVideoStream, UDPApplication, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  virtual void initialize();

  virtual void finish();

  virtual void handleMessage(cMessage* msg);
  //@}

protected:

  void requestStream();
  void receiveStream(cMessage* msg);

private:
  cOutVector eed;

  std::string address;

  unsigned int svrPort;
  double startTime;
};


#endif /* UDPVIDEOSTREAM_H */


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
 * @file UDPApplication.h
 * @author Johnny Lai
 * @date 25 May 2004
 *
 * @brief Definitition of class UDPApplication
 *
 *
 */

#ifndef UDPAPPLICATION_H
#define UDPAPPLICATION_H

#include <string>
#include <omnetpp.h>



/**
 * @class UDPApplication
 *
 * @brief Base class for all UDP simple modules
 *
 * For usage look at UDPVideoStream application. This allows all UDP apps to
 * share common port registering code
 */
class UDPApplication: public cSimpleModule
{
public:
  friend class UDPApplicationTest;

  Module_Class_Members(UDPApplication, cSimpleModule, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  virtual void initialize();

  virtual void finish();

  //throw if no response from UDP layer on registration and we want to send a
  //packet. Base class will always call us first
  virtual void handleMessage(cMessage* msg);
  //@}

protected:
  //Will bind the port port if server is true otherwise binds anonymously. If
  //binding anonymously the port will store the bound port.
  void bindPort();
  bool isReady() const { return bound; }

  std::string address;
  unsigned int port;
  bool server;

  //Should be same for all modules since it is not an array
  static int gateId;
private:
  //reject all packets if bound is false
  bool bound;
};


#endif /* UDPAPPLICATION_H */


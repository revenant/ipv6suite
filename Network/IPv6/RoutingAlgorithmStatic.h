// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/RoutingAlgorithmStatic.h,v 1.1 2005/02/09 06:15:58 andras Exp $
// Copyright (C) 2003 Johnny Lai
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
 * @file RoutingAlgorithmStatic.h
 * @author Johnny Lai
 * @date 13 Jul 2003
 *
 * @brief Definition of class RoutingAlgorithmStatic
 *
 * @test See RoutingAlgorithmStaticTest
 *
 * @todo
 */

#ifndef ROUTINGALGORITHMSTATIC_H
#define ROUTINGALGORITHMSTATIC_H 1

#include <omnetpp.h>

class WorldProcessor;
class RoutingTable6;

namespace xml
{
  class node;
}


#ifndef USE_XMLWRAPP
#error "Cannot use static routing algorithm with Xerces-c!"
#endif //USE_XMLWRAPP

/**
 * @class RoutingAlgorithmStatic
 *
 * @brief Concrete RoutingAlgorithmType for static routing using XML
 * configuration file inside module vector Routing6
 *
 * Moved initialisation of RoutingTable6 from XMLConfiguration to this class.
 */

class RoutingAlgorithmStatic: public cSimpleModule
{
 public:

  Module_Class_Members(RoutingAlgorithmStatic, cSimpleModule, 0);

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;
  
  virtual void initialize(int stageNo);

  virtual void handleMessage(cMessage* msg);
  //@}

 protected:
  void parseNetworkEntity(const char* nodeName);
  void parseInterface(const xml::node& iface);
  void parseNetworkNodeProperties(const xml::node& netNode);

 private:

  RoutingAlgorithmStatic(const RoutingAlgorithmStatic& src);

  RoutingAlgorithmStatic& operator=(RoutingAlgorithmStatic& src);

  WorldProcessor* wp;
  RoutingTable6* rt;
};


#endif /* ROUTINGALGORITHMSTATIC_H */


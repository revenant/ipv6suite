// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/PHY/Wireless/Attic/PHYSimple.cc,v 1.2 2005/02/10 06:26:21 andras Exp $
//
// Copyright (C) 2001, 2003 CTIE, Monash University
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
    @file PHYSimple.cc
    @brief Implementation file for PHYSimple

    Responsibilities:
        - simple physical layer which passes data between layers

    @author Eric Wu
*/

#include <cassert>
#include <iostream>

#include "PHYSimple.h"
#include "opp_utils.h"

Define_Module_Like( PHYSimple, PhysicalLayer );

void PHYSimple::initialize(void)
{}

void PHYSimple::handleMessage(cMessage* msg)
{
  bool found = false;
  int numOfPorts = gate("in")->size();

  for (int i = 0; i < numOfPorts; i++)
  {
    if (msg->arrivedOn(findGate("in", i)))
    {
      send((cMessage*)msg->dup(), "linkOut", i);
      found = true;
      std::string debugProblem(fullPath());
      if (debugProblem.find("router") != std::string::npos)
      {
//        std::cout<<simTime()<<" Physical At router sending to upper layer gate i="<<i<<"\n";
      }
      break;
    }
    else if (msg->arrivedOn(findGate("linkIn", i)))
    {
      std::string debugProblem(fullPath());
      if (debugProblem.find("router") != std::string::npos)
      {
//        std::cout<<simTime()<<" Physical At router slam it onto wire gate i="<<i<<"\n";
      }
          send((cMessage*)msg->dup(), "out", i);
          found = true;
          break;
    }
  }
//   if (!found)
//     std::cerr<<" Message arrived at Physical layer and unable to send it elsewhere\n";
  delete msg;
}

void PHYSimple::finish()
{}

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
    @file IPv6PreRouting.h
    @brief Header file for PreRouting Module
    @author Johnny Lai
    @date 27/08/01
*/

#ifndef PREROUTING6CORE_H
#define PREROUTING6CORE_H

#include <omnetpp.h>
#include "QueueBase.h"

class IPv6Forward;

/**
 * IPv6 implementation of PreRouting.
 */
class IPv6PreRouting : public QueueBase
{
public:
  Module_Class_Members(IPv6PreRouting, QueueBase, 0);

  virtual void initialize();
  virtual void finish();
  virtual void endService(cMessage *msg);
private:
  unsigned int ctrIP6InReceive;
  ::IPv6Forward* forwardMod;
};

#endif //PREROUTING6CORE_H

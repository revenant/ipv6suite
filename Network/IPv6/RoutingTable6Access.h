// -*- C++ -*-
//
// Copyright (C) 2000, 2004 Institut fuer Telematik, Universitaet Karlsruhe
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
   @file RoutingTable6Access.h
   @brief Provide access to RoutingTable6
   @author Johnny Lai
   @note Based on RoutingTableAccess by Jochen Reber
 */

#ifndef ROUTING_TABLE6_ACCESS_H
#define ROUTING_TABLE6_ACCESS_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

class RoutingTable6;

class RoutingTable6Access: public cSimpleModule
{
private:

protected:

    RoutingTable6 *rt;

public:
    Module_Class_Members(RoutingTable6Access, cSimpleModule, 0);

    virtual void initialize();
};

#endif


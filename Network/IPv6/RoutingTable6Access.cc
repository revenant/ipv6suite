// -*- C++ -*-
//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
   @file RoutingTable6Access.cc
   -Purpose Provide access to RoutingTable6
   -Author Johnny Lai
   -miscellaneous
   based on RoutingTableAccess by Jochen Reber
 */

#include "sys.h"
#include "debug.h"

#include "RoutingTable6Access.h"
#include "RoutingTable6.h"

Define_Module( RoutingTable6Access );

void RoutingTable6Access::initialize()
{
        cObject *foundmod;
        cModule *curmod = this;



        // find Routing Table
        rt = NULL;
        for (curmod = parentModule(); curmod != NULL;
                        curmod = curmod->parentModule())
        {
                if ((foundmod = curmod->findObject("routingTable6", false)) != NULL)
                {
                        rt = (RoutingTable6 *)foundmod;
                        break;
                }
        }

}


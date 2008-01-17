//
// Copyright (C) 2001 Monash University, Australia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


//
// Handles most MIPv6 (Mobile IPv6) related operations
//
// Contains the Home Agents List (HAL), Binding Cache (BC), Binding Updates List
// (BUL) and maintain the lifetime of those entries according to Draft 16.
//
// Provides an interface for MIPv6NDStateHomeAgent/Host to update HAL, BC.
//
// Provides interface to update BUL for MIPv6StateMobileNode.
//
simple IPv6Mobility
       parameters:
         homeAgent: string;
       gates:
       in: localDeliverIn;
       in: NDiscDIn;
       out: routingOut;
       out: NDiscDOut;
endsimple

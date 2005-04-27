// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/IPv6InputQueue.ned,v 1.3 2005/04/27 08:19:34 andras Exp $
//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
// Demultiplexes incoming packets from all network interfaces to one queue
// for IP.
//
// author: Jochen Reber
//
simple IPv6InputQueue
    parameters:
        procDelay: numeric const;
    gates:
        in: fromNW[], processorManagerIn;
        out: toIP;
endsimple


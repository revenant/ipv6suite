//
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



//
// Prototype module for Link Layer protocols.
//# FIXME or at least it appears to be... (Andras)
//
// Responsibilities:
// - stub version for test only,
// - can be replaced later, possibly by compound module
// - can be used as a "like"-module
//
//
// Prototype module for Link Layer protocols.
//
// Responsibilities:
// - can be used as a "like"-module
//
module NetworkInterface6
    parameters:
        procdelay : numeric const,
        nwiXmlConfig: xml;
    gates:
        in: physicalIn, ipOutputQueueIn, extSignalIn[];
        out: physicalOut, ipOutputQueueOut, ipInputQueueOut, extSignalOut[];
endmodule

//
// Copyright (C) 2001 Monash University, Australia
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


//
// Manages terrain positions of all entities, and handles mobility in the
// simulation. There has to be <b>exactly one</b> WorldProcessor in every
// network model, and it has to be the <b>first</b> submodule in the top-level
// module.
//
// WorldProcessor acts as a database during simulation. At initialization,
// it parses the XML configuration file (see <a href="config.html">Configuring
// IPv6 models</a>) into a data structure which will be accessed by modules
// in the simulated network nodes.
//
// This information includes:
// - routing tables
// - node configurations, protocol parameters
// - movement information (path, etc.) for mobile nodes
//
// Other nodes read this information by directly calling methods of
// the class implementing WorldProcessor.
//
// The single WorldProcessor instance in the network model must be
// named <tt>worldProcessor</tt> and it must be on network level
// (submodule of the toplevel module) for the other modules to find it.
//
// Routing tables are read by RoutingTable simple modules.
//
// <b>Wireless nodes</b>
//
// The WorldProcessor also plays a part in the communication of wireless nodes.
// Wireless nodes maintain their own positions and manage mobility (perform
// movements by periodically updating their positions) themselves, but
// they register themselves with the WorldProcessor. The WorldProcessor thus
// has a list of all wireless entities in the network, and this information
// is used to determine recipients of a wireless frame when it is sent.
//
// Registration of wireless/mobile entities is done by directly calling methods
// of the WorldProcessor. Wireless nodes represent themselves in the
// WorldProcessor as instances of the <tt>Entity</tt> C++ class.
// The <tt>Entity</tt> objects themselves are kept by the wireless nodes
// (currently in the module representing the wireless physical layer),
// WorldProcessor only contains pointers to them. FIXME or not????
//
simple WorldProcessor
    parameters:
        IPv6routingFile : string, // name of XML file containing all configuration
                                  // (not only routing tables)
        max_longitude : numeric const, // only used to define the size of the network
                                       // module graphics in Tkenv (pixels)
        max_latitude : numeric const,  // see max_longitude
endsimple

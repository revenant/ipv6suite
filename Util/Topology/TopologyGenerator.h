// -*- C++ -*-
//
// Copyright (C) 2002 CTIE, Monash University
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
 * @file TopologyGenerator.h
 * @author Johnny Lai
 * @date 08 Nov 2002
 *
 * @brief Executable that generates topologies and corresponding XML
 * configuration file
 *
 * Perhaps use Brite to generate the initial topology and then we add HA/MAPs
 * and hosts/MN along with addresses and routes set up or use OSPF to distribute
 * route
 *
 * Initially generate an simplified topology with hierarchical routing i.e. if
 * subnet does not match us then forward up to default route otherwise send it
 * down to a specific subnet router.
 */

#ifndef TOPOLOGYGENERATOR_H
#define TOPOLOGYGENERATOR_H 1

#include <iosfwd>
#include <cstdlib> //size_t

class XMLWriterHandler;

/**
 * @class TopologyGenerator
 *
 * @brief Generate a sample XML configuration and binary topology based on the
 * number of router nodes.
 * @ingroup Prototype
 */

class TopologyGenerator
{
public:

  //@name constructors, destructors and operators
  //@{
  TopologyGenerator(std::size_t nodeCount);

  ~TopologyGenerator();

  //@}

  ///Generate a simple binary tree hierachy of routers
  template <class Graph>
  void generateGraph(Graph&, std::ostream& os);

  template <class GraphvizType, class Graph>
  void generateGraphFromGraphviz(Graph&, const char* dotfileName);

  template <class Graph>
  void outputXML(Graph& g, XMLWriterHandler*);

protected:

private:
  std::size_t nodeCount;
};


#endif /* TOPOLOGYGENERATOR_H */

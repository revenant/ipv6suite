// -*- C++ -*-
//
// Copyright (C) 2002 Johnny Lai
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
 * @file BGLHelpers.h
 * @author Johnny Lai
 * @date 01 Feb 2003
 *
 * @brief General utility functions for manipulating BGL (Boost Graph Library)
 *
 */

#ifndef BGLHELPERS_H
#define BGLHELPERS_H 1


#include <boost/tuple/tuple.hpp> //tie
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp> //vertex_attribute_t


template <class Vertex, class Graph>
const std::string&
vertex_label(const Vertex& u, const Graph& g, const std::string& label = "label") {
  typename boost::property_map<Graph, boost::vertex_attribute_t>::const_type
    va = boost::get(boost::vertex_attribute, g);
  return (*(va[u].find(label))).second;
}

template<class Vertex, class Graph>
void set_vertex_label(const std::string& value, const Vertex& u, Graph& g, const std::string& label = "label")
{
  typename boost::property_map<Graph, boost::vertex_attribute_t>::type
    va = boost::get(boost::vertex_attribute, g);
  va[u][label] = value;
}

template<class Graph>
void print(Graph& g) {
  typename boost::graph_traits<Graph>::vertex_iterator i, end;
  typename boost::graph_traits<Graph>::out_edge_iterator ei, edge_end;
  for(boost::tie(i,end) = boost::vertices(g); i != end; ++i) {
    std::cout << vertex_label(*i, g) << " --> ";
    for (boost::tie(ei,edge_end) = boost::out_edges(*i, g);
         ei != edge_end; ++ei)
      std::cout << vertex_label(boost::target(*ei, g), g) << "  ";
    std::cout << std::endl;
  }
}

#endif /* BGLHELPERS_H */

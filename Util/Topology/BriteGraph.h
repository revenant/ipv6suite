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
 * @file BriteGraph.h
 * @author Johnny Lai
 * @date 22 Nov 2002
 *
 * @brief Boost Graph Library (BGL) compliant wrapper around Brite Graph class
 *
 * @test Run it through some test class that is mentioned in boost doco
 *
 * Class is to be used in reading Brite topology files as well as generating
 * them (originally Brite func) but coupled with BGL interface so it is easier
 * to create and manipulate simulation topology and use BGL algorithms.
 */

#ifndef BRITEGRAPH_H
#define BRITEGRAPH_H 1



/**
 * @class BriteGraph
 *
 * @brief BGL wrapper around Brite's Graph class
 *
 * detailed description
 */

namespace boost
{

  struct graph_traits< ::Graph >
  {
    typedef int vertex_descriptor;
    typedef int edge_descriptor;

    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef int vertices_size_type;
    typedef int edges_size_type;
  };

  typename graph_traits< graph_traits<Graph>::vertex_descriptor
  source(typename graph_traits<Graph>::edge_descriptor e, const Graph& g)
  {
    //Implement edge() by returning the corresponding eid in the list of
    //edges
    return g.edge(e)->GetSrc()->GetId();
  }

  typename graph_traits< graph_traits<Graph>::vertex_descriptor
  target(typename graph_traits<Graph>::edge_descriptor e, const Graph& g)
  {
    return g.edge(e)->GetDst()->GetId();
  }

};

namespace boost {
  struct out_edge_iterator_policies
  {
    static void increment(int& e)
    {
      Node* from = g.edge(e)->GetSrc();
      //Implement outEdge by iterating through Node->incEdges and returning the
      //one following this one.
      e = from->nextOutEdge(e);
    }

    static void decrement(int& e)
    {
      Node* from = g.edge(e)->GetSrc();
      //Implement outEdge by iterating backwards through Node->incEdges and
      //returning the one following this one.
      e = from->prevOutEdge(e);
    }


    template <class Reference>
    static Reference dereference(type<Reference>, const int& e)
    { return const_cast<Reference>(e); }

    static bool equal(const int& x, const int& y)
    { return x == y; }
  };
} // namespace boost

class BriteGraph
{
public:

  //@name constructors, destructors and operators
  //@{
  BriteGraph();

  ~BriteGraph();

  BriteGraph(const BriteGraph& src);

  BriteGraph& operator=(BriteGraph& src);

  bool operator==(const BriteGraph& rhs);
  //@}

protected:

private:

};


#endif /* BRITEGRAPH_H */

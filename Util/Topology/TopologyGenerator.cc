// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/Topology/TopologyGenerator.cc,v 1.2 2005/02/10 07:06:45 andras Exp $
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
 * @file   TopologyGenerator.cc
 * @author Johnny Lai
 * @date   09 Nov 2002
 *
 * @brief Generates the XML Network configuration file for a very simple
 * synthetic binary tree topology
 *
 *
 */


#include "sys.h"
#include "debug.h"


#include "TopologyGenerator.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>  // for boost::graph_traits

#include "ipv6_addr.h"

#include <boost/tuple/tuple.hpp> //tie
#include <boost/graph/visitors.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <boost/shared_ptr.hpp>

#include <utility>
#include <iostream>
#include <algorithm>
#include <vector>


using namespace boost;
using namespace std;


/**
 * @addtogroup Prototype
 * @{
 */

TopologyGenerator::TopologyGenerator(size_t nodeCount)
  :nodeCount(nodeCount)
{
}

TopologyGenerator::~TopologyGenerator()
{
}

/**
 * @class property_label_writer<Graph>
 * @brief Write out the node name as a label in graphviz dot format
 *
 * node name is obtained from "router" + vertex id
 */

template < class Graph, class Tag = boost::vertex_index_t>
class property_label_writer {
public:
  property_label_writer(Graph _graph):
    graph(_graph), pm(get(tag, graph)){}
  template <class VertexOrEdge>
  void operator()(std::ostream& out, const VertexOrEdge& v) const {
    out << "[label=\"router"<< get(pm, v)<<"\"]"; //name[v] << "\"]";
  }
private:
  Tag tag;
  Graph graph;
  typedef typename boost::property_map<Graph, Tag>::type PropertyMap;
  PropertyMap pm;
};

#include <libxml++/libxml++.h>

/**
 * @struct generateXML<Graph>
 *
 * @brief Write out the XML elements and attributes based on the information in
 * the BGL graph property maps as well as generating routes.
 */

template <class Graph> struct generateXML//: public boost::noncopyable
{
  struct doOnceOnly
  {
    doOnceOnly(generateXML& outer)
      :parent(outer)
      {}
    ~doOnceOnly()
      {
        parent.handler->on_end_document();
      }

    generateXML& parent;
  };

  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::vertices_size_type Vsize;

  generateXML(Graph& g_, XMLWriterHandler* handler, std::vector<Vertex>& p, Vsize* d):
    g(g_), handler(handler), doOnce(new doOnceOnly(*this)), p(p),
    d(d)
    {
      handler->on_start_document();
    }

  const Vertex& operator()(const Vertex& v)
  {
    const std::string router("router");
    char id[20];

    xpm.clear();
    snprintf(id, sizeof(id), "%d", get(vertex_index, g, v));
    //xmlproperty is a leak fix libxml++
    xpm["node"] = new xmlpp::Attribute("node", router + id);
    //determine if it is a router or host (assume all routers for now
    xpm["routePackets"] = new xmlpp::Attribute("routePackets", "on");

    handler->on_start_element("local", xpm);

    xpm.clear();
    unsigned int outdeg = out_degree(v,g);
    Dout(dc::custom, "generateXML out_deg="<<outdeg);

    for (unsigned int i = 0; i < outdeg; i++)
    {
      snprintf(id, sizeof(id), "ppp%d", i);
      xpm["name"] = new xmlpp::Attribute("name", id);
      handler->on_start_element("interface", xpm);

      xpm.clear();
      handler->on_start_element("inet_addr", xpm);
      //ipv6 address
      handler->on_characters(ipv6_addr_toString(get(vertex_name, g, p[v])).c_str());

      handler->on_end_element("inet_addr");
      handler->on_end_element("interface");
    }

    handler->on_start_element("route", xpm);

    typedef graph_traits<Graph> GraphTraits;
    typename GraphTraits::out_edge_iterator out_i, out_end;
    typename GraphTraits::edge_descriptor e;

    unsigned int ifIndex = 0;
    for (tie(out_i, out_end) = out_edges(v, g); out_i != out_end; ++out_i, ++ifIndex)
    {
      e = *out_i;
      Vertex targ = target(e, g);

      xpm.clear();
      snprintf(id, sizeof(id), "ppp%d", ifIndex);
      xpm["routeIface"] = new xmlpp::Attribute("routeIface", id);

      xpm["routeDestination"] = new xmlpp::Attribute("routeDestination", ipv6_addr_toString(get(vertex_name, g, p[targ])).c_str());

      xpm["isRouter"] = new xmlpp::Attribute("isRouter", "on");


      handler->on_start_element("routeEntry", xpm);
      handler->on_end_element("routeEntry");
    }

    handler->on_end_element("route");

    handler->on_end_element("local");

    return v;
  }
  Graph& g;
  XMLWriterHandler* handler;
  //xmlpp::PropertyMap xpm;
  xmlpp::Element::AttributeMap xpm;
  boost::shared_ptr<doOnceOnly> doOnce;
  std::vector<Vertex>& p;
  Vsize* d;
};

#include <boost/graph/graphviz.hpp>

/**
 * @struct generateBinaryTopology<GraphX>
 *
 * @brief Generates a simple binary tree topology
 *
 * Using BGL to produce topology
 */

template<class Graph> struct generateBinaryTopology
{
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iter;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

  /**
   * @brief Generate the Graphviz Dot representation of network topology
   *
   * Inner class required to overcome <algorithm> limitation of copying functors
   * rather than taking them by reference.  By putting statements in the
   * destructor of the parent class which would run many times (from copy ctor)
   * and placing them into the inner classes dtor we assure that this stuff is
   * done only once.  This is guaranteed by boost::shared_ptr. Checkout doOnce
   * member of Parent.
   *
   * Another thing to be wary of is that the functor has to be constructed in
   * place for generateXML case otherwise only the copy has edges added but the
   * original still contains only vertices.  This is caused by
   * generateBinaryTopology<Graph> data member graph which should have been a
   * reference (fixed).
   */

  template<typename GraphX> struct doOnceOnly
  {
    doOnceOnly(generateBinaryTopology<GraphX>& outer)
      :parent(outer)
      {}
    ~doOnceOnly()
      {
        std::map<std::string,std::string> graph_attr, vertex_attr, edge_attr;
        graph_attr["size"] = "3,3";
        graph_attr["rankdir"] = "LR";
        graph_attr["ratio"] = "fill";
        vertex_attr["shape"] = "circle";

        boost::write_graphviz(parent.os, parent.g, property_label_writer<Graph>(parent.g));
/*,
                        property_label_writer<Graph>(graph, edge_index),
                        make_graph_attributes_writer(graph_attr, vertex_attr,
                                                     edge_attr));*/
      }
    generateBinaryTopology<GraphX>& parent;
  };

  generateBinaryTopology(Graph& g, std::ostream& os)
    :g(g), rootNode(true), adjacentNodesCount(2), doOnce(new doOnceOnly<Graph>(*this)), os(os)
    {}

  void operator()(const Vertex& v)
  {
    traversed_nodes.push_back(v);

    if (*traversed_nodes.begin() == v)
      return;

      //Won't work now because vertex is stored in vector and by default vector
      //has node ids of indexes already (non-lvalue)
      //put(vertex_index, graph, *vit, i);

      if (add_edge(*traversed_nodes.begin(), v, g).second)
      {
        if (out_degree(*traversed_nodes.begin(), g) == adjacentNodesCount)
        {
          if (rootNode)
          {
            //Only root node has 2 neighbours other nodes have 3
            adjacentNodesCount++;
            rootNode = false;
          }

          traversed_nodes.pop_front();
        }

      }
      else
      {
        std::cerr<< " sorry failed to add vertex"<<v;
      }
  }

  Graph& g;
  std::list<Vertex> traversed_nodes;
  bool rootNode;
  size_t adjacentNodesCount;
  boost::shared_ptr<doOnceOnly<Graph> > doOnce;
  std::ostream& os;
};

/**
 * @struct generateAttributes<Graph>
 *
 * @brief Generate the ipv6_addr/prefix of node and place into BGL property map
 * vertex_name
 *
 * IPv6 address based on its distance from root node and also vertex index away
 * from the leftmost node
 */

template<class Graph> struct generateAttributes
{
  //typedef typename graph_traits<Graph>::vertex_iterator vertex_iter;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::vertices_size_type Vsize;

/**
 * @param g BGL Graph
 * @param p is the list of parents
 * @param d is the list of distances from root node
 */

  generateAttributes(Graph& g, std::vector<Vertex>& p, Vsize* d)
    :g(g), p(p), d(d)
    {}

  void operator()(const Vertex& v)
    {
      if (v == *vertices(g).first)
      {
        put(vertex_name, g, v, c_ipv6_addr("2001:00FF:0001:0001:0000:0000:0000:0001"));
        cur_vertex = v;
        return;
      }

      Vsize rank = d[v];
      if (d[cur_vertex] < rank)
      {
        cur_vertex = v;
      }

      unsigned int intra_rank_distance = v - cur_vertex;
      ipv6_addr paddr = get(vertex_name, g, p[v]);
      ipv6_addr iid = { 0, 0, 0, intra_rank_distance };
      unsigned int shift = 0;

      if (rank <= 2)
      {
        shift = NLA_BITSHIFT;

        //Modify NLA
        //put(vertex_name, g, v, (paddr & IPv6_ADDR_NLA_MASK)>>TLA_BITSHIFT+intra_rank_distance<<TLA_BITSHIFT);
      }
      else
      {
        //Modify SLA
        shift = SLA_BITSHIFT;

      }

      paddr = (((paddr>>shift)<<rank)<<shift);
      paddr += iid;

      put(vertex_name, g, v, paddr);

    }

  Graph& g;
  std::vector<Vertex>& p;
  Vsize* d;

  unsigned int order;
  //topologically ascendent vertex in current rank
  Vertex cur_vertex;

};

struct vertex_successors_t
{
  typedef boost::vertex_property_tag kind;
};

// create a typedef for the Graph type
typedef boost::adjacency_list<
  boost::listS, boost::vecS, boost::undirectedS,
  boost::property<boost::vertex_name_t, ipv6_addr,
//    boost::property<boost::vertex_rank_t, unsigned int,
//    boost::property<boost::vertex_predecessor_t, ipv6_addr,
  boost::property<vertex_successors_t, std::list<ipv6_addr>   > >,
  boost::property<boost::edge_name_t, std::string,
                  boost::property<boost::edge_weight_t, int> >
> IPv6SuiteTopology;
//Automatically included as a property when vertices are vecS
//,  boost::property<boost::vertex_index_t, int>


/**
 * @brief Generate an XML file with address and routing information
 *
 * Full topology information is in a separate topology file
 * Multiple runs over the graph object should allow me to add properties like
 * IPaddress and also the route. Topology format is in Graphviz dot format.
 *
 *
 * @param xmlHandler the SAX Write handler interface to output the XML configuration into
 * @param dotfile the name of the graphviz dot topology file
 */
template <class Graph>
void TopologyGenerator::outputXML(Graph& g, XMLWriterHandler* xmlHandler)
{
  typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;

  // Array to store predecessor (parent) of each vertex. This will be
  // used as a Decorator (actually, its iterator will be).
  std::vector<Vertex> parents(boost::num_vertices(g));

  // Array to store distances from the source to each vertex .  We use
  // a built-in array here just for variety. This will also be used as
  // a Decorator.
  typename boost::graph_traits<Graph>::vertices_size_type d[boost::num_vertices(g)];
  std::fill_n(d, sizeof(d)/sizeof(typename boost::graph_traits<Graph>::vertices_size_type), 0);

  // The source vertex
  Vertex s = *(boost::vertices(g).first);
  parents[s] = s;
  boost::breadth_first_search
    (g, s,
     boost::visitor(
       boost::make_bfs_visitor
       (std::make_pair(
         boost::record_distances(d, boost::on_tree_edge()),
         boost::record_predecessors(&parents[0], boost::on_tree_edge())))));

  std::for_each(vertices(g).first, vertices(g).second, generateAttributes<Graph>(g, parents, d));


  //generateXML<Graph> genXML(g);
  std::for_each(vertices(g).first, vertices(g).second, generateXML<Graph>(g, xmlHandler, parents, d));//genXML);
//  std::transform(vertices(g).first, vertices(g).second, vertices(g).first,
//                 generateXML<Graph>(g));
}

/**
 * @param g is the graph object to store the generated topology in
 * @param os the stream where the graphviz dot topology description will be written to
 *
 */
template <class Graph>
void TopologyGenerator::generateGraph(Graph& g,  std::ostream& os)
{
  //generateBinaryTopology<Graph> genTop(g);
  std::for_each(vertices(g).first, vertices(g).second, generateBinaryTopology<Graph>(g, os)); //genTop);
}

/**
 * @param g is the graph object to store the topology information in
 *
 * @param dotfileName name of graphviz dot format file to read topology info
 * from
 *
 * @todo Translate the topology structure from Graphviz into our custom Graph
 * object
 */

template <class GraphvizType, class Graph>
void TopologyGenerator::generateGraphFromGraphviz(Graph& g, const char* dotfileName)
{
  GraphvizType viz;
  read_graphviz(dotfileName, viz);

  while (num_vertices(viz) > num_vertices(g))
  {
    add_vertex(g);
  }

  typedef graph_traits<GraphvizType> GraphTraits;
  typename GraphTraits::edge_iterator i, end;
  typename GraphTraits::edge_descriptor e;
  typename GraphTraits::vertex_descriptor src, dest;

  for (tie(i, end) = edges(viz); i != end; ++i)
  {
    e=*i;
    src = source(e, viz);
    dest = target(e, viz);

    if (!add_edge(src, dest, g).second)
      DoutFatal(dc::core|error_cf, "Failed to add edge "<<src<<" --> "<<dest);
  }


}


#include "sys.h"
#include "debug.h"



#include <cstdlib>
#include "opp_utils.h"  // for int/double <==> string conversions

#define CQN 1
#ifndef CQN
#include "XMLWriterHandler.h"

int usage(const char* programName)
{
  cout <<" Usage: "<<programName<<" [nodeCount] [XMLOutput] [graphviz.dotfile] [-r]"<<endl;
  cout <<"        -r read topology from [graphviz.dotfile]"<<endl;
  cout <<" dot -Tpng -o graph.png example.dot"<<endl;
  return 1;
}
#else
#include "CQNTopologyGenerator.h"

int usageCQN(const char* programName)
{
  cout <<" Usage: "<<programName<<" [no of switches] [no of servers in tandem] [graphviz dotfile]"<<endl;
  return 1;
}
#endif

//@todo convert to getopt processing to handle all these cases
int main(int argc, char *argv[])
{
  Debug( dc::notice.on() );             // Turn on the NOTICE Debug Channel.
  Debug( dc::custom.on() );
  Debug( libcw_do.on() );               // Turn on the default Debug Object.

  const char* programName = argv[0];

  size_t nodeCount = 10;

#ifdef CQN
  if (argc < 2)
    return usageCQN(programName);

  size_t serverTandemCount = 0, switchCount = 0;


  using boost::lexical_cast;
  using boost::bad_lexical_cast;

  if (argc > 2)
    try
    {
      switchCount = boost::lexical_cast<short>(argv[1]);
      serverTandemCount = boost::lexical_cast<short>(argv[2]);
    }
    catch(boost::bad_lexical_cast & e)
    {
      cout <<"switch or servers in tandem cannot be converted to number: "<<e.what()<<endl;
      return usageCQN(programName);
    }

  nodeCount = serverTandemCount*switchCount + switchCount;

  CQNTopology cqn(nodeCount);

  string dotfile = "example.dot";
  std::ostream* os;
  if (argc > 3)
  {
    dotfile = argv[3];
  }
  os = new std::ofstream(dotfile.c_str(), std::ios::trunc);
  CQNTopologyGenerator<CQNTopology> cqntopogen(cqn, *os, switchCount, serverTandemCount);
  cqntopogen();

#else

  if (argc > 1)
    try
    {
      nodeCount = boost::lexical_cast<short>(argv[1]);
    }
    catch(boost::bad_lexical_cast & e)
    {
      cout <<"NodeCount cannot be converted to number: "<<e.what()<<endl;
      return usage(programName);
    }

  string filename = "output.xml";
  if (argc >= 3)
    filename = argv[2];

  TopologyGenerator gen(nodeCount);

  string dotfile = "example.dot";
  std::ostream* os;
  if (argc >= 4)
  {
    dotfile = argv[3];
  }

  bool generateTopology = true;
  if (argc >= 5)
  {
    generateTopology = false;
  }

  XMLWriterHandler xmlHandler(filename.c_str(), dotfile.c_str());

  IPv6SuiteTopology g(nodeCount);

  if (generateTopology)
  {
    os = new std::ofstream(dotfile.c_str(), std::ios::trunc);
    gen.generateGraph(g, *os);
    gen.outputXML(g, &xmlHandler);
    delete os;
  }
  else
  {
    gen.generateGraphFromGraphviz<GraphvizGraph>(g, dotfile.c_str());
    gen.outputXML(g, &xmlHandler);
  }
#endif //CQN

  return 0;
}

//@}

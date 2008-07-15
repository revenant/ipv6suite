// -*- C++ -*-
//
// Copyright (C) 2002 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file CQNTopologyGenerator.h
 * @author Johnny Lai
 * @date 23 Jan 2003
 * @brief Topology Generator for CQN (Closed Queing Network)
 *
 * Refer to Bagrodia for more details
 */

#ifndef CQNTOPOLOGYGENERATOR_H
#define CQNTOPOLOGYGENERATOR_H 1

#ifndef BGLHELPERS_H
#include "BGLHelpers.h"
#endif //BGLHEPERS_H

#ifndef BOOST_GRAPHVIZ_HPP
#include <boost/graph/graphviz.hpp>
#endif //BOOST_GRAPHVIZ_HPP

#ifndef BOOST_SHARED_PTR_HPP
#include <boost/shared_ptr.hpp>
#endif  //BOOST_SHARED_PTR_HPP


#include <sstream>


///@struct for storing the hostname to map to
struct vertex_map_hostname_t
{
  typedef boost::vertex_property_tag kind;
};
///@typedef for Parallel Close Queuing Network
typedef boost::adjacency_list<
  boost::listS, boost::vecS, boost::bidirectionalS,
//Required for Graphviz property writers
  boost::property<boost::vertex_attribute_t, boost::GraphvizAttrList,
                  boost::property<vertex_map_hostname_t, std::string> > ,
  boost::no_property
  > CQNTopology;
//Automatically included as a property when vertices are vecS
//,  boost::property<boost::vertex_index_t, int>


//Better to obtain these from some config file too i.e. TypeX and Need a
//better way to figure this out automatically at the other end esp if name
//of node is totally different from nodetype
const static char* type1 = "switch";
const static char* type2 = "queue";

/**
 * @struct CQNTopologyGenerator<GraphX>
 *
 * @brief Generates CQN Topology
 *
 * Using BGL to produce topology
 */
template<class Graph> struct CQNTopologyGenerator
{
  typedef typename boost::graph_traits<Graph>::vertex_iterator vertex_iter;
  typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;

  /**
   * @class property_label_writer<Graph>
   * @brief Write out the node name as a label in graphviz dot format
   *
   * node name is obtained from "router" + vertex id
   */

  template < class GraphX2, class Tag = boost::vertex_index_t>
  class property_label_writer {
  public:
    property_label_writer(GraphX2& _graph, unsigned int switchCount):
      graph(_graph), pm(get(tag, graph)), switchCount(switchCount){}
    template <class VertexOrEdge>
    void operator()(std::ostream& out, const VertexOrEdge& v) const {

      out << "[label=\""<<(get(pm,v)<switchCount?type1:type2)<< get(pm, v)<<"\"]"; //name[v] << "\"]";
    }

    template <class VertexOrEdge>
    void operator()(const VertexOrEdge& v) const
      {
        std::stringstream s;
        s<<(get(pm,v)<switchCount?type1:type2)<<(get(pm,v)<switchCount?get(pm,v):get(pm,v)-switchCount);
        set_vertex_label(s.str(), v, graph);
      }

  private:
    Tag tag;
    GraphX2& graph;
    typedef typename boost::property_map<GraphX2, Tag>::type PropertyMap;
    PropertyMap pm;
    unsigned int switchCount;
  };

/**
 * @class property_hostname_writer

 * @brief partition the CQN queues and switches to work on one host for each
 * tandem queue

 * @todo get rid of type vertex_map_hostname_t by using vertex_attribute see the graphviz label attribute used for nodenames to moduletype mapping
 */

  template < class GraphX2, class Tag = vertex_map_hostname_t>
  class property_hostname_writer {
  public:
    property_hostname_writer(GraphX2& _graph, unsigned int switchCount, unsigned int tandemCount):
      graph(_graph), pm(get(tag, graph)), switchCount(switchCount), tandemCount(tandemCount){}
    template <class VertexOrEdge>
    void operator()(const VertexOrEdge& v) const
      {
        static const char* hostnames[8] =
          {
            //"tangles0", "tangles1", "tangles2", "tangles3", "tangles4", "tangles5", "tangles6", "tangles7"
            "host0", "host1", "host2", "host3", "host4", "host5", "host6", "host7"
          };

        std::stringstream s;
        s<<(v<switchCount?hostnames[v]:hostnames[(v-switchCount)/tandemCount]);
        //set_vertex_label(s.str(), v, graph, "hostname");
        boost::put(pm, v, s.str());
      }
  private:
    Tag tag;
    GraphX2& graph;
    typedef typename boost::property_map<GraphX2, Tag>::type PropertyMap;
    PropertyMap pm;
    unsigned int switchCount, tandemCount;
  };



  template<typename GraphX> struct doOnceOnly
  {
    doOnceOnly(CQNTopologyGenerator<GraphX>& outer)
      :parent(outer)
      {}
    ~doOnceOnly()
      {
 //        std::map<std::string,std::string> graph_attr, vertex_attr, edge_attr;
//         graph_attr["size"] = "3,3";
//         graph_attr["rankdir"] = "LR";
//         graph_attr["ratio"] = "fill";
//         vertex_attr["shape"] = "circle";

        boost::write_graphviz(parent.os, parent.g, CQNTopologyGenerator<GraphX>::property_label_writer<Graph>(parent.g, parent.switchCount));

      }

    CQNTopologyGenerator<GraphX>& parent;
  };

  CQNTopologyGenerator(Graph& g, std::ostream& os, unsigned int switchCount, unsigned int serversInTandem)
    :g(g), doOnce(new doOnceOnly<Graph>(*this)), os(os), switchCount(switchCount), serversInTandem(serversInTandem)
    {}

  void operator()()
    {
      assert(num_vertices(g) == switchCount + serversInTandem*switchCount);

//      int nodeTypes = 2;

      std::vector<unsigned int> switchs(switchCount),
        servers(serversInTandem*switchCount);
      Vertex src;
      vertex_iter i, end;

      //Assign indices to the respective node types first to ease topology
      //generation
      unsigned int switchIndex=0, serverIndex=0;
      for (tie(i, end) = vertices(g); i != end; ++i)
      {
        src = *i;
        if (src < switchCount)
        {
          switchs[switchIndex] = src;
          switchIndex++;
        }
        else
        {
          servers[serverIndex] = src;
          serverIndex++;
        }
      }

      for (unsigned int i = 0; i < switchCount; i++)
      {
        for (unsigned int j = 0; j < switchCount; j++)
        {
          add_edge(switchs[i], servers[serversInTandem*j], g);
        }
      }

      for (unsigned int i = 0; i < switchCount; i++)
        for (unsigned int j = 0; j < serversInTandem-1; j++)
        {
          add_edge(servers[i*serversInTandem + j], servers[i*serversInTandem + j +1], g);
        }


      for (unsigned int i = 1; i <= switchCount; i++)
      {
        add_edge(servers[i*serversInTandem-1], switchs[i-1], g);
      }

    }

  void labelGraph()
    {
      vertex_iter vi, vend;
      boost::tie(vi, vend) = vertices(g);
      std::for_each(vi, vend, property_label_writer<Graph>(g, switchCount));
      std::for_each(vi, vend, property_hostname_writer<Graph>(g, switchCount, serversInTandem));
    }

  Graph& g;
  boost::shared_ptr<doOnceOnly<Graph> > doOnce;
  std::ostream& os;

  unsigned int switchCount, serversInTandem;

};

template <class Vertex, class Graph>
const std::string&
//vertex_label(const Vertex& u, const Graph& g, const std::string& label="label");
vertex_label(const Vertex& u, const Graph& g, const std::string& label);

void print (CQNTopology& g)
{
  vertex_map_hostname_t tag;
  boost::graph_traits<CQNTopology>::vertex_iterator i, end;
  boost::graph_traits<CQNTopology>::out_edge_iterator ei, edge_end;
  typedef boost::property_map<CQNTopology, vertex_map_hostname_t>::type PropertyMap;
  PropertyMap pm(get(tag, g));
  for(boost::tie(i,end) = boost::vertices(g); i != end; ++i) {
    std::cout << vertex_label(*i, g) << " --> ";
    for (boost::tie(ei,edge_end) = boost::out_edges(*i, g);
         ei != edge_end; ++ei)
      std::cout << vertex_label(boost::target(*ei, g), g) << " on "<<get(pm,boost::target(*ei, g))<<" ; ";

    std::cout << std::endl;
  }
}


#endif /* CQNTOPOLOGYGENERATOR_H */


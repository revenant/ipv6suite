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
 * @file   DynamicTopologyBuilder.cc
 * @author Johnny Lai
 * @date   20 Nov 2002
 *
 * @brief  Prototype Implementation of Dynamic Module Generation from XML
 *
 * Currently its limited to handling two different types of top level modules
 * only and only two sets of named gates i.e. in and out.
 */

#include "sys.h"
#include "debug.h"

#include "DynamicTopologyBuilder.h"

#include <fstream>
#include <boost/cast.hpp>

#include "libcwdsetup.h"
#include "BGLHelpers.h"

//#define ANTNET 1
//#define DYNAMICNET 1

//Currently CQN really just means a directed graph i.e. for graphs that do not
//have symmetrical in/out degrees
#define CQN 1

///@warning This can cause BGL compilation error (don't know which omnetpp exactly)
//#include <omnetpp.h>

namespace
{

cModuleType* createModuleType(const char* modTypename)
{
  cModuleType* modType = findModuleType(modTypename);
  if (!modType)
    DoutFatal(dc::fatal, "findModuleType failed for "<<modTypename<<" at "<<FILE_LINE);
  return modType;
}

}

typedef std::vector<cModule*> Nodes;

#include <boost/regex.hpp>
//#include "XMLDocHandle.h"
#include "opp_utils.h"  // for int/double <==> string conversions


//Leave here until createModules fixed
#ifdef CQN
#include "CQNTopologyGenerator.h"
#endif

//required for simulation.systemmodule.isLocalMachineIn
#include <cnetmod.h>

/**
   @struct createModules<Graph>
   @brief create module from the vertex object and also set module parameters

   Currently edgeRouterExp is only used by AntNet to distinguish between core
   and edge routers.

   @todo Get rid of dependency on vertex_map_hostname_t.  i.e. use vertex_attribute
   itself to determine if hostname mapping attribute exists thus deferring to
   runtime for answer instead of preventing compilation for certain builds
*/

template<class Graph> struct createModules
{

  typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename boost::graph_traits<Graph>::vertices_size_type Vsize;
  typedef typename boost::graph_traits<Graph>::edge_iterator EI;
  typedef typename boost::graph_traits<Graph>::degree_size_type EdgeSize;
  vertex_map_hostname_t tag;
  // Constructor/destructor.
  createModules(Graph& g, cModule* mod, Nodes& nodes /*, const std::auto_ptr<XMLConfiguration::XMLDocHandle>& xmlDoc*/,
                const char* routerRE, const char* edgeRouterRE, bool undirected = false)
    :g(g), mod(mod), nodes(nodes)/*, xmlDoc(xmlDoc)*/ ,routerExp(routerRE, true),
     edgeRouterExp(edgeRouterRE, true), pm(get(tag, g))
    {
    }

  unsigned int in_degree(boost::undirected_tag, const Vertex& v)
    {
      assert(false);
      std::cerr<<FILE_LINE<<": Serious compiler failure for compile time dispatch"<<std::endl;
    }

  unsigned int in_degree(boost::bidirectional_tag, const Vertex& v)
    {
      return boost::in_degree(v, g);
    }

  unsigned int in_degree(boost::directed_tag, const Vertex& v)
    {
      //Work it out manually
      EI ei,edge_end;
      EdgeSize inlinks = 0;
      for (boost::tie(ei,edge_end) = boost::edges(g); ei != edge_end; ++ei)
      {
        if (target(*ei,g) == v)
          inlinks++;
      }
      return inlinks;
    }

///@define check_error copied from ned generated sources
//OMNETPP_VERSION from defs.h
#if defined OMNETPP_VERSION && OMNETPP_VERSION == 0x0202
#define check_error() \
    {if (!simulation.ok()) return;}
#else
#define check_error() \
    {(void)0;}
#endif

#define check_memory() \
    {if (mod->memoryIsLow()) {opp_error(eNOMEM); return;}}

  void operator()(const Vertex& v)
    {

      cModule* createdMod = 0;

      // 'on:' section
      cArray machines;
      //Should return the logical mapping name e.g. host1
      cPar* par = new cPar;
      ///@warning This may not work as we are a simple module perhaps need to
      ///retrieve the parent compound module which does have these strings.
      *par = mod->parentModule()->machinePar(get(pm, v).c_str());
      machines.add(par);

      Dout(dc::debug, "hostname mapping: host"<<v<<" par="<<(const char*)*par);

      // module creation:
      bool islocal = simulation.netInterface()==NULL ||
        simulation.netInterface()->isLocalMachineIn( machines );

      Dout(dc::debug, "host"<<v<<" executed "<<(islocal?"locally":"remotely"));

      std::string nodename = vertex_label(v,g);
      boost::match_results<std::string::const_iterator> what;
      cModuleType* createType = 0;
      //use regex_grep
      if (routerExp.Match(nodename))
        createType = routerType;
      else
        createType = nodeType;

      createdMod = createType->create(nodename.c_str(), simulation.systemModule(), islocal);
      check_error(); //check_memory();

      // set machine list:
      createdMod->setMachinePar("default", ((cPar *)machines[0])->stringValue());
      check_error(); //check_memory();

      EdgeSize outlinks = out_degree(v, g);
      EdgeSize inlinks = outlinks;

      if (boost::is_directed(g))
      {
        typename boost::graph_traits<Graph>::directed_category cat;
        inlinks = in_degree(cat, v);
      }

      // set up parameters and gate sizes before we set up its submodules
#ifdef DYNAMICNET
      createdMod->par("numOfPorts") = outlinks;
#endif //DYNAMICNET

#ifdef ANTNET
      bool edgeRouter = edgeRouterExp.Match(nodename);
#endif //ANTNET

      if (inlinks != outlinks)
        Dout(dc::debug, createdMod->name()<<" outdeg="<<outlinks<<" indeg="<<inlinks);
      else
        Dout(dc::debug, createdMod->name()<<" outdeg="<<outlinks);


#ifdef ANTNET
      if (!edgeRouter)
      {
#endif //ANTNET
#ifndef ANTNET

        createdMod->setGateSize("in", inlinks);
        createdMod->setGateSize("out", outlinks);
#else
      }
      else
      {
        //AntNet
        createdMod->setGateSize("in", 1);
        createdMod->setGateSize("out", 1);
        createdMod->setGateSize("from_gen", outlinks - 1);
        createdMod->setGateSize("to_sink", outlinks - 1);
      }
#endif //ANTNET

      nodes[v] = createdMod;
    }

  static cModuleType *routerType;
  static cModuleType *nodeType;

private:
  Graph& g;
  cModule* mod;
  Nodes& nodes;
//  const std::auto_ptr<XMLConfiguration::XMLDocHandle>& xmlDoc;
  boost::RegEx routerExp, edgeRouterExp;
  typedef typename boost::property_map<Graph, vertex_map_hostname_t>::type PropertyMap;
  PropertyMap pm;

};

template<class Graph>
cModuleType* createModules<Graph>::routerType = 0;

template<class Graph>
cModuleType* createModules<Graph>::nodeType = 0;


/**
 * @brief Setup links between router nodes
 * @note Assuming bidirectional links with equivalent transport characteristics

 * @todo Links for ant net are going to be tough to do.  Perhaps create a
 * different function to do that.
 * Add soure and dest gate names as omnetpp.ini parameters.  Currently fixed to "in" and "out"
 * @note
 * Add soure and dest gate names as omnetpp.ini parameters. Currently fixed to "in" and "out"
 */

template<class Graph>
void setupLinks(Graph& g, const Nodes& nodes, cSimpleModule* mod)
{
  typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef typename boost::graph_traits<Graph>::degree_size_type Degree;

  ///Must be a local registration :(
  //Parsed from ned already
  /*
  cLinkType* slow = findLink( "intranetCable" );
  if (!slow)
    DoutFatal(dc::fatal, "intranetCable linkType not found");
  cLinkType* fast = findLink( "internetCable" );
  if (!fast)
    DoutFatal(dc::fatal, "internetCable linkType not found");
  cLinkType* curLinkType = fast;
  */

  //Probably better just to read from brite topo file for each links
  //characteristics or can make it part of links name in graphviz translation
  //and we parse the name for these link attributes.
  cPar* highDelay = new cPar("0.05");
  cPar* highBW = new cPar("10e9");
  cPar* lowDelay = new cPar("0.0000005");
  cPar* lowBW= new cPar("100e6");
  cPar* error =new cPar("0");

  cPar* curDelay = highDelay;
  cPar* curBW = highBW;
  cPar* curError = error;

#ifdef VERTICES
  typename boost::graph_traits<Graph>::vertex_iterator i, end;
  typename boost::graph_traits<Graph>::out_edge_iterator ei, edge_end;
  for(boost::tie(i,end) = boost::vertices(g); i != end; ++i)
  {
    Vertex src = *i;
    if (src == 10)
    {
      curDelay = lowDelay;
      curBW = lowBW;
      //curLinkType = slow;
    }


    unsigned int edgeIndex = 0;
    for (boost::tie(ei,edge_end) = boost::out_edges(src, g); ei != edge_end; ++ei, edgeIndex++)
    {
      // Create link in this direction src--->dest
      int srcGate = nodes[src]->findGate("out", edgeIndex);
      if (srcGate == -1)
      {
        Dout(dc::fatal|flush_cf, nodes[src]->name()<<":"<<edgeIndex<<" src gate not found");
        continue;
      }
#ifdef NOTPHOENIXCVS
      if (nodes[src]->gate(srcGate)->isConnectedOutside())
#else
      if (nodes[src]->gate(srcGate)->isConnected())
#endif
        continue;

      Edge e = *ei;
      Vertex dest = boost::target(e, g);
      unsigned int destEdgeIndex = 0;
      bool found = false;
      Degree outdeg = out_degree(dest, g);
      int destGate = 0;
      for (destEdgeIndex = 0; destEdgeIndex < outdeg; destEdgeIndex++)
      {
        destGate = nodes[dest]->findGate("in", destEdgeIndex);
        if(destGate == -1)
        {
          DoutFatal(dc::fatal, nodes[dest]->name()<<":"<<destEdgeIndex<<" dest gate not found");
        }
#ifdef NOTPHOENIXCVS
        if (!nodes[dest]->gate(destGate)->isConnectedOutside())
#else
        if (!nodes[dest]->gate(destGate)->isConnected())
#endif
        {
          found = true;
          break;
        }
      }

      assert(found);
      if (!found)
        DoutFatal(dc::fatal, FILE_LINE<<" Either algorithm is wrong or symmetry in links have been violated i.e. bidirectional link turned to directed graph");

      Dout(dc::debug|flush_cf, vertex_label(src, g) <<":"<<edgeIndex<< " --> "<<vertex_label(dest, g)<<":"<<destEdgeIndex);

      //todo figure out what channelType we want i.e. wher to obtain from
      //connect(nodes[src], srcGate, curLinkType, nodes[dest], destGate);
      connect(nodes[src], srcGate, curDelay, curError, curBW, nodes[dest], destGate);

      //Unnecessary to connect in the other direction since we are iterating
      //through all vertices anyway
/*
      // Create link in this direction dest--->src
      srcGate = nodes[dest]->findGate("out", destEdgeIndex);
      destGate = nodes[src]->findGate("in", edgeIndex);

      if (srcGate == -1)
      {
        Dout(dc::warning|flush_cf, nodes[dest]->name()<<":"<<edgeIndex<<" src gate not found");
        continue;
      }
      else if(destGate == -1)
      {
        Dout(dc::warning|flush_cf, nodes[src]->name()<<":"<<edgeIndex<<" dest gate not found");
        continue;
      }

      Dout(dc::debug|flush_cf,  nodes[src]->name()<<":"<<edgeIndex<<" <-- "<<nodes[dest]->name()<<":"<<edgeIndex);
      //connect(nodes[dest], srcGate, curLinkType, nodes[src], destGate);
      connect(nodes[dest], srcGate, curDelay, curError, curBW, nodes[src], destGate);
*/
    }

  }
#else

  typename boost::graph_traits<Graph>::edge_iterator ei, eend;
  Vertex src, dest;
  int srcGate, destGate, srcIndex, destIndex;
  std::string srcGateLabel("out"), destGateLabel("in");

  for (boost::tie(ei, eend) = edges(g); ei != eend; ei++)
  {
    src = source(*ei, g);
    dest = target(*ei, g);

    bool found = false;
    srcIndex = srcGate = nodes[src]->findGate(srcGateLabel.c_str(), 0);
    if (srcGate == -1)
    {
      Dout(dc::fatal|flush_cf, nodes[src]->name()<<":"<<srcGateLabel<<" src gate not found");
      continue;
    }

    assert(nodes[src]->gate(srcGate)->size() == (int) out_degree(src,g));

    for (;srcGate-srcIndex < (int) out_degree(src, g); srcGate++)
    {
      if (!nodes[src]->gate(srcGate)->toGate())//isRouteOK()//isConnected())
      {
        found = true;
        break;
      }
    }

    assert(found);

    found = false;

    destIndex = destGate = nodes[dest]->findGate(destGateLabel.c_str(), 0);
    if(destGate == -1)
    {
      DoutFatal(dc::fatal, nodes[dest]->name()<<":"<<destGateLabel<<" dest gate not found");
    }

    //assert(nodes[dest]->gate(destGate)->size() == in_degree(dest, g));

    for (;destGate-destIndex < nodes[dest]->gate(destGate)->size(); destGate++)
    {
      if (!nodes[dest]->gate(destGate)->fromGate())//isRouteOK()) //Connected())
      {
        found = true;
        break;
      }
    }

    assert(found);

    connect(nodes[src], srcGate, 0, 0, 0, nodes[dest], destGate);
    Dout(dc::debug|flush_cf, vertex_label(src, g) <<":"<<(srcGate-srcIndex)<< " --> "<<vertex_label(dest, g)<<":"<<(destGate-destIndex));

    //Go and connect in the other direction
    if (boost::is_undirected(g))
    {
      found = false;
      srcIndex = srcGate = nodes[dest]->findGate(srcGateLabel.c_str(), 0);
      if (srcGate == -1)
      {
        Dout(dc::fatal|flush_cf, nodes[dest]->name()<<":"<<srcGateLabel<<" src gate not found");
        continue;
      }
      for (;srcGate-srcIndex < (int) out_degree(dest, g); srcGate++)
      {
        if (!nodes[dest]->gate(srcGate)->toGate())
        {
          found = true;
          break;
        }
      }
      assert(found);

      found = false;

      destIndex = destGate = nodes[src]->findGate(destGateLabel.c_str(), 0);
      if(destGate == -1)
      {
        DoutFatal(dc::fatal, nodes[src]->name()<<":"<<destGateLabel<<" dest gate not found");
      }


      for (;destGate-destIndex < nodes[src]->gate(destGate)->size(); destGate++)
      {
        if (!nodes[src]->gate(destGate)->fromGate())
        {
          found = true;
          break;
        }
      }

      assert(found);

      connect(nodes[dest], srcGate, 0, 0, 0, nodes[src], destGate);
      Dout(dc::debug|flush_cf, vertex_label(dest, g) <<":"<<(srcGate-srcIndex)<< " --> "<<vertex_label(src, g)<<":"<<(destGate-destIndex));
    }
  }

#endif //VERTICES


  for (unsigned int i = 0; i < nodes.size(); i++)
  {
    createModules<Graph>::routerType->buildInside( nodes[i] );
    nodes[i]->scheduleStart( mod->simTime() );
  }
}

//Leave it here cause #include omnetpp.h is deadly
#ifdef WORLDPROCESSOR_H
#include "opp_utils.h"
#include "IPv6XMLManager.h"
#include "XMLDocHandle.h"
#else
#include <cpar.h>
#endif //WORLDPROCESSOR_H
#include <boost/scoped_array.hpp>

#ifdef CQN
#include "CQNTopologyGenerator.h"
#endif

using namespace std;


DynamicTopologyBuilder::~DynamicTopologyBuilder()
{}

/**
 * @brief Parse XML and build modules dynamically.
 *
 *  If we are to use Sax then we must create a SAXhandler and pass an instance
 *  of this class into it so that the current Module gets created with
 *  parameters and the appropriate gate size based on number of interface
 *  elements.
 *
 *  There aren't really any parameters in the top level nodes, most parameters
 *  are specified in ini file and parsed into program by itself.  Perhaps we
 *  should start the modules only after we have created them all or schedule
 *  them to start at same time as long as buildInside does not invoke initialise
 *
 *  We should add the actual adjacency information in directly rather than
 *  leaving it as the xml addresses because that is just quicker and easier than
 *  searching for dest addr in different xml elements. e.g. print the graphviz
 *  rep into an element type.
 *
 *  Actually sticking with DOM because that works at a node level and we can
 *  extract the node parameters and set them before doing buildInside.  Also
 *  since DOM already parses everything at start this is prob. better and easier
 *  to implement for now.  Only in future if size of XML file becomes really
 *  unwieldy would we move to SAX. In order to use SAX we'd have to have a chain
 *  of SAX handlers to delineate the different objectives (Problem with current
 *  DOM is the functions for parsing simulation data are all over the place).
 *
 *  XML file shall have an element for the topology file to load.  Don't really
 *  want to include it seperately since BGL already has a graphviz parser.  Use
 *  awk script to convert Brite to graphviz.  Latter on if we really want all
 *  the information BRITe offers then create a Brite parser in BGL or wrap BRITE
 *  Graph/Node/Edge.
 */
void DynamicTopologyBuilder::initialize()
{
  Debug(libcwdsetup::l_debugSettings("debug.log:debug:notice"); );

  //Get topo filename from XML
  string topoFilename = "example.dot";
#ifdef WORLDPROCESSOR_H
  WorldProcessor* wp = boost::polymorphic_downcast< ::WorldProcessor*> (
    OPP_Global::findModuleByType(this, "WorldProcessor"));
  assert(wp != 0 );

  typedef boost::scoped_array<char> MyString;
  topoFilename = MyString(wp->xmlManager()->doc()->doc().getDocumentElement().getAttribute(DOMString("topologyFile")).transcode()).get();
#else
  topoFilename = static_cast<const char*> (par("topoFilename"));
#endif //WORLDPROCESSOR_H

  if (topoFilename.empty())
    DoutFatal(dc::fatal|error_cf, "No Topology file specified");

  std::ifstream f(topoFilename.c_str());
  if (!f)
    DoutFatal(dc::fatal|error_cf, "Topology file: "<<topoFilename<<" does not exist");
  else
    f.close();


  //Start creating topology
#ifdef CQN
  typedef CQNTopology GraphType;
  unsigned int serverTandemCount = 4, switchCount = 4;
  unsigned int nodeCount = serverTandemCount*switchCount + switchCount;
  GraphType g(nodeCount);
  CQNTopologyGenerator<GraphType> cqntopogen(g, cout, switchCount, serverTandemCount);
  cqntopogen();
  cqntopogen.labelGraph();
#else
  typedef boost::GraphvizGraph GraphType;
  GraphType g;
  read_graphviz(topoFilename, g);
#endif //CQN

  print(g);

  //Improve by having a list of node types and just call them type1/type2 etc
  //and load all their types.  Then in the dot file the labels of those modules
  //should have the typename.
  createModules<GraphType>::routerType = createModuleType(par("routerType"));
  createModules<GraphType>::nodeType = createModuleType(par("nodeType"));

  Nodes nodes(num_vertices(g));

  std::for_each(vertices(g).first, vertices(g).second, createModules<GraphType>(g, this, nodes/*,wp->xmlManager()->doc()*/, par("routerRegExp"), par("nodeRegExp")));

  setupLinks(g, nodes, this);

}


void DynamicTopologyBuilder::handleMessage(cMessage* msg)
{}

void DynamicTopologyBuilder::finish()
{}


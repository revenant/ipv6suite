// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/XML/Attic/IPv6XMLParser.h,v 1.1 2005/02/09 06:15:59 andras Exp $
//
// Copyright (C) 2001 CTIE, Monash University 
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
 * @file IPv6XMLParser.h
 *
 * @brief Read in the interfaces and routing table from a file and parse
 * information to RoutingTable6
 *
 * @author Eric Wu
 *
 * @date 10/11/2001
 *
 */

#ifndef IPV6XMLPARSER_H
#define IPV6XMLPARSER_H

#ifndef XMLPARSER_H
#include "XMLParser.h"
#endif

class IPv6Address;
class RoutingTable6;
class DOM_Node;
class WirelessEtherModule;
class MobilityStatic;
class MobilityRandomWalk;

namespace IPv6NeighbourDiscovery
{
  class PrefixEntry;
}

namespace Loki
{
  template <class T> class CreateUsingNew;
}

namespace XMLConfiguration
{

  class IPv6XMLParser;
  
/**
 * @class IPv6XMLParser
 * @brief Parsing of IPv6 Network configuration
 *
 * There is only one IPv6XMLParser in the whole simulation.  This is wrapped up by 
 * IPv6XMLManager.
 *
 * @todo Leave only IPv6 specific parsing functions and put all OMNeT++ module
 * instance specific code in IPv6XMLManager that way we don't have to pass
 * pointers around everywhere.  What is better is to totally eliminate all
 * module references and return app neutral lists of objs and let RoutingTable6
 * add routes, tunnels and other config to appropriate modules.  Also the _mod
 * inherited member from XMLParser is totally inappropriate.
 */

class IPv6XMLParser : public XMLParser
{
  friend class IPv6XMLParserTest;
  friend class IPv6XMLManager;
  friend class Loki::CreateUsingNew<IPv6XMLParser>;
  
public:

  const std::auto_ptr<XMLDocHandle>& doc() const { return _XMLDoc; }
  
  size_t version() const { return _version; }
  
  virtual void parse(RoutingTable6* rt);

  // TODO: may have to change these functions to a better structure

  // return a list of prefix entries
  IPv6NeighbourDiscovery::PrefixEntry* prefixes(const string& netNodeName,
                                               size_t iface_index, 
                                               size_t& numOfPrefixes);

  /// static routing table
  void staticRoutingTable(RoutingTable6* rt, const string& netNodeName);

  void tunnelConfiguration(RoutingTable6* rt, const DOM_Node& netNode);
  
  void sourceRoute(RoutingTable6* rt, const DOM_Node& netNode);

#ifdef USE_MOBILITY
  void parseWirelessEtherInfo(WirelessEtherModule* mod);

  // parse movement information
  void parseMovementInfo(MobilityStatic* mod);

  // parse movement information
  void parseRandomWalkInfo(MobilityRandomWalk* mod);
#endif //USE_MOBILITY  

#ifdef USE_HMIP
  // parse MAP information
  void parseMAPInfo(RoutingTable6* rt);
#endif // USE_HMIP

  /// return a list of IPv6 addresses
  IPv6Address* addresses(RoutingTable6* rt, const string& netNodeName, size_t iface_index, size_t& numOfAddrs);

private:
  IPv6XMLParser(void);

  ~IPv6XMLParser(void);

  // traverse through the XML document
  void traverse(NetParam& value, 
                const string& netNodeName = "", 
                int iface_index = 0);

  bool findNetNode(const string& netNodeName, DOM_Node& netNode);

  bool findNetNodeIface(const string& netNodeName, DOM_Node& netNodeIface, 
                        int iface_idx);

  size_t retrieveVersion();

  ///Returns a string for further tokenising into logfile & debug channel names
  string retrieveDebugChannels();
  
  size_t _version;
};


}

#endif

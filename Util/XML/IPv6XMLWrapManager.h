// -*- C++ -*-
// Copyright (C) 2003, 2004 Johnny Lai
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
 * @file IPv6XMLWrapManager.h
 * @author Johnny Lai
 * @date 15 Jul 2003
 *
 * @brief definition of class IPv6XMLWrapManager
 *
 *
 */

#ifndef IPV6XMLWRAPMANAGER_H
#define IPV6XMLWRAPMANAGER_H 1

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp>
#endif //BOOST_UTILITY_HPP

#define ATEXIT_FIXED 1
#include "Singleton.h" //Loki

#ifndef IPV6XMLWRAPPARSER_H
#include "XMLWrapParser.h"
#endif //IPV6XMLWRAPPARSER_H
#ifndef XMLDOCHANDLE_H
#include "XMLDocHandle.h"
#endif //XMLDOCHANDLE_H


namespace xml
{
  class document;
  class node;
}

class RoutingTable6;
class WirelessEtherModule;
class MobilityStatic;
class MobilityRandomWP;
class MobilityRandomPattern;
class IPv6XMLWrapManagerTest;

namespace XMLConfiguration
{

/**
 * @class IPv6XMLWrapManager
 *
 * @brief IPv6Suite XML Parser
 *
 * Manually written to parse the netconf2.dtd. Versioning of the schema is done
 * by applying the correct defaults for a particular version and only parsing
 * elements that exist in newer versions.
 */

class IPv6XMLWrapManager: boost::noncopyable
{
 public:
  typedef xml::node::const_iterator NodeIt;
  typedef xml::node Node;
  friend class IPv6XMLWrapManagerTest;
  friend class RoutingAlgorithmStatic;

  typedef Loki::SingletonHolder
  <XMLWrapParser, Loki::CreateUsingNew, Loki::PhoenixSingleton,
   Loki::SingleThreaded> SingularParser;

  const xml::document& doc() const 
    { 
      return SingularParser::Instance().doc();
    }

  const xml::node& root() const
    {
      return doc().get_root_node();
    }

#ifdef USE_MOBILITY
  void parseWirelessEtherInfo(WirelessEtherModule* mod);

  // parse movement information
  void parseMovementInfo(MobilityStatic* mod);

  // parse radom walk information
  void parseRandomWPInfo(MobilityRandomWP* mod);

  // parse radom pattern information
  void parseRandomPatternInfo(MobilityRandomPattern* mod);
#endif //USE_MOBILITY  

#ifdef USE_HMIP
  // parse MAP information
  void parseMAPInfo(RoutingTable6* rt);
#endif // USE_HMIP
  
  void staticRoutingTable(RoutingTable6* rt);

  ///Returns a string for further tokenising into logfile & debug channel names
  std::string retrieveDebugChannels();

  unsigned int version() const;


  //@name constructors, destructors and operators
  //@{
  IPv6XMLWrapManager(const char* filename);

  ~IPv6XMLWrapManager();

  //@}

  void parseNetworkEntity(RoutingTable6* rt);
 protected:
  
 private:

  void tunnelConfiguration(RoutingTable6* rt);
  void sourceRoute(RoutingTable6* rt);

  /// parse node level attributes
  void parseNodeAttributes(RoutingTable6* rt);

  NodeIt getNetNode(const char* name) const;
  std::string getNodeProperties(const xml::node& n, const char* attrName, bool required = true) const;

  mutable unsigned int _version;
};


}

#endif /* IPV6XMLWRAPMANAGER_H */


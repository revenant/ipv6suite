// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/XML/Attic/IPv6XMLManager.h,v 1.1 2005/02/09 06:15:59 andras Exp $
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
 * @file IPv6XMLManager.h
 * @author Johnny Lai
 * @date 08 Apr 2002
 * @brief Manages the IPv6XMLParser to read the XML file once only for all
 * network nodes in simulation.  Should save lots of memory.  Perhaps
 * testNetwork won't return a null pointer after this and can work again.  
 * @test see IPv6XMLParserTest
 * @todo see IPv6XMLParser for details
 */

#ifndef IPv6XMLMANAGER_H
#define IPv6XMLMANAGER_H 1

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp>
#endif //BOOST_UTILITY_HPP

#define ATEXIT_FIXED 1
#include "Singleton.h"

#ifndef IPV6XMLPARSER_H
#include "IPv6XMLParser.h"
#endif //IPV6XMLPARSER_H

class RoutingTable6;

#ifdef USE_MOBILITY
class WirelessEtherModule;
class MobilityStatic;
class MobilityRandomWalk;
#endif // USE_MOBILITY

namespace XMLConfiguration
{
  
  /**
   * @class IPv6XMLManager
   *
   * @brief Manage global IPv6XMLParser. Provide XML interface for simulation.
   *
   * @todo see IPv6XMLParser for details.  Its rather clumsy at the moment
   * Takes over the task of reading the XML configuration file once only for the
   * entire simulation from RoutingTable6
   *
   * @sa RoutingTable6 IPv6XMLParser
   */
  class IPv6XMLManager: boost::noncopyable
  {
    friend class IPv6XMLParserTest;
    
    typedef Loki::SingletonHolder
    <IPv6XMLParser, Loki::CreateUsingNew, Loki::PhoenixSingleton,
     Loki::SingleThreaded> SingularParser;

  public:

    IPv6XMLManager(const char* filename);

    ~IPv6XMLManager();

    const std::auto_ptr<XMLDocHandle>& doc() const 
      { 
        return SingularParser::Instance()._XMLDoc;
      }

    void parseNetworkEntity(RoutingTable6* mod);

    void staticRoutingTable(RoutingTable6* rt, const std::string& nodeName)
      {
        SingularParser::Instance().staticRoutingTable(rt, nodeName);
      }

    string retrieveDebugChannels()
      {
        return SingularParser::Instance().retrieveDebugChannels();
      }

#ifdef USE_MOBILITY    
    void parseWirelessEtherInfo(WirelessEtherModule* mod)
      {
        SingularParser::Instance().parseWirelessEtherInfo(mod);
      }

    void parseMovementInfo(MobilityStatic* mod)
      {
        SingularParser::Instance().parseMovementInfo(mod);
      }

    void parseRandomWalkInfo(MobilityRandomWalk* mod)
      {
        SingularParser::Instance().parseRandomWalkInfo(mod);
      }

#endif //USE_MOBILITY

#ifdef USE_HMIP
    void parseMAPInfo(RoutingTable6* rt)
      {
        SingularParser::Instance().parseMAPInfo(rt);
      }
#endif

  private:
    
    /**
       construct mapping between the parametres and the XML
       tagname. When there is a new parametre, this is the first place
       to set up the mapping to be parsed from the XML file
    */
    void constructXMLMapping(RoutingTable6* mod);

    /// parse node level attributes
    void parseNodeAttributes(RoutingTable6* rt);

    /// Just load XML file into XML parser for all network entities to use
    void readRoutingTableFromFile(const char* filename);

  };
 
} //namespace XMLConfiguration


#endif //IPv6XMLMANAGER_H


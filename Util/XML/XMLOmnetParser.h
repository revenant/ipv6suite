// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
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
 * @file XMLOmnetParser.h
 * @author Johnny Lai
 * @date 08 Sep 2004
 *
 * @brief Definition of class XMLOmnetParser
 *
 *
 */

#ifndef XMLOMNETPARSER_H
#define XMLOMNETPARSER_H

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

class cXMLElement;
class InterfaceTable;
class RoutingTable6;
class WirelessEtherModule;
class MobilityStatic;
class MobilityRandomWP;
class MobilityRandomPattern;

namespace XMLConfiguration
{

/**
 * @class XMLOmnetParser
 *
 * @brief OMNet++ XML Parser
 *
 * Use omnetpp's inbuilt XML parser
 *
 * @note preferred and supported method (the others are deprecated)
 */

class XMLOmnetParser
{
 public:
#ifdef USE_CPPUNIT
  friend class XMLOmnetParserTest;
#endif //USE_CPPUNIT
  friend class IPv6XMLOmnetManager;

  //@name constructors, destructors and operators
  //@{
  XMLOmnetParser();

  ~XMLOmnetParser();

  //@}

  void parseFile(const char* filename);

  ///Make sure that such an attribute exists (required) otherwise will abort
  std::string getNodeProperties(const cXMLElement* netNode, const char* attrName, bool required = true) const;

  /**
     @brief Returns true or false depending on value of attribute attrName.
     @warning no tests are done to check if attrName is a boolean attribute
  */
  bool getNodePropBool(const cXMLElement* netNode, const char* attrName);

  cXMLElement* getNetNode(const char* name) const;

  cXMLElement* doc() const
  {
    return root;
  }

  ///Actual parsing functions
  //@{
#ifdef USE_MOBILITY
  /**
   * Fills in parameters inside the given WirelessEtherModule.
   */
/*XXX these params went to NED parameters
  void parseWirelessEtherInfo(WirelessEtherModule* mod);
*/
  /**
   * Fills in parameters inside the given WirelessEtherModule.
   */
/*XXX these params went to NED parameters
  void parseWEInfo(WirelessEtherModule* wlanMod, cXMLElement* weInfo);
*/
  /**
   * Fills in parameters inside the given MobilityStatic.
   */
  void parseMovementInfo(MobilityStatic* mod);

  /**
   * Fills in parameters inside the given MobilityRandomWP and
   * MobilityRandomWalk.
   */
  void parseRandomWPInfo(MobilityRandomWP* mod);

  /**
   * Fills in parameters inside the given MobilityRandomPattern.
   */
  void parseRandomPatternInfo(MobilityRandomPattern* mod);
#endif //USE_MOBILITY

#ifdef USE_HMIP
  /**
   * Fills in the MAP info part of the given RoutingTable6 data structure
   * for the host whose hostname is passed in the RoutingTable6 data structure.
   */
  void parseMAPInfo(InterfaceTable *ift, RoutingTable6 *rt);
#endif // USE_HMIP

  /**
   * Fills in the given RoutingTable6 data structure with the static routing table
   * of the host given by the nodeName parameter.
   */
  void staticRoutingTable(InterfaceTable *ift, RoutingTable6 *rt);

  ///Returns a string for further tokenising into logfile & debug channel names
  std::string retrieveDebugChannels();

  unsigned int version() const;

  /**
   * Fills in the given RoutingTable6 data structure with the routing table
   * of the host whose hostname is passed in the RoutingTable6 data structure.
   */
  void parseNetworkEntity(InterfaceTable *ift, RoutingTable6 *rt);

 protected:

 private:

  void tunnelConfiguration(InterfaceTable *ift, RoutingTable6 *rt);
  void sourceRoute(InterfaceTable *ift, RoutingTable6 *rt);

  /// parse node level attributes
  void parseNodeAttributes(RoutingTable6* rt, cXMLElement* ne);
  /// parse the attributes of interface nif at index iface_index
  void parseInterfaceAttributes(InterfaceTable *ift, RoutingTable6* rt, cXMLElement* nif, unsigned int iface_index);
  //@}


  //@{ disable generation
  XMLOmnetParser(const XMLOmnetParser& src);

  XMLOmnetParser& operator=(XMLOmnetParser& src);
  //@}

  std::string _filename;

  mutable unsigned int _version;

  cXMLElement* root;

};

};

#endif /* XMLOMNETPARSER_H */


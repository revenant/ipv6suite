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
 * @file XMLWrapParser.h
 * @author Johnny Lai
 * @date 15 Jul 2003
 *
 * @brief Definition of class XMLWrapParser
 *
 *
 *
 *
 */

#ifndef XMLWRAPPARSER_H
#define XMLWRAPPARSER_H 1

#ifndef STRING
#define STRING
#include <string> 
#endif //STRING

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp> //boost::noncopyable
#endif

#ifndef _xmlwrapp_tree_parser_h_
#include <xmlwrapp/tree_parser.h>
#endif //_xmlwrapp_tree_parser_h_

namespace xml
{
  class node;
  class init;
}


namespace XMLConfiguration
{

  class XMLDocHandle;

/**
 * @class XMLWrapParser
 *
 * @brief xmlwrapp parser handle
 *
 * replacing XMLParser which uses Xerces-c
 */

class XMLWrapParser: boost::noncopyable
{
 public:
  friend class XMLWrapParserTest;
  friend class IPv6XMLWrapManager;

  void parseFile(const char* filename);

  ///Make sure that such an attribute exists (required) otherwise will abort
  std::string getNodeProperties(const xml::node& netNode, const char* attrName, bool required = true) const;

  XMLWrapParser();
  ~XMLWrapParser();

 const xml::document& doc() const 
    {
      return parser->get_document();
    }
 protected:

  xml::tree_parser* parser;
 private:

  ///Only want this done exactly once only
  xml::init* xmlinit;
  std::string _filename;
};



}

#endif /* XMLWRAPPARSER_H */


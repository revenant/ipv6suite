//
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
 * @file   XMLWrapParser.cc
 * @author Johnny Lai
 * @date   15 Jul 2003
 *
 * @brief Implementation of xmlwrapp parser handle
 *
 *
 *
 */

#include "sys.h"
#include "debug.h"

#include "XMLWrapParser.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include "opp_utils.h"  // for int/double <==> string conversions
#include <exception>

//#include <xsltwrapp/init.h>
#include <xmlwrapp/xmlwrapp.h>

namespace XMLConfiguration
{
  using std::cerr;
  using std::endl;
  using std::cout;

///@name General libxmlwrapp functions
//@{
///Only looks into n's direct descendents
xml::node::const_iterator
find_first_element_by_value(const xml::node& n, const char* elementName,
          const char* attrName, const char* attrValue);

std::string get_attribute_value(const xml::node& n, const char* attrName, bool required = true);
//@}

/**
   @return n.end() if element not found
*/
xml::node::const_iterator
find_first_element_by_value(const xml::node& n, const char* elementName,
          const char* attrName, const char* attrValue)
{
  assert(elementName && attrName[0] != '\0');
  assert(attrName && attrName[0] != '\0');
  assert(attrValue && attrValue[0] != '\0');

  typedef xml::node::const_iterator NodeIt;
  typedef xml::attributes::const_iterator AttIt;
  for (NodeIt cit = n.begin(); cit != n.end(); cit++)
  {
    if ((*cit).get_type() != xml::node::type_element)
      continue;
    if (strcmp((*cit).get_name(), elementName))
      continue;
    AttIt ait;
    if ((ait = (*cit).get_attributes().find(attrName)) != (*cit).get_attributes().end() && !strcmp((*ait).get_value(), attrValue))
      return cit;
  }
  return n.end();
}

/**
   @warning For some inane reason xmlwrapp prefers not to return values with a
   default of "" in the DTD instead it will just throw some exception. I'll have
   to put a hack so when required is false just search for the attribute via
   iteration and not find and so if I don't find it I can return a "". I'll have
   to manually place required = false for dtd with default value of "". I could
   put some other bogus value like hello but that would confuse xerces which
   allows DTD defaults of "".
   When we migrate to XSD without any support for default value in xmlwrapp I
   would set my own default values manually and this would not be a problem
   anymore.

   @arg required If false will not use find() and can return "". If DTD default
   value is "" please set to false otherwise an exception is thrown if value is
   not specified.
 */
std::string get_attribute_value(const xml::node& n, const char* attrName, bool required)
{
  typedef xml::attributes::const_iterator AttIt;
  AttIt ait;
  try
  {
    if ((ait=n.get_attributes().find(attrName)) != n.get_attributes().end())
    {
       Dout(dc::xml_addresses|flush_cf, "read value = "<< (*ait).get_value()
            <<" "<<(*ait).get_name());
      return (*ait).get_value();
    }
  }
  catch(std::exception& e)
  {
    Dout(dc::warning, "Exception in parsing attribute "<<attrName<<":"<<e.what());
  }
  catch(...)
  {
    cerr<<"Unknown exception thrown"<<endl;
  }

  if (required)
    DoutFatal(dc::core, "Unable to find attribute with name "<<attrName);
  return "";
}

///For non omnetpp csimplemodule derived classes
XMLWrapParser::XMLWrapParser():parser(0)
{
  //xsltinit = new xslt::init;
  xmlinit = new xml::init;
}

XMLWrapParser::~XMLWrapParser()
{
  //delete xsltinit;
  delete xmlinit;
  delete parser;
}


void XMLWrapParser::parseFile(const char* filename)
{
  delete parser;
  parser =  0;
  if(!filename || '\0' == filename[0])
  {
    cerr << "Empty name for XML input file"<<endl;
    exit(1);
  }

  _filename = filename;
  const bool throw_exceptions = false;
  parser = new  xml::tree_parser(filename, throw_exceptions);
  if (!(*parser))
  {
    cerr<<"Error parsing XML file "<<filename<<" "<<parser->get_error_message()<<endl;
    exit(6);
  }
  else if (parser->had_warnings())
  {
    cerr<<"Warnings encountered when parsing file "<<filename<<endl;
    cerr<<"Run xmllint for further information"<<endl;
    exit(7);
  }

  // get XML document
  xml::document &doc = parser->get_document();

  if (!doc.has_external_subset() && !doc.has_internal_subset())
  {
    //cerr<<"XML document "<<filename<<" does not contain any DTDs at all"<<endl;
    cout<<"Make sure "<<filename<<" is validated externally"<<endl;
    //exit(8);
  }
  else
  {
    if (!doc.validate())
//      cout<<"doc validation returned="<<boost::lexical_cast<bool>(doc.validate())<<endl;
//    else
    {
      cerr<<"XML document "<<filename<<" fails to validate"<<endl;
      exit(9);
    }
  }

}

std::string XMLWrapParser::getNodeProperties(const xml::node& n, const char* attrName, bool required) const
{
  return get_attribute_value(n, attrName, required);
}


} //end namespace XMLConfiguration


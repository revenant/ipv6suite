// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/XML/Attic/XMLParser.cc,v 1.1 2005/02/09 06:15:59 andras Exp $
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
 * @file XMLParser.cc
 *    
 * @brief Read in the interfaces and routing table from a file and parse
 * information to cSimpleModule where specified
 *
 * @author  Eric Wu
 *
 * @date    10/11/2001
 *
 * Parsing the XML into cSimpleModule
 *
 */


#include <iostream>

#include <xercesc/util/PlatformUtils.hpp> // general file input/output stream
#include <xercesc/util/XMLString.hpp>     // general XML string
#include <xercesc/parsers/DOMParser.hpp> // XML parser
#include <xercesc/dom/DOM_DOMException.hpp>

#include "XMLParser.h"
#include "XMLDocHandle.h"
#include "DOMTreeErrorReporter.h" // Error handler

using namespace std;

namespace XMLConfiguration
{

XMLParser::XMLParser(void)
  : _filename("") 
{}
  
XMLParser::~XMLParser(void)
{
  XMLPlatformUtils::Terminate();    
}

void XMLParser::initialize(void)
{
  if(_filename.empty())
  {
    cerr << "No input network file"<<endl;
    exit(1);    
  }     

  // Initialize the XML4C2 system
  try
  {
    XMLPlatformUtils::Initialize();
  }

  catch(const XMLException &toCatch)
  {
    cerr << "Error during Xerces-c Initialization.\n"
         << "  Exception message:"
         << DOMString(toCatch.getMessage()).transcode()
         << endl;
    exit(2);
  }    

  DOMParser* parser = new DOMParser;
  parser->setValidationScheme(DOMParser::Val_Always);
  parser->setDoNamespaces(true);
  parser->setDoSchema(true);
  DOMTreeErrorReporter* errReporter = new DOMTreeErrorReporter();
  parser->setErrorHandler(errReporter);

  try
  {
    parser->parse(_filename.c_str());
  }

  catch (const XMLException& e)
  {    
    cerr << "An error occured during parsing\n   Message: "
         << DOMString(e.getMessage()).transcode() << endl;
    exit(3);    
  }  

  catch (const DOM_DOMException& e)
  {
    cerr << "A DOM error occured during parsing\n   DOMException code: "
       << e.code << endl;
    exit(4);    
  }

  catch (...)
  {    
    cerr << "An error occured during parsing\n " << endl;
    exit(5);    
  }

  // get XML document
  _XMLDoc.reset(new XMLDocHandle(parser->getDocument()));
}

void XMLParser::saveInfo(const string& key, char* value_str, 
                         const void* value, DATATYPE type)
{
  switch(type)
  {
      case TYPE_BOOL:
        if(!strcasecmp(value_str, "on"))
          *((bool*)value) = 1;
        else if(!strcasecmp(value_str, "off"))
          *((bool*)value) = 0;
        else
        {          
          cerr<<key.c_str()<<" not set correctly!"<<endl;
          exit(6);
        }        
        break;
            
      case TYPE_INT:        
        *((int*)value) = atoi(value_str);
        break;

      case TYPE_DOUBLE:
        *((double*)value) = atof(value_str);
        break;

      case TYPE_STRING:
        *((string*)value) = value_str;
        break;  

    case TYPE_NONE:
    case TYPE_LONG:
    default:
      cerr << __FILE__<<":"<<__LINE__<<"Unknown or unhandled type "<<type<<endl;
  }
}

} // end namespace

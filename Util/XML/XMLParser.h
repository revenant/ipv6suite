// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/XML/Attic/XMLParser.h,v 1.1 2005/02/09 06:15:59 andras Exp $
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
 * @file XMLParser.h
 *
 * @brief An abstract class that provides interface to the concrete sub-class
 * to parse specific set of information in XML file
 *
 * @author Eric Wu
 *
 * @date 10/11/2001
 *
 */

#ifndef XMLPARSER_H
#define XMLPARSER_H

#include <string>
#include <list>
#include <cstdlib>
#include <cstring>
#include <memory> //std::auto_ptr

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp>
#endif


using std::string;


/**
 * @namespace XMLConfiguration
 * XML parsing of network configuration supplied through a file
 */
namespace XMLConfiguration
{

enum DATATYPE
{
  TYPE_NONE   = 0,
  TYPE_BOOL   = 1,
  TYPE_INT    = 2,
  TYPE_LONG   = 3,
  TYPE_DOUBLE = 4,
  TYPE_STRING = 5
};

/**
 * @class NetParam
 * 
 * @brief Try to make parsing the long list of common XML attributes for each
 * interface a breeze
 *
 * @todo Needs a lot of rework to make it universal, safe to use and no
 * memory leaks.  In face the whole of XMLConfiguration needs some thorough
 * revamp of interfaces and implementation.
 */

class NetParam
{
 public:
  NetParam* attrs;     // net parametre attributes
  int numOfAttrs;      // number of net parametre attributes

 public:  
  NetParam()
    : attrs(0), numOfAttrs(0), element(0), type(TYPE_BOOL), tagname("")
    {}

  NetParam(const NetParam& obj)
    {
      NetParam::operator=(obj);      
    }

  NetParam(void* elm, DATATYPE datatype,const string& tag)
    {
      setParam(elm, datatype, tag);      
    }  

  ~NetParam()
    {
      numOfAttrs = 0;      
      if(attrs)
        delete[] attrs;
    }  

  NetParam& operator=(const NetParam& obj)
    {      
      element = obj.element;
      type = obj.type;
      tagname = obj.tagname;      
      attrs = obj.attrs;
      numOfAttrs = obj.numOfAttrs;      
     
      return *this;
    }

  void setParam(const void* elm, DATATYPE datatype, const string& tag)
    {
      element = elm;
      type = datatype;
      tagname = tag;      
    }

  const void* getElement(void)
    {
      return element;
    }
  
  DATATYPE getType(void)
    {
      return type;      
    }

  const string& getTagname(void)
    {
      return tagname;      
    }

 private:
  /// net parametre element
  ///This is the leak however it cannot always be deleted as it does not always
  ///point to dynamic object
  const void* element; 
  DATATYPE type;       ///< net parametre data type
  string tagname;      ///< XML tagname to look for  
};


class XMLDocHandle;

/**
 * @class XMLParser
 * @brief General XML Parser
 * 
 * @Note this class is still not really generic enough to be truly reusable.
 * Perhaps using boost::any is the way.
 */
class XMLParser: boost::noncopyable
{
 public:

  virtual void addEntry(NetParam* parametre)
    {
       paramList.push_back(parametre);
    }

  virtual void initialize(void);  

  const char* filename() const { return _filename.c_str(); }

  void setFile(const string& filename)
    {
      _filename = filename;
    }

  // parse all information in XML to the allocators in the map
  //virtual void parse(void) = 0;

 protected:
  XMLParser(void);

  virtual ~XMLParser(void);

  // a pointer to the XML document
  //std::auto_ptr<DOM_Document> _XMLDoc;
  //std::auto_ptr<DOMParser> parser;
  std::auto_ptr<XMLDocHandle> _XMLDoc;
  
  string _filename;  

  typedef std::list<NetParam*> ParamList;
  ParamList paramList;

 protected:
  void saveInfo(const string& tagname, char* value_str, 
                const void* value, DATATYPE type);
};
}

#endif //XMLPARSER_H

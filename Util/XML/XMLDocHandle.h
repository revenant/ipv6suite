// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/XML/Attic/XMLDocHandle.h,v 1.1 2005/02/09 06:15:59 andras Exp $
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
 * @file XMLDocHandle.h
 * @author Johnny Lai
 * @date 05 Aug 2002
 *
 * @brief Simple Implementation handle class to remove DOM_Document dependency
 * from headers
 *
 */

#include <boost/utility.hpp>

#ifndef XMLDOCHANDLE_H
#define XMLDOCHANDLE_H 1

#ifndef USE_XMLWRAPP
#include <xercesc/dom/DOM.hpp>
#else
#include <xmlwrapp/document.h>
#endif

namespace XMLConfiguration
{

#ifndef USE_XMLWRAPP
/**
 * @class XMLDocHandle
 *
 * @brief Remove dependency of DOM_Document from headers
 *
 * Inspired by Pimpl idiom but not as eloquent
 */

class XMLDocHandle: public boost::noncopyable
{
public:

  //@name constructors, destructors and operators
  //@{
  XMLDocHandle(const DOM_Document& ddoc):_doc(ddoc)
    {}

  //@}

  DOM_Document& doc()
    {
      return _doc;
    }

protected:
  
private:
  DOM_Document _doc;
};
#else  //ifndef USE_XMLWRAPP

/**
 * @class XMLDocHandle
 *
 * @brief Remove dependency of document from headers and preserve existing code. 
 *
 * Hopefully this will be the last XML API change so we can get rid of this
 * mess and use the XML API directly.
 */

class XMLDocHandle: public boost::noncopyable
{
public:

  //@name constructors, destructors and operators
  //@{
  XMLDocHandle(const xml::document& ddoc):_doc(ddoc)
    {}

  //@}

  const xml::document& doc() const
    {
      return _doc;
    }

  xml::document& doc()
    {
      return _doc;
    }

protected:
  
private:
  xml::document _doc;
};
  
#endif //ifndef USE_XMLWRAPP

} //namespace XMLConfiguration


#endif /* XMLDOCHANDLE_H */

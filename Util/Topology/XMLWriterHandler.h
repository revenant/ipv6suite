// -*- C++ -*-
//
// Copyright (C) 2002 Johnny Lai
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

#include <libxml++/libxml++.h>

/**
 * @file XMLWriterHandler.h
 * @author Johnny Lai
 * @date 11 Dec 2002
 *
 * @brief libxml++ SAX Handler to output XML
 *
 */

#ifndef XMLWRITERHANDLER_H
#define XMLWRITERHANDLER_H 1


/**
   @class XMLWriterHandler
   @brief SAX handler to enable simple output of XML.
*/

class XMLWriterHandler: public xmlpp::SaxParser
{
public:

  // Constructor/destructor.
  XMLWriterHandler(const char* fname = "", const char* dotfile = "");
  virtual ~XMLWriterHandler();

  virtual void on_start_document(void);
  virtual void on_end_document(void);
  virtual void on_start_element(const std::string &n, const xmlpp::Element::AttributeMap& pm);
  virtual void on_end_element(const std::string &n);
  virtual void on_characters(const std::string &s);
  virtual void on_warning(const std::string &s);
  virtual void on_error(const std::string &s);
  virtual void on_fatal_error(const std::string &s);

private:
  xmlpp::DomParser tree;
  std::list<xmlpp::Node* > elements;
  std::string filename;
  std::string dotfile;

  // Unused ctor and assignment op.
  XMLWriterHandler(const XMLWriterHandler&);
  XMLWriterHandler& operator=(const XMLWriterHandler&);
};


#endif /* XMLWRITERHANDLER_H */

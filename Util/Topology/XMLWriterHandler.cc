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


/**
 * @file   XMLWriterHandler.cc
 * @author Johnny Lai
 * @date   11 Dec 2002
 *
 * @brief  Implementation of class XMLWriterHandler
 *
 *
 */


#include "XMLWriterHandler.h"

#include "sys.h"
#include "debug.h"

#include <algorithm>

using namespace xmlpp;
using namespace std;


XMLWriterHandler::XMLWriterHandler(const char* fname, const char* dotfile)
  :filename(fname?fname:""), dotfile(dotfile?dotfile:"")
{}

XMLWriterHandler::~XMLWriterHandler()
{}

void XMLWriterHandler::on_start_document(void)
{
//  const std::string encoding("iso-8859-1");
//   if (encoding != tree.set_encoding(encoding))
//   {
//     cerr<<" Problem encountered in setting XML docuemnt to encoding "<<encoding;
//   }

//   Element* root = new Element("netconf");
//   if (!root->initialized())
//   {
//     assert(false);
//   }

  xmlpp::Element* root = dynamic_cast<xmlpp::Element*>(tree.set_root_node("netconf"));
  assert(root);

  root->add_attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
  root->add_attribute("xsi:noNamespaceSchemaLocation", "../../Scripts/netconf.xsd");
  root->add_attribute("debugChannel", "debug.log:notice");
  root->add_attribute("topologyFile", dotfile);

  root->add_content("\n");

  //handler->characters("<!DOCTYPE netconf SYSTEM \"../../Scripts/netconf2.dtd\">");

  elements.push_front(root);
}

void XMLWriterHandler::on_end_document(void)
{
  const std::string encoding("iso-8859-1");
  assert(elements.size() == 1);
  //std::cout<<tree.write_buffer();
  try
  {
    tree.write_to_file(filename, encoding);
  }
  catch(xmlpp::exception &e)
  {
    assert(false);
    cerr<<"Failed to write XML document to file "<<filename<<endl;
    cerr<<"Exception was: "<<e.what()<<endl;
  }
}

struct add_attribute
{
  add_attribute(Element* elem)
    :elem(elem)
    {}

  void operator()(const xmlpp::Element::AttributeMap::value_type& prop)
    {
      elem->add_attribute(prop.second->get_name(),prop.second->get_value());
    }
private:
  Element* elem;
};

void XMLWriterHandler::on_start_element(const std::string &n, const xmlpp::Element::AttributeMap& pm)
{
  Element* elem = dynamic_cast<Element*>((*elements.begin())->add_child(n));
  assert(elem);

  (*elements.begin())->add_content("\n");

  std::for_each(pm.begin(), pm.end(), add_attribute(elem));

  elements.push_front(elem);
}

void XMLWriterHandler::on_end_element(const std::string &n)
{
  elements.pop_front();
}

void XMLWriterHandler::on_characters(const std::string &s)
{
  (*elements.begin())->add_content(s);
}

void XMLWriterHandler::on_warning(const std::string &s)
{
  cerr<<s;
}

void XMLWriterHandler::on_error(const std::string &s)
{
  DoutFatal(dc::core|error_cf, s);
}

void XMLWriterHandler::on_fatal_error(const std::string &s)
{
  DoutFatal(dc::core|error_cf, s);
}

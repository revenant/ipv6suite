// -*- c++ -*-
// Copyright (C) 2006 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

%module datasorter
%{
#include "engine/scalarmanager.h"
#include "engine/datasorter.h"
%}

//%include "/usr/share/swig/1.3.25/ruby/std_vector.i"
%include stl.i
%include std_vector.i
%include std_string.i

//Pretty sad this does not apply at all but only the typedefed pattern does is it bug?
%typemap(out) std::set<std::string>::iterator, std::set<std::string>::const_iterator {
//  $result = SWIG_STR2RB(*$1)
}

%typemap(out) StringRef {
  $result = SWIG_STR2RB(*$1);
}

%typemap(out) RunRef {
  const Run* r = &(*$1);
  $result = SWIG_NewPointerObj((void*) r, $descriptor(Run *), 0);
}

//Will not match on std::list<File> pattern because of typedef? must be bug in
//typedef backtracking for templates
%typemap(out) FileRef, std::list<File>::iterator {
	#require to convert back from iterator
	const File* f = &(*$1);
	$result = SWIG_NewPointerObj((void *) f,
				       $descriptor(File *),0);
}

typedef std::set<std::string> StringSet;
typedef StringSet::const_iterator StringRef;

typedef std::list<Run> RunList;
typedef RunList::const_iterator RunRef;

typedef std::list<File> FileList;
typedef FileList::const_iterator FileRef;


//custom made
%include std_set.i
%include std_list.i


//handle all stl exceptions
%include exception.i
%exception {
  try {
    $action
      } 
  catch (const std::exception& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

%exception {
  try {
    $action
      }
  catch (TException* a) {
    static VALUE cpperror = rb_define_class("CPPError", rb_eStandardError);
    rb_raise(cpperror, a->message());
  }
}

//in swig 1.3.28 can do this instead of above will handle conv automatically
//%exceptionclass TException;


%rename("files") ScalarManager::getFiles() const;
%rename("values") ScalarManager::getValues() const;
%rename("runs") ScalarManager::getRuns() const;
%rename("modules") ScalarManager::getModuleNames() const;
%rename("scalars") ScalarManager::getScalarNames() const;

%rename("run") Datum::runRef;
%rename("module") Datum::moduleNameRef;
%rename("scalar") Datum::scalarNameRef;

%template(Values) std::vector<Datum>;
%template(Strings) std::vector<std::string>;
%template(Ints) std::vector<int>;

%template(Stringset) std::set<std::string>;

%template(Runs) std::list<Run>;
%template(Files) std::list<File>;
%include "engine/scalarmanager.h"

%include "engine/datasorter.h"

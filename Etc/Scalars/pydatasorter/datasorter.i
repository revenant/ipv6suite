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

%include stl.i
%include std_set.i
%include <std_list.i>
//handle pointers in fn args so we can pass normal stuff only for pods though
//%include typemaps.i

//%typemap(in) ScalarManager * {
//if ((SWIG_ConvertPtr($input,(void **) &$1, $1_descriptor, 
//    SWIG_POINTER_EXCEPTION)) == -1)
//  return NULL;
//}

//handle all stl exceptions
%include exception.i
%exception {
  try {
    $action
  } catch (const std::exception& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

//in swig 1.3.28 can do this instead of above will handle conv automatically
%exceptionclass TException;


%rename("files") ScalarManager::getFiles() const;
%rename("values") ScalarManager::getValues() const;
%rename("runs") ScalarManager::getRuns() const;
%rename("modnames") ScalarManager::getModuleNames() const;
%rename("scalars") ScalarManager::getScalarNames() const;

%rename("run") Datum::runRef;
%rename("modname") Datum::moduleNameRef;
%rename("scalar") Datum::scalarNameRef;

%template(Values) std::vector<Datum>;
%template(Strings) std::vector<std::string>;
%template(Ints) std::vector<int>;

%template(StringSet) std::set<std::string>;
%template(Runs) std::list<Run>;
%template(Files) std::list<File>;

%include "scalarmanager.h"
%include "datasorter.h"

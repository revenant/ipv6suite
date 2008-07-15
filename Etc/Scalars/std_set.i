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

%include <std_common.i>
    %include <std_pair.i>

    %{
#include <set>
  %}

namespace std {
  %mixin set "Enumerable";
  template<class T> class set {

    %typemap(out) set<T>, const std::set<T>& {
      $result = rb_hash_new();
      for (std::set<T>::iterator i = $1.begin(); i!=$1.end(); ++i)
	{
	  T* val = new T(i);
	  rb_hash_aset($result, SWIG_NewPointerObj((void*) val, $descriptor(T *), 1),
		       Qnil);
	}
    }
    //Doesn't seem to have effect
    %typemap(out) set<T>::iterator, set<T>::const_iterator, iterator, const_iterator {
      $result = SWIG_NewPointerObj((void*) &(*$1), $descriptor(T *), 1)
	}

    %rename(__len__) size;
    %rename("empty?") empty;
    %rename("delete") __delitem__;
    %rename("has_key?") has_key;    
    public:
	class iterator;
    class reverse_iterator;
    class const_iterator;
    class const_reverse_iterator;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;


    set();
    set(const set<T> &);

    unsigned int size() const;
    bool empty() const;
    void clear();
    pair<set::iterator, bool> insert(const T&);
    %extend {
      const T& __getitem__(const T& key) throw (std::out_of_range) 
	{
	  std::set<T>::iterator i = self->find(key);
	  if (i != self->end())
	    return *i;
	  else
	    throw std::out_of_range("key not found");
	}
      T __delitem__(const T& key) throw (std::out_of_range) {
	std::set<T>::iterator i = self->find(key);
	if (i != self->end()) {
	  T x = *i;
	  self->erase(i);
	  return x;
	} else
	    throw std::out_of_range("key not found");
      }
      bool has_key(const T& key) {
	std::set<T >::iterator i = self->find(key);
	return i != self->end();
      }
      VALUE keys() {
	VALUE keyList = rb_ary_new2(self->size());
	std::set<T >::iterator i;
	unsigned int j;
	for (i=self->begin(), j=0; i!=self->end(); ++i, ++j) {
	  T* ptr = new T(*i);
	  rb_ary_store(keyList,j,
		       SWIG_NewPointerObj((void *) ptr,
					  $descriptor(T *),1));
	}
	return keyList;
      }
      void each() {
	std::set<T >::iterator i;
	for (i=self->begin(); i!=self->end(); ++i) {
	  T* key = new T(*i);
	  VALUE x = SWIG_NewPointerObj((void *) key,
				       $descriptor(T *),0);
	  rb_yield(x);
	}
      }
    }

  };


  %define specialize_std_set_on_key(T,CHECK,CONVERT_FROM,CONVERT_TO) 
     %mixin set<T> "Enumerable";
  template<> class set<T> {
    %typemap(out) set<T> {
      $result = rb_hash_new();
      for (std::set<T>::iterator i = $1.begin(); i!=$1.end(); ++i)
	{
	  T* val = new T(i);
	  rb_hash_aset($result, CONVERT_TO(val),
		       Qnil);
	}
    }
    //no effect
    %typemap(out) iterator, const_iterator, set<T>::iterator, set<T>::const_iterator, std::set<T>::iterator, std::set<T>::const_iterator {
      $result = CONVERT_TO(*$1);
    }
    %rename(__len__) size;
    %rename("empty?") empty;
    %rename("delete") __delitem__;
    %rename("has_key?") has_key;    
    public:
	//	set();
	//    set(const set<T> &);
	class iterator;
    class reverse_iterator;
    class const_iterator;
    class const_reverse_iterator;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;



    unsigned int size() const;
    bool empty() const;
    void clear();
    pair<set<T>::iterator, bool> insert(const T&);
    %extend {
      const T& __getitem__(const T& key) throw (std::out_of_range) 
	{
	  std::set<T>::iterator i = self->find(key);
	  if (i != self->end())
	    return *i;
	  else
	    throw std::out_of_range("key not found");
	}
      T __delitem__(const T& key) throw (std::out_of_range) {
	std::set<T>::iterator i = self->find(key);
	if (i != self->end()) {
	  T x = *i;
	  self->erase(i);
	  return x;
	} else
	    throw std::out_of_range("key not found");
      }
      bool has_key(const T& key) {
	std::set<T >::iterator i = self->find(key);
	return i != self->end();
      }

      VALUE keys() {
	VALUE keyList = rb_ary_new2(self->size());
	std::set<T >::iterator i;
	unsigned int j;
	for (i=self->begin(), j=0; i!=self->end(); ++i, ++j) {
	  //T* ptr = new T(*i);
	  rb_ary_store(keyList,j,
		       CONVERT_TO(*i));
	}
	return keyList;
      }
      void each() {
	std::set<T >::iterator i;
	for (i=self->begin(); i!=self->end(); ++i) {
	  //T* key = new T(*i);
	  VALUE x = CONVERT_TO(*i);
	  rb_yield(x);
	}
      }
    }
  };
  %enddef

     specialize_std_set_on_key(bool,SWIG_BOOL_P,
			       SWIG_RB2BOOL,SWIG_BOOL2RB);
  specialize_std_set_on_key(int,FIXNUM_P,
			    FIX2INT,INT2NUM);
  specialize_std_set_on_key(short,FIXNUM_P,
			    FIX2INT,INT2NUM);
  specialize_std_set_on_key(long,FIXNUM_P,
			    FIX2INT,INT2NUM);
  specialize_std_set_on_key(unsigned int,FIXNUM_P,
			    FIX2INT,INT2NUM);
  specialize_std_set_on_key(unsigned short,FIXNUM_P,
			    FIX2INT,INT2NUM);
  specialize_std_set_on_key(unsigned long,FIXNUM_P,
			    FIX2INT,INT2NUM);
  specialize_std_set_on_key(double,SWIG_FLOAT_P,
			    SWIG_NUM2DBL,rb_float_new);
  specialize_std_set_on_key(float,SWIG_FLOAT_P,
			    SWIG_NUM2DBL,rb_float_new);
  specialize_std_set_on_key(std::string,SWIG_STRING_P,
			    SWIG_RB2STR,SWIG_STR2RB);

};

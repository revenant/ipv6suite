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

%{
#include <list>
#include <algorithm>
#include <stdexcept>
%}

//Cannot enable functions that return any iterator because it generates a
//typemap for T=File anyway that assumes it is a std::string. No idea why though

namespace std {
  %mixin list "Enumerable";
  
  template<class T> class list {

    %typemap(out) list<T> {
      $result = rb_ary_new2($1.size());
      for (std::list<T>::iterator it = $1.begin(), unsigned int i = 0;it != $1.end(); it++, i++) {
	T* x = new T(*it);
	rb_ary_store($result,i,
		     SWIG_NewPointerObj((void *) x, 
					$descriptor(T *), 1));
      }
    }   
    public:
	%rename(__len__) size;
    %rename("empty?") empty;
    %rename(push) push_back;
    // Types


    typedef T value_type;
    typedef size_t size_type;
    typedef T& reference;
    typedef const T &  const_reference;
   
    // Construct/copy/destroy

    explicit list();

    explicit list(size_type n, const T& value = T(  ));

    list(const list<T>& x);

    ~list(  );

    // Iterators

    class iterator;
    class reverse_iterator;
    class const_iterator;
    class const_reverse_iterator;
    /*
      iterator begin();
      const_iterator begin() const;
      iterator end();
      const_iterator end() const;
      reverse_iterator rbegin();
      const_reverse_iterator rbegin() const;
      reverse_iterator rend();
      const_reverse_iterator rend() const;
    */
    // Capacity

    bool empty(  ) const;

    size_type size(  ) const;

    size_type max_size(  ) const;

    void resize(size_type sz, T c = T(  ));

    // Element access

    reference front(  );

    const_reference front(  ) const;

    reference back(  );

    const_reference back(  ) const;

    // Modifiers

    void push_front(const T& x);

    void pop_front(  );

    void push_back(const T& x);

    void pop_back(  );

    //  iterator insert(iterator position, const T& x);

    void insert(iterator position, size_type n, const T& x);

    template <class InputIterator>

	void insert(iterator position, InputIterator first, InputIterator last);

    /*
      iterator erase(iterator position);

      iterator erase(iterator position, iterator last);
    */

    void clear(  );

    // List operations
    /*
      void splice(iterator position, list<T,Alloc>& x);

      void splice(iterator position, list<T,Alloc>& x, iterator i);

      void splice(iterator position, list<T,Alloc>& x, iterator first, 

      iterator last);

      void remove(const T& value);

      template <class Predicate>

      void remove_if(Predicate pred);

      void unique(  );

      template <class BinaryPredicate>

  void unique(BinaryPredicate binary_pred);

  void merge(list<T,Alloc>& x);

  template <class Compare>

  void merge(list<T,Alloc>& x, Compare comp);

  void sort(  );
    */
  template <class Compare> void sort(Compare comp);

  void reverse(  );

    %extend {
    T pop() throw (std::out_of_range) {
       if (self->size() == 0)
	 throw std::out_of_range("pop from empty list");
       T x = self->back();
       self->pop_back();
       return x;
    }
    void each() {
      for (std::list<T>::iterator i = self->begin();i != self->end(); i++) {
	T* x = &(*i);
	rb_yield(SWIG_NewPointerObj((void*) x, $descriptor(T *), 0));
      }
    }

    }
};


  %define specialize_std_list(T,CHECK,CONVERT_FROM,CONVERT_TO)
     %mixin list "Enumerable";
  template<> class list<T> {
    %typemap(out) list<T> {
      $result = rb_ary_new2($1.size());
      for (std::list<T>::iterator it = $1.begin(), unsigned int i = 0;it != $1.end(); it++, ++i) {
	rb_ary_store($result,i,
		     CONVERT_TO(*it));
      }
    }   
    public:
	%rename(__len__) size;
    %rename("empty?") empty;
    %rename(push) push_back;
    // Types


    typedef T value_type;


   
    // Construct/copy/destroy

    explicit list();

    explicit list(size_type n, const T& value = T(  ));

    list(const list<T>& x);

    ~list(  );

    // Iterators

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

    // Capacity

    bool empty(  ) const;

    size_type size(  ) const;

    size_type max_size(  ) const;

    void resize(size_type sz, T c = T(  ));

    // Element access

    reference front(  );

    const_reference front(  ) const;

    reference back(  );

    const_reference back(  ) const;

    // Modifiers

    void push_front(const T& x);

    void pop_front(  );

    void push_back(const T& x);

    void pop_back(  );

    iterator insert(iterator position, const T& x);

    void insert(iterator position, size_type n, const T& x);

    template <class InputIterator>

	void insert(iterator position, InputIterator first, InputIterator last);

    iterator erase(iterator position);

    iterator erase(iterator position, iterator last);


    void clear(  );

    // List operations

    void splice(iterator position, list<T,Alloc>& x);

    void splice(iterator position, list<T,Alloc>& x, iterator i);

    void splice(iterator position, list<T,Alloc>& x, iterator first, 

		iterator last);

    void remove(const T& value);

    template <class Predicate>

	void remove_if(Predicate pred);

    void unique(  );

    template <class BinaryPredicate>

	void unique(BinaryPredicate binary_pred);

    void merge(list<T,Alloc>& x);

    template <class Compare>

	void merge(list<T,Alloc>& x, Compare comp);

    void sort(  );

    template <class Compare> void sort(Compare comp);

    void reverse(  );

    %extend {
      T pop() throw (std::out_of_range) {
	if (self->size() == 0)
	  throw std::out_of_range("pop from empty list");
	T x = self->back();
	self->pop_back();
	return x;
      }
      void each() {
	for (std::list<T>::iterator i = self->begin();i != self->end(); i++) {
	  T* x = &(*i);
	  rb_yield(CONVERT_TO(*i));
	}
      }

    }
  };
  %enddef

     specialize_std_list(bool,SWIG_BOOL_P,SWIG_RB2BOOL,SWIG_BOOL2RB);
  specialize_std_list(char,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(int,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(short,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(long,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(unsigned char,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(unsigned int,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(unsigned short,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(unsigned long,FIXNUM_P,FIX2INT,INT2NUM);
  specialize_std_list(double,SWIG_FLOAT_P,SWIG_NUM2DBL,rb_float_new);
  specialize_std_list(std::string,SWIG_STRING_P,SWIG_RB2STR,SWIG_STR2RB);
  specialize_std_list(float,SWIG_FLOAT_P,SWIG_NUM2DBL,rb_float_new);


};

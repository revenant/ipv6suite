// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Util/Attic/cTypedArrays.h,v 1.1 2005/02/09 06:15:58 andras Exp $
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
	@file cTypedArrays.h
	@brief Typesafe carray classes.
	@author Johnny Lai
	@date 19.9.01
	@test see cTypedContainerTest

*/


#ifndef CTYPEDARRAY_H
#define CTYPEDARRAY_H
#ifndef __CARRAY_H
#include "carray.h"
#endif //__CARRAY_H
#ifndef STRING
#define STRING
#include <string>
#endif //STRING
#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR
#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif //CASSERT


using std::vector;
using std::stringstream;
using std::string;


/**
   @class cTypedArray

   @brief Typesafe carray of homogenous elements.  Removes the need for those
   unsightly typecasts.
*/
template <class T>
class cTypedArray:public cArray
{
public:

  /** @name Constructors, destructor, assignment. */
  //@{
  cTypedArray(const cTypedArray& list)
    {
      operator=(list);      
    }
  
  explicit cTypedArray(const char* name = NULL, int size=0, int dt = 10):
    cArray(name, size, dt)
    {}

  /**
     Destructor. The contained objects that were owned by the container
     will be deleted.
  */
  virtual ~cTypedArray(){};
  /**
     Assignment operator. The name member doesn't get copied;
     see cObject's operator=() for more details.
     Duplication and assignment work all right with cArray.
     Contained objects that are owned by cArray will be duplicated
     so that the new cArray will have its own copy of them.
  */
  cTypedArray& operator=(const cTypedArray& list)
    {
      cArray::operator=(list);
      return *this;      
    }
  
  //@}

  /** @name Redefined cObject member functions */
  //@{

  /**
   * Returns pointer to a string containing the class name, "cArray".
   */
  virtual const char *className() const
    {
      static string buf;        
      buf = "cTypedArray<";
      for (int i = 0; i < items(); i++)
      {  
        if (exist(i))
        {  
          buf.append(get(i)->className());
          break;
        }
      }
      buf += ">";      
      return buf.c_str();
    }

  /**
   * Duplication and assignment work all right with cArray.
   * Contained objects that are owned by cArray will be duplicated
   * so that the new cArray will have its own copy of them.
   */
  virtual cTypedArray* dup() const  {return new cTypedArray(*this);}
  //@}


  /** @name Container functions. */
  //@{
  /**
   * Inserts a new object into the array. Only the pointer of the object
   * will be stored. The return value is the object's index in the
   * array.
   */
  int add(T* obj) { return cArray::add(obj); }
  
  /**
   * Inserts a new object into the array, at the given position. If
   * the position is occupied, the function generates an error message.
   */
  int addAt(int m,T* obj) { return cArray::addAt(m, obj); }
  
  /**
   * Searches the array for the pointer of the object passed and returns
   * the index of the first match. If the object wasn't found, -1 is
   * returned.
   */
  int find(T* obj) const { return cArray::find(obj); }
  
  /**
   * Returns reference to the mth object in the array. Returns NULL
   * if the mth position is not used.
   */
  T* get(int m)
    {
      return static_cast<T*> (cArray::get(m));
    }
  /**
   * Returns reference to the mth object in the array. Returns NULL
   * if the mth position is not used.
   */
  const T* get(int m) const
    {
      return static_cast<const T*> (cArray::get(m));
    }
  

  /**
   * The same as get(int). With the indexing operator,
   * cArray can be used as a vector.
   */
  T* operator[](int m) {return get(m);}
    
  /**
   * The same as get(int). With the indexing operator,
   * cArray can be used as a vector.
   */
  const T* operator[](int m) const {return get(m);}

  /**
   * Removes the object given with its index/name/pointer from the
   * container. (If the object was owned by the container, drop()
   * is called.)
   */
  T* remove(int m) 
    {
      return static_cast<T*> (cArray::remove(m));
    }
  
  /**
   * Removes the object given with its index/name/pointer from the
   * container. (If the object was owned by the container, drop()
   * is called.)
   */
  T* remove(T* obj)
    {
      return static_cast<T*> (cArray::remove(obj));
    }

  /**
     Use this to improve compatibility with STL instead of items()
  */
  size_t size() const
    {
      return static_cast<size_t> (items());      
    }

  /**
     Use this to improve compatibility with STL
  */
  bool empty() const
    {
      return size() == 0?true:false;
    }
  
  //@} 

};

/**
   @class cTypedVector

   @brief Typesafe carray with vector operations i.e. return a reference
   to the object rather than a pointer.  Removes need for typecasts.
   Makes it easier to change code from container of cObjects to STL
   containers.  

   The add operations have not been implemented with the correct
   semantics yet (Instead of copying the object it treats the object like
   a normal cArray). 
*/
template <class T>
class cTypedVector:public cArray
{
public:

  /** @name Constructors, destructor, assignment. */
  //@{
  cTypedVector(const cTypedVector& list)
    :cArray(list)
    {}
  
  explicit cTypedVector(const char* name = NULL, int size=0, int dt = 10):
    cArray(name, size, dt)
    {}

  /**
     Destructor. The contained objects that were owned by the container
     will be deleted.
  */
  virtual ~cTypedVector(){};
  /**
     Assignment operator. The name member doesn't get copied;
     see cObject's operator=() for more details.
     Duplication and assignment work all right with cArray.
     Contained objects that are owned by cArray will be duplicated
     so that the new cArray will have its own copy of them.
  */
  cTypedVector& operator=(const cTypedVector& list)
    {
      cArray::operator=(list);
      return *this;      
    }
  
  //@}

  /// @name Redefined cObject member functions 
  //@{

  /**
   * Returns pointer to a string containing the class name, "cArray".
   */
  virtual const char *className() const
    {
      static string buf;
      buf = "cTypedVector<";
      //Changed because calling get(0) will cause an endless recursion when
      //get(0) == 0 (cArray calls className to display warning - can happen when
      //things are removed from array)
      for (int i = 0; i < items(); i++)
      {  
        if (exist(i))
        {  
          buf.append(get(i)->className());
          break;
        }
      }
      buf += ">";      
      return buf.c_str();
    }

  /**
   * Duplication and assignment work all right with cArray.
   * Contained objects that are owned by cArray will be duplicated
   * so that the new cArray will have its own copy of them.
   */
  virtual cTypedVector* dup() const  {return new cTypedVector(*this);}
  //@} 
  /** @name Container functions. */
  //@{
  /**
   * Inserts a new object into the array. Only the pointer of the object
   * will be stored. The return value is the object's index in the
   * array.
   */
  int add(T* obj) { return cArray::add(obj); }
  
  /**
   * Inserts a new object into the array, at the given position. If
   * the position is occupied, the function generates an error message.
   */
  int addAt(int m,T* obj) { return cArray::addAt(m, obj); }
  
  /**
   * Searches the array for the pointer of the object passed and returns
   * the index of the first match. If the object wasn't found, -1 is
   * returned.
   */
  int find(T* obj) const { return cArray::find(obj); }
  
  /**
   * Returns reference to the mth object in the array. Returns NULL
   * if the mth position is not used.
   */
  T* get(int m)
    {
      return static_cast<T*> (cArray::get(m));
    }
  /**
   * Returns reference to the mth object in the array. Returns NULL
   * if the mth position is not used.
   */
  const T* get(int m) const
    {
      return static_cast<const T*> (cArray::get(m));
    }
  

  /**
   * The same as get(int). With the indexing operator,
   * cArray can be used as a vector.
   */
  T& operator[](int m) { return *get(m); }  
    
  /**
   * The same as get(int). With the indexing operator,
   * cArray can be used as a vector.
   */
  const T& operator[](int m) const {return *get(m);}

  /**
   * Removes the object given with its index/name/pointer from the
   * container. (If the object was owned by the container, drop()
   * is called.)
   */
  T* remove(int m) 
    {
      return static_cast<T*> (cArray::remove(m));
    }
    
  /**
   * Removes the object given with its index/name/pointer from the
   * container. (If the object was owned by the container, drop()
   * is called.)
   */
  T* remove(T* obj)
    {
      return static_cast<T*> (cArray::remove(obj));
    }

  /**
     Use this to improve compatibility with STL instead of items()
  */  
  size_t size() const
    {
      return static_cast<size_t> (items());      
    }

  /**
     Use this to improve compatibility with STL
  */
  bool empty() const
    {
      return size() == 0?true:false;
    }

/**
   @brief Fill the "holes" in cArray by rearranging existing elements

   @param swapArray vector which will temporarily hold the pointers whilst this
   container is resizing itself to "fill" the holes. It can be reused many
   times to conserve the cost of dynamic memory allocation. i.e. empty() but
   capacity() is > 0.

   @invariant Relative ordering of elements is preserved
   - Precond:	swapArray is empty and "holes" exist
   - Postcond: swapArray is empty and no "holes"	

   This is a hack but if we want a cObject container without holes this is the
   only efficient way.
*/

  void removeHoles(vector<T*> &swapArray)
    {
      assert(swapArray.empty());
      
      for (size_t i = 0; i < size(); i++)
      {
        if (!exist(i))
          continue;
        T* a = remove(i);
        swapArray.push_back(a);
      }
     
      clear();
      
      for (size_t i = 0; i < swapArray.size(); i++)
      {
        add(swapArray[i]);
      }
      swapArray.clear();
    }
  
  //@}  

};

#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

#ifndef SSTREAM
#define SSTREAM
#include <sstream>
#endif //SSTREAM
#ifndef IPV6ADDRESS_H
#include "IPv6Address.h"
#endif //IPV6ADDRESS_H

/**
 * @class cTypedContainerTest
 * @brief Unit test for cTypedArray and cTypedVector
 * @ingroup TestCases
 */

class cTypedContainerTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( cTypedContainerTest );
  CPPUNIT_TEST( testcTA );
  CPPUNIT_TEST( testcTV );
  CPPUNIT_TEST_SUITE_END();
public:
                        
  void testcTA()
    {
      //Preallocate a 10 element container
      cta_a = new cTypedArray<IPv6Address> ("ctaObj", 10);
      CPPUNIT_ASSERT(cta_a->size() == 0);
      CPPUNIT_ASSERT(cta_a->add(new IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/0")) == 0);
      //Add at position 1 automatically
      CPPUNIT_ASSERT(cta_a->addAt(1, new IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/32")) ==
             1);
      CPPUNIT_ASSERT(cta_a->size() == 2);
      CPPUNIT_ASSERT(*(*cta_a)[0]==IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/0"));
      CPPUNIT_ASSERT(*(*cta_a)[1] == IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/32"));
      
      
      cTypedArray<IPv6Address> cta_b(*cta_a);
      CPPUNIT_ASSERT(cta_b.size() == 2);

      ipv6_addr addr1 = {0x12345678,0xabcdef00,0x1234,0};
      cta_b.addAt(2, new IPv6Address(addr1));
      CPPUNIT_ASSERT(cta_b.size() == 3 && cta_a->size() == 2);

      delete cta_a;

      CPPUNIT_ASSERT(cta_b.size() == 3);

#if defined UNITTESTOUTPUT
      ev<<cta_b.className();
#endif
      stringstream ss;
      cta_b.writeContents(ss);
#if defined UNITTESTOUTPUT
      ev<<ss.str().c_str();
#endif

    }

  void testcTV()
    {
      //Preallocate a 10 element container
      ctv_a = new cTypedVector<IPv6Address> ("ctVObj", 10);
      CPPUNIT_ASSERT(ctv_a->size() == 0);
      CPPUNIT_ASSERT(ctv_a->add(new IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/10")) == 0);
      //Add at position 1 automatically
      CPPUNIT_ASSERT(ctv_a->addAt(1, new IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/0")) ==
             1);
      CPPUNIT_ASSERT(ctv_a->size() == 2);
      
      cTypedVector<IPv6Address>* ctv_b = new cTypedVector<IPv6Address>(*ctv_a);
      CPPUNIT_ASSERT(ctv_b->size() == 2);

      ipv6_addr addr1 = {0x12345678,0xabcdef00,0x1234,0};
      ctv_b->addAt(2, new IPv6Address(addr1));
      CPPUNIT_ASSERT(ctv_b->size() == 3 && ctv_a->size() == 2);

      delete ctv_a;
      
      ctv_a = 0;
      
      CPPUNIT_ASSERT(ctv_b->size() == 3);
#if defined UNITTESTOUTPUT      
      ev<<ctv_b->className();

      ev<<ctv_b->className();
#endif
      stringstream ss;
      ctv_b->writeContents(ss);
#if defined UNITTESTOUTPUT
      ev<<ss.str().c_str();
#endif
      ipv6_addr addr2 = {0x1,0x2,0x3,4};
      ctv_b->add(new IPv6Address(addr2));
      
      //Remove 3rd element i.e. addr1
      ctv_b->remove(2);
      CPPUNIT_ASSERT(ctv_b->size() == 4);
      
      vector<IPv6Address*> swapArr;
      ctv_b->removeHoles(swapArr);
      CPPUNIT_ASSERT(ctv_b->size() == 3);

#if defined UNITTESTOUTPUT      
      //Test relative ordering
      cout << (*ctv_b)[0] 
           << "should be equivalent to (scope is determined at equality test)"
           <<IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/10")<<endl;
#endif // UNITTESTOUTPUT 
    
      CPPUNIT_ASSERT((*ctv_b)[0] == IPv6Address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd/10"));
      
      CPPUNIT_ASSERT((*ctv_b)[1] == IPv6Address("ef00:abcd:ef00:ffff:0:0f0f:0:0/0"));
      
      CPPUNIT_ASSERT((*ctv_b)[2] == addr2);
      
      delete ctv_b;
    }

private:
  cTypedArray<IPv6Address>* cta_a;
  cTypedVector<IPv6Address>* ctv_a;  
  
};

#endif // USE_CPPUNIT
#endif //CTYPEDARRAY_H

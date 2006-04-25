// -*- C++ -*-
// Copyright (C) 2005, 2006 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file ExpiryEntryListSignal.h
 * @author Johnny Lai
 * @date 07 Aug 2005
 *
 * @brief Definition of class ExpiryEntryListSignal
 *
 */

#ifndef EXPIRYENTRYLISTSIGNAL_H
#define EXPIRYENTRYLISTSIGNAL_H

#ifndef BOOST_MPL_IF_HPP_INCLUDED
#include <boost/mpl/if.hpp>
#endif

#ifndef BOOST_CALL_TRAITS_HPP
#include <boost/call_traits.hpp>
#endif

#ifndef BOOST_TT_REMOVE_POINTER_HPP_INCLUDED
#include <boost/type_traits/remove_pointer.hpp>
#endif

#ifndef BOOST_BIND_HPP_INCLUDED
#include <boost/bind.hpp>
#endif

#ifndef CCALLBACKMESSAGE_H
#include "cCallbackMessage.h"
#endif


/**
   @struct greaterExpiryTime
   @brief  Default Function object for heap comparison in ExpiryEntryListSignal

   @ingroup Timer Framework
*/
template<typename value_type>
struct greaterExpiryTime : public std::binary_function<value_type, value_type, bool>
{

  typedef typename boost::call_traits<value_type>::param_type param_type;

  ///Overloaded to handle pointer/non-pointer Entry types. Only one version
  ///will evaluate to the comparison function signature.
  bool operator()(typename boost::mpl::if_c< boost::is_pointer<value_type>::value, void*, param_type>::type lhs, param_type rhs) const
  {
    return (lhs.expiryTime() > rhs.expiryTime());
  }
  bool operator()(param_type lhs, typename boost::mpl::if_c< boost::is_pointer<value_type>::value, param_type, void*>::type rhs) const
  {
    return (lhs->expiryTime() > rhs->expiryTime());
  }
};

/**
 * @class ExpiryEntryListSignal
 *
 * @brief Manage entries that expire with a single timer
 *
 * @arg Container is a std sequence container and Container::value will have to
 * provide expiryTime() function that returns a simtime_t value denoting either
 * a relative time from simTime() or absolute time when element has expired
 *
 * @note Comp was a comparison functor to sort the entries in chronological
 * order. Now a custom Comp can be specified at call time. The default
 * greaterExpiryTime will sort correctly for a heap and assumes
 * Container::value_type::expiryTime is defined.
 *
 * @ingroup Timer Framework
 */

template<typename Container>
class ExpiryEntryListSignal:
  public Container,
  public boost::function<
  void (typename boost::call_traits<typename Container::value_type>::param_type)>
{
  //Was not able to use this otherwise value_type function
  //name can be anything besides expiryTime and we could invoke it
  typedef boost::function<simtime_t ()> ExpireCB;
public:
  //allow only list/vector/deque i.e. seq containers. Can we enforce this?
  typedef typename Container::value_type value_type;
  typedef typename boost::call_traits<value_type>::param_type param_type;
  typedef boost::function<void (param_type)> selfcb;
  typedef struct greaterExpiryTime<typename Container::value_type> Comp;

  using Container::begin;
  using Container::end;
  using Container::front;
  using Container::back;
  using Container::empty;
  using Container::pop_back;

  //@name constructors, destructors and operators
  //@{

  ExpiryEntryListSignal(bool relative = false):
      sig(0), _relative(relative)
  {}

  ~ExpiryEntryListSignal()
  {
    clear();
    delete sig;
  }

  ///use Nullary's operator= to rebind new callback or args
  ExpiryEntryListSignal& operator=(const selfcb& f)
  {
    (selfcb&)(*this)=f;
    return *this;
  }

  //@}

  ///Called only once to start the timer internally for a collection of heap objects
  ///with expiryTime
  void startTimer()
  {
    assert(!sig);
    //contextModule is not ready yet when list default initialised in
    //cSimpleModule as a data member
    sig = new cCallbackMessage("expiryEntryList");
    (*sig) = boost::bind(&ExpiryEntryListSignal::removeExpiredEntry, this);   
  }

  
/**
 * @brief Resorts the container and resets timer
 * 
 * @note Required to be called when the entry's expiryTime has changed. While it is
 *possible to make the setExpiredEntry call this automatically this requires
 *an extra pointer to this object in every entry
 *
 */
  void sortAndReset()
  {
    make_heap(begin(), end(), Comp());
    if (sig->isScheduled())
      sig->cancel();
    reschedule();
  }

  template <typename Comparator> void sortAndReset(const Comparator& c)
  {
    make_heap(begin(), end(), c());
    if (sig->isScheduled())
      sig->cancel();
    reschedule();
  }

  void remove(param_type p)
  {
/* 
//Don't ever do this (similar to ExpiryEntryList) because in client code we call
//external callback (if we do then double deletion occurs since this functions
//invokes on external callback which is different from ExpiryEntryList
//behaviour!)

    if (front() == p)
    {
      removeExpiredEntry();
    }
    else
*/
    erase(find(begin(), end(), p));
  }

  /// Add/Update an entry into the list
  void addOrUpdate(param_type p)
  {
    // Remove any existing entry from the list 
    Container::erase(std::find(begin(), end(), p), end());
    push_back(p);
  }

  //@name redefined std Sequence Container functions
  //@{

  void clear()
  {
    Container::clear();
    if (sig->isScheduled())
      sig->cancel();    
  }

  void push_back(param_type x)
  {
    Container::push_back(x);
    sortAndReset();
  }

  typedef typename Container::iterator iterator;
  iterator erase(iterator p)
  {
    if (p == end())
      return end();

    Container::erase(p);
    sortAndReset();
    return begin();
  }

  iterator
  erase(iterator first, iterator last)
  {
    Container::erase(first, last);
    sortAndReset();
    return begin();
  }

  //@}

  void setRelative(bool relative = true)
  {
    _relative = relative;
  }

  template <class E> friend std::ostream& operator<< (std::ostream&, const ExpiryEntryListSignal<E> &);

private:

  void reschedule()
  {
    typedef typename boost::mpl::if_c< boost::is_pointer<value_type>::value,
      typename boost::remove_pointer<value_type>::type,
      value_type>::type realtype;
    simtime_t t = boost::bind(&realtype::expiryTime, _1)(front());

    if (_relative)
      sig->rescheduleDelay(t);
    else
      sig->reschedule(t);
  }

  /**
   *@name timer callback function to remove expired entry
   *
   * @todo create a template version like sortAndReset and also for startTimer
   * if we want custom comparison
   */
  //@{
  void removeExpiredEntry()
  {
    assert(!empty());
      
    pop_heap(begin(), end(), Comp());
      
    ///invoke callback assigned externally
    (*this)(back());
    pop_back();
    if (!empty())
    {
      assert(sig && !sig->isScheduled());
      if (sig->isScheduled())
        sig->cancel();
      reschedule();
    }
  }
  //@}

  cCallbackMessage* sig;
  bool _relative;
}; 


/**
   @struct HelpMePrint
   @brief Helps to print out value_type  that may be pointers
   @ingroup Timer Framework
*/

  template<typename value_type>
    struct HelpMePrint : public std::unary_function<value_type, void>
{
  HelpMePrint(std::ostream& os):os(os){}

  typedef typename boost::call_traits<value_type>::param_type param_type;

  void operator()(typename boost::mpl::if_c< boost::is_pointer<value_type>::value, void*, param_type>::type lhs) const
  {
    os<<lhs;
  }
  void operator()(typename boost::mpl::if_c< boost::is_pointer<value_type>::value, param_type, void*>::type rhs) const
  {
    os<<*rhs;
  }
private:
  std::ostream& os;
};

/**
   @brief output operator for ExpiryEntryListSignal<Entry>	
   @ingroup Timer Framework
*/

template <class Entry>
std::ostream& operator<<(std::ostream& os, const ExpiryEntryListSignal<Entry>& list)
{  
  typedef ExpiryEntryListSignal<Entry> const EEL;
  typedef typename EEL::value_type val;
  typedef typename boost::mpl::if_c< boost::is_pointer<val>::value,
    typename boost::remove_pointer<val>::type,
    val>::type realtype;
  os<<typeid(realtype).name()<<"\n";
  typedef typename EEL::const_iterator ELCI;

  HelpMePrint<val> helpMePrint(os);
  typedef typename boost::call_traits<realtype>::param_type param_type;
  for (ELCI cit = list.begin(); cit != list.end(); ++cit)
  {
    helpMePrint(*cit);
//  Too bad not all types have member operator<<    
//  boost::bind(&realtype::operator<<, *cit)(os);

    os<<" expires at "<<
      boost::bind(&realtype::expiryTime, _1)(*cit)
      <<std::endl;
  }
  return os;
}


#endif /* EXPIRYENTRYLISTSIGNAL_H */

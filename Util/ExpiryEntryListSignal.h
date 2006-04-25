// -*- C++ -*-
// Copyright (C) 2005 Johnny Lai
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

#include <boost/call_traits.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/if.hpp>

#ifndef CSIGNALMESSAGE_H
#include "cSignalMessage.h"
#endif //CSIGNALMESSAGE_H

namespace mpl = boost::mpl;

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
  bool operator()(typename mpl::if_c< boost::is_pointer<value_type>::value, void*, param_type>::type lhs, param_type rhs) const
  {
    return (lhs.expiryTime() > rhs.expiryTime());
  }
  bool operator()(param_type lhs, typename mpl::if_c< boost::is_pointer<value_type>::value, param_type, void*>::type rhs) const
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
 * @arg Comp is a comparison functor to sort the entries in chronological
 * order. The default greaterExpiryTime will sort correctly for a heap and
 * assumes Container::value_type::expiryTime is defined.
 *
 * @ingroup Timer Framework
 */

//  template<typename Container, typename Signature = void (*)(typename Container::value_type), typename Comp = greaterExpiryTime >
template<typename Container,
         typename Comp = greaterExpiryTime<typename Container::value_type>,
         //Should not change to anything else really perhaps remove this template parameter?
         typename Signature = void (typename boost::call_traits<typename Container::value_type>::param_type)
  >
class INET_API ExpiryEntryListSignal: public Container, public boost::signal<Signature>

{
public:
  //allow only list/vector/deque i.e. seq containers. Can we enforce this?
  typedef typename Container::value_type value_type;
  typedef typename boost::call_traits<value_type>::param_type param_type;

  using Container::begin;
  using Container::end;
  using Container::front;
  using Container::back;
  using Container::empty;
  using Container::pop_back;

  //@name constructors, destructors and operators
  //@{

  ExpiryEntryListSignal(bool relative = false):_relative(relative), sig(0)
  {
  }

  ~ExpiryEntryListSignal()
  {
//    delete sig;
    sig = 0;
  }

  ///Called only once to start the timer internally for a collection of heap objects
  ///with expiryTime
  void startTimer()
  {
    assert(!sig);
    //contextModule is not ready yet when list default initialised in
    //cSimpleModule as a data member
    sig = new cSignalMessage<void ()> ("expiryEntryList");
    sig->connect(boost::bind(&ExpiryEntryListSignal::removeExpiredEntry, this));
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
  //@}

  //@name redefined std Sequence Container functions
  //@{

  void push_back(param_type x)
  {
    Container::push_back(x);
    sortAndReset();
  }

  typedef typename Container::iterator iterator;
  iterator erase(iterator p)
  {
    if (p == end())
      return;

    Container::erase(p);
    sortAndReset();
  }

  iterator
  erase(iterator first, iterator last)
  {
    Container::erase(first, last);
    sortAndReset();
  }

  //@}

  void setRelative(bool relative = true)
  {
    _relative = relative;
  }

private:

  void reschedule()
  {
    simtime_t t = expiryTime(front(), boost::mpl::int_<boost::is_pointer<value_type>::value >() );
      if (_relative)
      sig->rescheduleDelay(t);
      else
      sig->reschedule(t);
  }

  /**
   *@name timer callback functions
   */
  //@{
  void removeExpiredEntry()
  {
    assert(!empty());
      
      pop_heap(begin(), end(), Comp());
      
      (*this)(back());
      pop_back();
      if (!empty())
    {
      assert(!sig->isScheduled());
      reschedule();
    }
  }
  //@}

  ///overloaded expiryTime to handle non-pointer Entry types
  simtime_t expiryTime(param_type e, boost::mpl::int_<false>) const
  {
    return e.expiryTime();
  }

  ///overloaded expiryTime to handle pointer Entry types    
  simtime_t expiryTime(param_type e, boost::mpl::int_<true>) const
  {
    return e->expiryTime();
  }


  bool _relative;
  //timer callback
  cSignalMessage<void ()>* sig;
};


#endif /* EXPIRYENTRYLISTSIGNAL_H */


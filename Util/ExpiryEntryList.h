// -*- C++ -*-
// Copyright (C) 2003, 2004 Monash University, Melbourne, Australia
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
 * @file ExpiryEntryList.h
 * @author Steve Woon, Johnny Lai
 * @date 29 Aug 2003
 *
 * @brief Definition of ExpiryEntryList
 *
 *
 * 
 */


#ifndef EXPIRYENTRYLIST_H
#define EXPIRYENTRYLIST_H

#ifndef CTIMERMESSAGECB_H
#include "cTimerMessageCB.h"
#endif //CTIMERMESSAGECB_H

#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR

#ifndef ALGORITHM
#define ALGORITHM
#include <algorithm>
#endif //ALGORITHM

#ifndef FUNCTIONAL
#define FUNCTIONAL
#include <functional>
#endif //FUNCTIONAL

#ifndef IOSFWD
#define IOSFWD
#include <iosfwd>
#endif //IOSFWD

const int TMR_ENTRYEXPIRED = 5001;

/**
   @class ExpiryEntryList
   @brief  A generic list to store and expire entries

   Typically used for Conceptual Data Structure entries where lifetimes have to
   be managed.  Things need to be done during different phases of their
   lifetime. Usually its removal but in some cases it can get rather complex
   like address timers that need refreshing halfway through their lifetime and
   and if that fails removal once lifetime expires. Once the most recent entry
   has expired will schedule the next most recent to expire will be scheduled.

   A partial specialization of this class for pointer Entry types would have
   achieved the same effect as the use of Loki::Select,Loki::TypeTraits and
   Loki::Int2Type but more code duplication.

   @note Template argument Entry needs to have operator==() and expiryTime()
   defined and identifier() if printList() is used.  Timer is a template
   argument for the timer message object and is owned by this object. The
   default template argument for Timer is valid only for the callback function
   removeExpiredEntry and thus the ctor that does not accept Timer as argument.

   @ingroup Timer Framework
*/


template <class Entry, class Timer = Loki::cTimerMessageCB<
//Return type of callback
typename  Loki::Select< Loki::TypeTraits<Entry>::isPointer, Entry, void>::Result,
//Arguments of callback
typename  Loki::Select< Loki::TypeTraits<Entry>::isPointer, TYPELIST_1(Loki::Int2Type<true>), TYPELIST_1(Loki::Int2Type<false>) >::Result
> >
class ExpiryEntryList
{
  typedef typename Loki::Select< Loki::TypeTraits<Entry>::isPointer, Entry, void>::Result ReturnType;
  typedef typename  Loki::Select< Loki::TypeTraits<Entry>::isPointer, TYPELIST_1(Loki::Int2Type<true>), TYPELIST_1(Loki::Int2Type<false>) >::Result ArgType;
  public:
  typedef Entry ElementType;
    
  ExpiryEntryList(cSimpleModule *module, unsigned int timerId = TMR_ENTRYEXPIRED, bool relative = false);
  ExpiryEntryList(Timer* tmr, bool relative = false);
  ~ExpiryEntryList(void);
    
  void addEntry(Entry newEntry);
  ///Callback for aggregate Entry types. Removes the entry from list and reschedules.
  void removeExpiredEntry(Loki::Int2Type<false>);
  ///Callback for pointer Entry types. Removes the entry from list and reschedules.
  Entry removeExpiredEntry(Loki::Int2Type<true>);
  void removeEntry(Entry &target);
    
  bool empty(void);
  bool findEntry(Entry &target);

  bool smallestExpiryEntry(Entry &smallest);
  ///@deprecated debug function. Use operator<< instead
  void printList(void);

  /// Suitable binary predicate (lessThan) function is needed as argument
  bool findMaxEntry(Entry &max, bool (*lessThan)(const Entry&,const Entry&));

  private:

/**
   @struct greaterExpiryTime
   @brief  Function object for heap comparison 

   @ingroup Timer Framework
*/

  struct greaterExpiryTime : public std::binary_function<Entry, Entry, bool>
  {
    ///Overloaded to handle pointer/non-pointer Entry types. Only one version
    ///will evaluate to the comparison function signature.
    bool operator()(const typename Loki::Select< Loki::TypeTraits<Entry>::isPointer, void*, Entry>::Result& lhs, const Entry& rhs) const
      {
        return (lhs.expiryTime() > rhs.expiryTime());
      }
    bool operator()(const Entry lhs, const typename Loki::Select< Loki::TypeTraits<Entry>::isPointer, Entry, void*>::Result rhs) const
      {
	return (lhs->expiryTime() > rhs->expiryTime());
      }
  };

  ///overloaded expiryTime to handle non-pointer Entry types
  simtime_t expiryTime(const Entry& e, Loki::Int2Type<false>) const
  {
    return e.expiryTime();
  }

  ///overloaded expiryTime to handle pointer Entry types
  simtime_t expiryTime(Entry e, Loki::Int2Type<true>) const
  {
    return e->expiryTime();
  }
  
  ///overloaded printEntry for non-pointer Entry types
  std::ostream& printEntry(std::ostream& os, const Entry& e, Loki::Int2Type<false>) const
  {
    return os<<e;
  }

  ///overloaded printEntry for pointer Entry types
  std::ostream& printEntry(std::ostream& os, Entry e, Loki::Int2Type<true>) const
  {
    return os<<*e;
  }
  
  typedef typename std::vector<Entry> EntryList;
  typedef typename std::vector<Entry>::iterator EntryListIt;
  typedef typename std::vector<Entry>::const_iterator ELCI;

  //Has to be a friend template i.e. all instantiations of operator<< and cannot
  //be a one for one match with Entry, Timer template arguments i.e. operator<<
  //<> because function template operator<< cannot be declared/defined prior to ExpiryEntryList.
  //Alternative is to define an inline non member, non template operator<< at the
  //point of this friend declaration. See 8.4.1 of C++ Templates by Nicholai Josuttis
  
  template <class E, class T> friend std::ostream& operator<< (std::ostream&, const ExpiryEntryList<E,T> &);

  ///required to disambiguate between overloaded removeExpiredEntry call
  typedef void (ExpiryEntryList::*removeExpiredEntryPtr)(typename Loki::Select< Loki::TypeTraits<Entry>::isPointer, Loki::Int2Type<true>, Loki::Int2Type<false> >::Result);
  
  Timer* entryExpiredNotifier;
  EntryList entries;
  bool relative;
};

/**
   @brief Construct list to manage lifetime of entries using default timer message.
   
   Default timer is assigned an appropriate version of removeExpiredEntry as the
   callback function determined by whether Entry is a pointer or not.
   
   @param module the module in which the self message timer will be running in

   @param timerId A unique identifier within scope of each module type (only
   required to be unique if timer is stored with other timers of different
   types)

   @param relative treat expiryTime as the relative time i.e. add simTime() to
   it before rescheduling. By default false i.e. treat as absolute time to
   expire at.   
*/
template <class Entry, class Timer>
ExpiryEntryList<Entry, Timer>::ExpiryEntryList(
  cSimpleModule *module, unsigned int timerId, bool relative):
    entryExpiredNotifier(
      new Loki::cTimerMessageCB<ReturnType, ArgType>(
	timerId, module, this, static_cast<removeExpiredEntryPtr>(
	  &ExpiryEntryList<Entry,Timer>::removeExpiredEntry),
        "removeExpiredEntry")), relative(relative)
{}

/**
   @brief ctor that accepts a Timer and so can schedule any arbitrary function
   as a callback.

   @param tmr with any arbitrary function assigned as a callback. The tmr is
   scheduled according to the smallest lifetime that exists in the list and upon
   expiration of timer will invoke callback function in tmr.

   @param relative treat expiryTime as the relative time i.e. add simTime() to
   it before rescheduling. By default false i.e. treat as absolute time to
   expire at.   

*/
template <class Entry, class Timer>
ExpiryEntryList<Entry, Timer>::ExpiryEntryList(Timer* tmr, bool relative):
    entryExpiredNotifier(tmr), relative(relative)
{
  assert(tmr);
}

template <class Entry, class Timer>
ExpiryEntryList<Entry, Timer>::~ExpiryEntryList(void)
{
  if(!entries.empty())
  {
    entries.clear();
  }
  
  if (!entryExpiredNotifier->isScheduled())
    delete entryExpiredNotifier;
}

/// Add/Update an entry into the list
template <class Entry, class Timer>
void ExpiryEntryList<Entry, Timer>::addEntry(Entry newEntry)
{
  // Remove any existing entry from the list
  entries.erase(std::remove(entries.begin(), entries.end(), newEntry), entries.end());
  
  // Insert the new entry into the heap
  entries.push_back(newEntry);
  make_heap(entries.begin(), entries.end(), greaterExpiryTime());
    
  assert(entryExpiredNotifier);
	
  // Cancel the old scheduled expiry and schedule the soonest to expire.
  // Since the new entry could be the most recent.
  if (!relative)
    entryExpiredNotifier->reschedule(
      expiryTime(entries.front(),
                 Loki::Int2Type<Loki::TypeTraits<Entry>::isPointer>()));
  else
  {
    if (entryExpiredNotifier->isScheduled())
      entryExpiredNotifier->cancel();
    entryExpiredNotifier->rescheduleDelay(
      expiryTime(entries.front(),
                 Loki::Int2Type<Loki::TypeTraits<Entry>::isPointer>()));
  }
}

/**
   @brief Non-pointer version. Remove the entry that has expired and reschedule for next one. 
 
   There are two versions of this due to non/pointer semantics for the call to
   expiryTime. While it is possible to reduce this to a single function via use
   of the ExpiryEntryList::expiryTime this is necessary for the
   prefix/routerEntryTimers of RoutingTable6 that require Entry to be
   returned in order to be removed from the containers there too. Returning a
   complete object for non pointer Entry is unnecessary and inefficient.
   @todo What happens if there is more than 1 entry that has expired?

*/
template <class Entry, class Timer>
void ExpiryEntryList<Entry, Timer>::removeExpiredEntry(Loki::Int2Type<false>)
{
  assert(!entries.empty());

  //Remove expired entry
  pop_heap(entries.begin(), entries.end(), greaterExpiryTime());
  entries.pop_back();

  // Check if there are entries to expire
  if(!entries.empty())
  {
    assert(entryExpiredNotifier && !entryExpiredNotifier->isScheduled());
	
    // Schedule next entry to expire
    if (!relative)
      entryExpiredNotifier->reschedule(entries.front().expiryTime());
    else
      entryExpiredNotifier->rescheduleDelay(entries.front().expiryTime());
  }

}

/// @brief pointer version. Remove the entry that has expired and reschedule
/// for next one. Please see other version for extra comments
template <class Entry, class Timer>
Entry ExpiryEntryList<Entry, Timer>::removeExpiredEntry(Loki::Int2Type<true>)
{
  assert(!entries.empty());

  //Remove expired entry
  pop_heap(entries.begin(), entries.end(), greaterExpiryTime());
  Entry ent = entries.back();
  entries.pop_back();

  // Check if there are entries to expire
  if(!entries.empty())
  {
    assert(entryExpiredNotifier && !entryExpiredNotifier->isScheduled());
	
    // Schedule next entry to expire
    //mod->scheduleAt(entries.front().expiryTime(), entryExpiredNotifier);
    if (!relative)
      entryExpiredNotifier->reschedule(entries.front()->expiryTime());
    else
      entryExpiredNotifier->rescheduleDelay(entries.front()->expiryTime());
  }
  return ent;
}

/// Finds target in entry list and removes it
/// @note operator== for Entry must be defined
template <class Entry, class Timer>
void ExpiryEntryList<Entry, Timer>::removeEntry(Entry &target)
{
  assert(!entries.empty());
  assert(entryExpiredNotifier);
  if (entries.front() == target)
  {
    if (entryExpiredNotifier->isScheduled())
      entryExpiredNotifier->cancel();
    removeExpiredEntry(Loki::Int2Type<Loki::TypeTraits<Entry>::isPointer>());
  }
  else
  {
    // Remove the entry from the list
    entries.erase(std::remove(entries.begin(), entries.end(), target), entries.end());
    make_heap(entries.begin(), entries.end(), greaterExpiryTime());
  }

}

template <class Entry, class Timer>
bool ExpiryEntryList<Entry, Timer>::empty(void)
{
  return entries.empty();
}

/// Finds target in entry list and returns a copy of it in target.
/// @note operator== for Entry must be defined
template <class Entry, class Timer>
bool ExpiryEntryList<Entry, Timer>::findEntry(Entry &target)
{
  EntryListIt it = std::find(entries.begin(), entries.end(), target);
  
  if (it != entries.end())
  {  
        target = *(it);
        return true;
  }
  return false;
}

template <class Entry, class Timer>
bool ExpiryEntryList<Entry, Timer>::smallestExpiryEntry(Entry &smallest)
{
  if(!entries.empty())
  {
    smallest = entries.front();
    return true;
  }

  return false;
}

///Requires Entry::operator<<(std::ostream&) or equivalent if used
template <class Entry, class Timer>
std::ostream& operator<<(std::ostream& os, const ExpiryEntryList<Entry, Timer>& list)
{
  typedef ExpiryEntryList<Entry, Timer> const EEL;
  typedef typename EEL::ELCI ELCI;
  for (ELCI cit = list.entries.begin(); cit != list.entries.end(); cit++)
  {
    if (cit == list.entries.begin())
      os <<" listType="<<typeid(*cit).name()<<"\n";
    list.printEntry(os, *cit, Loki::Int2Type<Loki::TypeTraits<Entry>::isPointer>());
    os<<" expires at "<<
      list.expiryTime(*cit, Loki::Int2Type<Loki::TypeTraits<Entry>::isPointer>())
      <<std::endl;
  }
  return os;
//Causes everything that includes RoutngTable6 to #include sys.h and debug.h
//    Dout(dc::debug, typeid(*cit).name()<<" expires at " << cit->expiryTime());
}

template <class Entry, class Timer>
void ExpiryEntryList<Entry,Timer>::printList(void)
{
  std::cerr << "\nExpiry List:" << std::endl;
  for(EntryListIt it = entries.begin(); it != entries.end(); it++)
  {
    std::cerr << it->identifier() << " " << it->expiryTime() << std::endl;
  }
 }
    
template <class Entry, class Timer>
bool ExpiryEntryList<Entry,Timer>::findMaxEntry(Entry &max, bool (*lessThan)(const Entry&,const Entry&))
{
  if(entries.empty())
    return false;
  else
    max = *(std::max_element(entries.begin(), entries.end(), lessThan));
  return true;
}

#endif // EXPIRYENTRYLIST_H

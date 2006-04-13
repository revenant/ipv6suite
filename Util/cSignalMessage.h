// -*- C++ -*-
// Copyright (C) 2006 by Johnny Lai
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
 * @file cSignalMessage.h
 * @author Johnny Lai
 * @date 27 Jul 2005
 *
 * @brief Definition of class cSignalMessage
 *
 *
 */

#ifndef CSIGNALMESSAGE_H
#define CSIGNALMESSAGE_H

#if !defined CTIMERMESSAGE_H
#include "cTimerMessage.h"
#endif //CTIMERMESSAGE_H

#ifdef BOOST_WITH_LIBS
#if !defined BOOST_NONCOPYABLE_HPP_INCLUDED
#include <boost/noncopyable.hpp>
#endif

#if !defined BOOST_SIGNAL_HPP
#include <boost/signal.hpp>
#endif

/**
 * @class cSignalMessage
 *
 * @brief Allows simple binding of functions to self messages
 *
 * functions can be of arbitrary complexity since we can use boost::bind to
 * actually bind complex member functions with many args and set those args
 */
class cSignalMessage: public cTimerMessage, public boost::signal<void ()>
{
 public:

  //@name constructors, destructors and operators
  //@{
  cSignalMessage(const char* name = NULL, int message_id = 0):
      //module is 0 as it is unused argument
      cTimerMessage(message_id, 0, name), boost::signal<void ()>()
  {}

  //@}

  virtual void callFunc()
  {
    (*this)();
  }

 protected:

 private:

};

#endif
#endif /* CSIGNALMESSAGE_H */


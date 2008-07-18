// -*- C++ -*-
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
 * @file libcwdsetup.h
 * @author Johnny Lai
 * @date 29 Jan 2003
 *
 * @brief function for configuring the libcwd debug channels
 * Conditionally compile with -DCWDEBUG at the gcc command line to enable
 */

#ifndef LIBCWDSETUP_H
#define LIBCWDSETUP_H 1

#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

//Do not use this directly as the Debug macro has to be defined correctly in client's debug.h
//#include <libcw/debug.h>

#endif //CWDEBUG

namespace libcwdsetup
{

#ifdef CWDEBUG

  /**
   * @name l_debugSettings
   * @param channels is a string composed of channel names separated by
   * colons. The very first channel name serves as the filename to log
   * debug output to.
   *
   * Each channel name will be examined to see if a corresponding libcwd debug
   * channel exists.  Consequently that debug channel is turned on.
   *
   * A debug channel is a Dout statement with a flag indicating which channel
   * this message belongs to. Each channel can be switched on or off
   * independently to output its debug messages to the libcwd debug stream.

   * @note This should be called as early as possible.  For OMNeT++ In a simple
   * module's initialise function. This simple module should be the first module
   * declared in the respective ned file's network construct's submodules
   * declaratioin.
   */
  void l_debugSettings(std::string channels);

  ///Search through list of channels and activate it if found
  bool activateChannel(const char* channelName, bool on = true);
  ///Add a channel and associate with name in xml file
  void addChannel(const char* channelName, ::libcwd::channel_ct& chn);
  ///dynamically allocate new channel and add to list
  ::libcwd::channel_ct* createChannel(const char* channelName);

#endif //CWDEBUG

}




#endif /* LIBCWDSETUP_H */

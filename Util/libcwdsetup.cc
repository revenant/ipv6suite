//
// Copyright (C) 2002, 2004 Johnny Lai
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
 * @file   libcwdsetup.cc
 * @author Johnny Lai
 * @date   29 Jan 2003
 *
 * @brief  Implementation of l_debugSettings function
 *
 *
 */


#include "libcwdsetup.h"

#ifdef CWDEBUG

#include "debug.h"

#include <map>
#include <list>


#include <fstream>
#include <exception> //set_terminate
#include <boost/tokenizer.hpp>
#include <string>
#include "opp_utils.h"  // for int/double <==> string conversions
#include <boost/cast.hpp>
#include <iostream>

namespace
{
  ///In hindsight could have used libcw::find_channel
  typedef std::map<std::string, ::libcwd::channel_ct*> Channels;
  Channels g_channels;

  /**
     @class  DeleteThem
     @brief Handle deallocation of debug objects at program termination
  */

  class DeleteThem
  {
  public:

    ///@warning no attempt is made to detect if channel  has already been added
    void addChannel(::libcwd::channel_ct* channel)
      {
        delChannels.push_back(channel);
      }
    ~DeleteThem()
      {
        for (DynamicAllocChannels::iterator it = delChannels.begin(); it != delChannels.end(); it++)
          delete *it;
      }
  private:
    typedef std::list< ::libcwd::channel_ct* > DynamicAllocChannels;
    DynamicAllocChannels delChannels;
  } deleteThem;

  std::ofstream g_debugFile;

  /**
   * @name g_handleExit
   * Function called when exceptions are unhandled or when program quits.
   * This functioin closes the libcwd debug stream and will flush all pending
   * output
   *
   */

  void g_handleExit(void)
  {
    if (g_debugFile.is_open())
      g_debugFile.close();
    std::cerr<<"g_handleExit called\n";
  }
}

namespace libcwdsetup
{
  using std::string;
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ofstream;

  /**
   * @name l_debugSettings
   *
   * processes the channels specified in the XML config file. First token is the
   * filename and others are channel names
   *
   * @note handles case of -DEARLY_CWDEBUG in omnetpp main.cc by closing the debug
   * file and opening up a new one.
   */
  void l_debugSettings(std::string channels)
  {
    static bool doOnceOnly = false;
    if (doOnceOnly)
      return;
    doOnceOnly = true;

    if (!channels.empty())
    {

      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      typedef tokenizer::iterator tokenizerIt;

      boost::char_separator<char> sep(":");
      tokenizer tokens(channels, sep);

      string filename = *tokens.begin();
      //Turn on pid insertion by using a 'p' as first token?
      bool insertPid = false;
      if (filename.size() == 1 && filename.find('p') == 0)
      {
        insertPid = true;
        filename = *(++(tokens.begin()));
      }
      size_t pos = filename.find('.');
      if (pos == std::string::npos)
      {
        cerr<<"Please specify logfile as the first or second token in debugChannel attribute of\n"
            <<"the xml configuration file\n";
        exit(-1);
      }

      if (insertPid)
        filename.insert(pos, "-" + boost::lexical_cast<std::string>(getpid()));

      cout<<"debug output is placed into file "<<filename<<endl;
      g_debugFile.open(filename.c_str());

      if (!libcwd::channels::dc::malloc.is_on())//!defined EARLY_CWDEBUG
      {
        Debug( libcw_do.set_ostream(&g_debugFile) );

        Debug( libcw_do.on() );
        Debug( check_configuration() );

      }
      else
      {
        std::ostream* oldOStream;
        Debug( oldOStream = libcw_do.get_ostream() );
        Debug( libcw_do.set_ostream(&g_debugFile) );
        boost::polymorphic_downcast<ofstream*>(oldOStream)->close();

        if (find(tokens.begin(), tokens.end(), "malloc") == tokens.end())
          Debug( dc::malloc.off());

        if (find(tokens.begin(), tokens.end(), "bfd") == tokens.end())
          Debug( dc::bfd.off() );
      }


      Debug( dc::addChannels(); );

      std::set_terminate(g_handleExit);
      atexit(g_handleExit);

      ///Read rc file if libcwd >= 0.33 and "rcfile" channel exists in
      ///XML and ignore any other channels.  Otherwise if "all" channel exists
      ///then specified channels are turned off. Otherwise just specified
      ///channels are turned on.
#if defined OPP_VERSION && OPP_VERSION >= 3
      if (find(tokens.begin(), tokens.end(), "rcfile") != tokens.end())
      {
        Debug( read_rcfile() );
      }
      else
#endif //defined OPP_VERSION && OPP_VERSION >= 3
      {
        bool activate = true;
        if (find(tokens.begin(), tokens.end(), "all") != tokens.end())
        {
          Debug( dc::notice.on() );
          Dout(dc::notice|dc::custom, "all channels on. Specified channels will be deactivated");
          Debug( dc::notice.off() );
          //Turn on all channels. Any other channels listed will be turned
          //off. Thus we get all + exclusions as well as original just inclusions
          ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on() );
          activate = false;
        }

        for (tokenizerIt it = tokens.begin(); it != tokens.end(); it++)
        {
          bool found = false;
          Debug( found = activateChannel((*it).c_str(), activate); );
          if (!found)
            Dout(dc::notice|dc::custom, "Debug channel "<<*it<<" not found");
        }
      }

      if (find(tokens.begin(), tokens.end(), "listAllocations") != tokens.end())
        Debug( list_allocations_on(libcw_do) );


      Debug( list_channels_on(libcw_do) );

      //Force output up to this point
      g_debugFile.flush();
    }

  }

  ///Add a channel and associate with name in xml file
  void addChannel(const char* channelName, ::libcwd::channel_ct& chn)
  {
    Debug( ::g_channels.insert(std::make_pair(channelName, &chn)); );
  }

  bool activateChannel(const char* channelName, bool on)
  {
    typedef Channels::iterator ChannelsIt;
    ChannelsIt it = ::g_channels.find(std::string(channelName));
    if (it != ::g_channels.end())
    {
      if (on)
      {
    if (!it->second->is_on())
      it->second->on();
    Dout(dc::notice|dc::custom, "activated channel "<<channelName);
      }
      else
      {
    if (it->second->is_on())
      it->second->off();
    Dout(dc::notice|dc::custom, "DeActivated channel "<<channelName);
      }
      return true;
    }
    return false;
  }

  /**
   * @name createChannel
   * @return created channel which can be used subsequently in Dout debug statements
   * @warning no check is made to see if another debug channel shares the same name
   */

  ::libcwd::channel_ct* createChannel(const char* channelName)
  {
    assert(channelName);
    ::libcwd::channel_ct* chan = new ::libcwd::channel_ct(channelName);
    deleteThem.addChannel(chan);
    Debug( ::g_channels.insert(std::make_pair(channelName, chan)); );
    return chan;
  }

}


#endif //CWDEBUG

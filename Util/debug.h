// -*- C++ -*-
// Copyright (C) 2002, 2003, 2004 Johnny Lai
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
 * @file debug.h
 * @author Johnny Lai
 * @date 23 Nov 2001
 *
 * @brief Custom debug channel declarations
 * Conditionally compile with -DCWDEBUG at the gcc command line to enable.
 * Generated automatically from libcwd's autoconf scripts and adapted.
 * To use channels, add #include sys.h and debug.h in this exact order.
 * @see http://libcwd.sourceforge.net
 */

#ifndef DEBUG_H
#define DEBUG_H

#if 0
#undef CWDEBUG
#warning "Undefining CWDEBUG to turn off libcwd"
#endif //CWDEBUG

#ifndef CWDEBUG

#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, x)
#define AllocTag(p, x)
#define Debug(x)
#if !defined Dout
#define Dout(a, b)
#endif //Dout
#define DoutFatal(a, b) LibcwDoutFatal(::std, , a, b)
#define ForAllDebugChannels(STATEMENT)
#define ForAllDebugObjects(STATEMENT)
#define LibcwDebug(dc_namespace, x)
#define LibcwDout(a, b, c, d)
#define LibcwDoutFatal(a, b, c, d) do { ::std::cerr << d << ::std::endl; ::std::exit(254); } while(1)
#define NEW(x) new x
#define CWDEBUG_ALLOC 0
#define CWDEBUG_MAGIC 0
#define CWDEBUG_LOCATION 0
#define CWDEBUG_LIBBFD 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGM 0
#define CWDEBUG_DEBUGT 0
#define CWDEBUG_MARKER 0

#else // CWDEBUG

#if !CWDEBUG_ALLOC
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, x)
#define AllocTag(p, x)
#endif //CWDEBUG_ALLOC

#ifndef DEBUGCHANNELS
// This must be defined before <libcw/debug.h> is included and must be the name
// of the namespace containing your `dc' (Debug Channels) namespace (see below).
// You can use any namespace(s) you like, except existing namespaces (like ::,
// ::std and ::libcwd).
#define DEBUGCHANNELS ::simulacrum::debug::channels
#endif

#if defined OPP_VERSION && OPP_VERSION >= 3
#include <libcwd/debug.h>
#else
#include <libcw/debug.h>
#endif //defined OPP_VERSION && OPP_VERSION >= 3

namespace simulacrum {       // >
  namespace debug {          //  >---> This part must match DEBUGCHANNELS
    namespace channels {     // >

      namespace dc {
        using namespace ::libcwd::channels::dc;

        // Add the declaration of new debug channels here
        // and add their definition in a custom debug.cc file.
        extern ::libcwd::channel_ct custom;

        // Our own debug channels:

        extern ::libcwd::channel_ct xml_addresses;
        extern ::libcwd::channel_ct routing;
        extern ::libcwd::channel_ct forwarding;
        extern ::libcwd::channel_ct encapsulation;
        extern ::libcwd::channel_ct prefix_timer;
        extern ::libcwd::channel_ct address_timer;
        extern ::libcwd::channel_ct router_timer;
        extern ::libcwd::channel_ct addr_resln;
        extern ::libcwd::channel_ct rrprocedure; // return routability procedure
        extern ::libcwd::channel_ct udp;
        extern ::libcwd::channel_ct mobile_move;
        extern ::libcwd::channel_ct mip_missed_adv;
        extern ::libcwd::channel_ct udp_video_svr;
        extern ::libcwd::channel_ct eh;
        extern ::libcwd::channel_ct hmip;
        extern ::libcwd::channel_ct dest_cache_maint;
        extern ::libcwd::channel_ct mipv6;
        extern ::libcwd::channel_ct ipv6;
        extern ::libcwd::channel_ct dual_interface;
        extern ::libcwd::channel_ct ethernet;
        extern ::libcwd::channel_ct wireless_ethernet;
        extern ::libcwd::channel_ct router_disc;
        extern ::libcwd::channel_ct neighbour_disc;
        extern ::libcwd::channel_ct ping6;

        extern ::libcwd::channel_ct statistic;

        //invalid deallocation at exit
        extern ::libcwd::channel_ct ipv6addrdealloc;

        extern const char* ch_Custom;
        extern const char* ch_Debug;
        extern const char* ch_Notice;
        extern const char* ch_XMLAddress;
        extern const char* ch_Routing;
        extern const char* ch_Forwarding;
        extern const char* ch_Encapsulation;
        extern const char* ch_PrefixTimer;
        extern const char* ch_AddressTimer;
        extern const char* ch_RouterTimer;
        extern const char* ch_AddrResln;
        extern const char* ch_RRProc;
        extern const char* ch_UDP;
        extern const char* ch_MobileMove;
        extern const char* ch_MIPv6MissedAdv;
        extern const char* ch_UDPVideoStreamSvr;
        extern const char* ch_EdgeHandover;
        extern const char* ch_HMIP;
        extern const char* ch_MIPv6;
        extern const char* ch_IPv6;
        extern const char* ch_DualInterface;
        extern const char* ch_Ethernet;
        extern const char* ch_WirelessEthernet;
        extern const char* ch_RouterDisc;
        extern const char* ch_NeighbourDisc;
        extern const char* ch_Ping6;

        extern const char* ch_Statistic;

        extern const char* ch_DestCacheMaint;
        extern const char* ch_IPv6AddrDeAlloc;

        ///Add predefined channels from debug.h
        void addChannels();
      } // namespace dc

    } // namespace DEBUGCHANNELS
  }
}

#endif // CWDEBUG

#define FILE_LINE __FILE__<<":"<<__LINE__

#endif // DEBUG_H

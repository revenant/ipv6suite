// -*- C++ -*-
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
 * @file debug.cc
 * @author Johnny Lai
 * @date 23 Nov 2001
 *
 * @brief Custom debug channels definitions
 * Conditionally compile with -DCWDEBUG at the gcc command line to enable.
 * Generated automatically from libcwd's autoconf scripts and adapted
 * @see http://libcwd.sourceforge.net
 */

#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG

#include "libcwdsetup.h"

namespace simulacrum {      // >
  namespace debug {         //  >--> This part must match DEBUGCHANNELS, see debug.h
    namespace channels {    // >

      namespace dc {

        // Add new debug channels here. (NOTE strlen(channel_name) < 16
        ::libcwd::channel_ct custom(ch_Custom);
        ::libcwd::channel_ct xml_addresses(ch_XMLAddress);
        ::libcwd::channel_ct routing(ch_Routing);
        ::libcwd::channel_ct forwarding(ch_Forwarding);
        ::libcwd::channel_ct encapsulation(ch_Encapsulation);
        ::libcwd::channel_ct prefix_timer(ch_PrefixTimer);
        ::libcwd::channel_ct address_timer(ch_AddressTimer);
        ::libcwd::channel_ct router_timer(ch_RouterTimer);
        ::libcwd::channel_ct addr_resln(ch_AddrResln);
        ::libcwd::channel_ct rrprocedure(ch_RRProc); // return routability procedure
        ::libcwd::channel_ct udp(ch_UDP);
        ::libcwd::channel_ct mobile_move(ch_MobileMove);
        ::libcwd::channel_ct mip_missed_adv(ch_MIPv6MissedAdv);
        ::libcwd::channel_ct udp_video_svr(ch_UDPVideoStreamSvr);
      ::libcwd::channel_ct eh(ch_EdgeHandover);
        ::libcwd::channel_ct hmip(ch_HMIP);
        ::libcwd::channel_ct dest_cache_maint(ch_DestCacheMaint);
        ::libcwd::channel_ct mipv6(ch_MIPv6);
        ::libcwd::channel_ct ipv6(ch_IPv6);
        ::libcwd::channel_ct dual_interface(ch_DualInterface);
        ::libcwd::channel_ct ethernet(ch_Ethernet);
        ::libcwd::channel_ct wireless_ethernet(ch_WirelessEthernet);
        ::libcwd::channel_ct router_disc(ch_RouterDisc);
        ::libcwd::channel_ct neighbour_disc(ch_NeighbourDisc);
        ::libcwd::channel_ct ping6(ch_Ping6);

        ::libcwd::channel_ct statistic(ch_Statistic);
        ::libcwd::channel_ct ipv6addrdealloc(ch_IPv6AddrDeAlloc);

        const char* ch_Debug = "debug";
        const char* ch_Notice = "notice";
        const char* ch_Malloc = "malloc";
        const char* ch_BFD = "bfd";

        const char* ch_XMLAddress = "XMLAddresses";
        const char* ch_Routing = "Routing";
        const char* ch_Forwarding = "Forwarding";
        const char* ch_Encapsulation = "Encapsulation";
        const char* ch_PrefixTimer = "PrefixTimer";
        const char* ch_AddressTimer = "AddressTimer";
        const char* ch_RouterTimer = "RouterTimer";
        const char* ch_AddrResln = "AddrResln";
        const char* ch_RRProc = "RRProcedure";
        const char* ch_UDP = "UDP";
        const char* ch_MobileMove = "MobileMove";
        const char* ch_MIPv6MissedAdv = "MIPv6MissedAdv";
        const char* ch_UDPVideoStreamSvr = "UDPVidStrmSvr";
        const char* ch_EdgeHandover = "EdgeHandover";
        const char* ch_HMIP = "HMIPv6";
        const char* ch_Custom = "custom";
        const char* ch_MIPv6 = "MIPv6";
        const char* ch_IPv6 = "IPv6";
        const char* ch_DualInterface = "DualInterface";
        const char* ch_Ethernet = "Ethernet";
        const char* ch_WirelessEthernet = "WirelessEthernet";
        const char* ch_RouterDisc = "RouterDisc";
        const char* ch_NeighbourDisc = "NeighbourDisc";
        const char* ch_Ping6 = "Ping6";

        const char* ch_Statistic = "Statistic";

        const char* ch_IPv6AddrDeAlloc = "IPv6AddrDeAlloc";
        const char* ch_DestCacheMaint = "DestCacheMaint";

        void addChannels()
        {
          using libcwdsetup::addChannel;

          //libcw defined channels
          Debug( addChannel(ch_Debug, dc::debug); );
          Debug( addChannel(ch_Notice, notice); );
          Debug( addChannel(ch_Malloc, dc::malloc); );
          Debug( addChannel(ch_BFD, bfd); );

          //IPv6Suite debug channels
          Debug( addChannel(ch_Custom, custom); );
          Debug( addChannel(ch_XMLAddress, xml_addresses); );

          Debug( addChannel(ch_Routing, routing); );
          Debug( addChannel(ch_Forwarding, forwarding); );
          Debug( addChannel(ch_Encapsulation, encapsulation); );
          Debug( addChannel(ch_PrefixTimer, prefix_timer); );
          Debug( addChannel(ch_AddressTimer, address_timer); );
          Debug( addChannel(ch_RouterTimer, router_timer); );
          Debug( addChannel(ch_AddrResln, addr_resln); );
          Debug( addChannel(ch_RRProc, rrprocedure); );
          Debug( addChannel(ch_UDP, udp); );
          Debug( addChannel(ch_MobileMove, mobile_move); );

          Debug( addChannel(ch_MIPv6MissedAdv, mip_missed_adv); );

          Debug( addChannel(ch_UDPVideoStreamSvr, udp_video_svr); );
          Debug( addChannel(ch_EdgeHandover, eh); );
          Debug( addChannel(ch_HMIP, hmip); );
          Debug( addChannel(ch_DestCacheMaint, dest_cache_maint); );
          Debug( addChannel(ch_MIPv6, mipv6); );
          Debug( addChannel(ch_IPv6, ipv6); );
          Debug( addChannel(ch_DualInterface, dual_interface); );
          Debug( addChannel(ch_Ethernet, ethernet); );
          Debug( addChannel(ch_WirelessEthernet, wireless_ethernet); );
          Debug( addChannel(ch_RouterDisc, router_disc); );
          Debug( addChannel(ch_NeighbourDisc, neighbour_disc); );
          Debug( addChannel(ch_Ping6, ping6); );
          Debug( addChannel(ch_Statistic, statistic); );
        }

      } // namespace dc

    } // namespace DEBUGCHANNELS
  }
}

#endif // CWDEBUG

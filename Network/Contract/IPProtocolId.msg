//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


//
// Protocol field: taken from RFC 1700.
// Replacing old RFC1700 with newer (28/06/01) Protocol Numbers
//
enum IPProtocolId
{
  IP_PROT_HOPOPT    = 0;
  IP_PROT_ICMP      = 1;
  IP_PROT_IGMP      = 2;
  IP_PROT_IP        = 4; // used for IP tunneling. FIXME: RFC1700 specifies =3 (old IPSuite) ???
  IP_PROT_TCP       = 6;
  IP_PROT_EGP       = 8;
  IP_PROT_IGP       = 9;
  IP_PROT_UDP       = 17;
  IP_PROT_XTP       = 36;
  IP_PROT_IPv6      = 41;
  IPv6_PROT_ROUT    = 43;
  IPv6_PROT_FRAG    = 44;
  IP_PROT_RSVP      = 46;
  IPv6_PROT_ESP     = 50;
  IPv6_PROT_AUTH    = 51;
  IPv6_PROT_ICMP    = 58;
  IP_PROT_NONE      = 59;
  IPv6_PROT_DEST    = 60;
  IPv6_PROT_MOB     = 62;
  IP_PROT_OSPF      = 89;
  IP_PROT_SCTP      = 132;
}

//
// Copyright (C) 2005 Andras Varga
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
//

#ifndef __IPV4OR6ADDRESS_H
#define __IPV4OR6ADDRESS_H

#include <omnetpp.h>
#include <string.h>
#include "inetdefs.h"
#include "IPAddress.h"

struct IPv6Address_ {uint32 d[4];std::string str();};  // temporary, until IPv6 addresses get sorted out


/**
 * Stores an IPv4 or an IPv6 address. This class should be used everywhere
 * in transport layer and up, to guarantee IPv4/IPv6 transparence.
 *
 * Storage is efficient: an object occupies size of an IPv6 address
 * (128bits=16 bytes) plus a bool.
 */
class IPvXAddress
{
  protected:
    uint32 d[4];
    bool is6;

  public:
    /** name Constructors, destructor */
    //@{
    /**
     * Constructor for IPv4 addresses.
     */
    IPvXAddress() {is6 = false; d[0] = 0;}

    /**
     * Constructor for IPv4 addresses.
     */
    IPvXAddress(const IPAddress& addr) {set(addr);}

    /**
     * Constructor for IPv6 addresses.
     */
    IPvXAddress(const IPv6Address_& addr) {set(addr);}

    /**
     * Copy constructor.
     */
    IPvXAddress(const IPvXAddress& addr) {set(addr);}

    /**
     * Destructor
     */
    ~IPvXAddress() {}
    //@}

    /** name Getters, setters */
    //@{
    /**
     * Is this an IPv6 address?
     */
    bool isIPv6() const {return is6;}

    /**
     * Get IPv4 address. Throws exception if this is an IPv6 address.
     */
    IPAddress get4() const {
        if (is6)
            throw new cException("IPvXAddress: cannot return IPv6 address %s as IPv4", str().c_str());
        return IPAddress(d[0]);
    }

    /**
     * Get IPv6 address. Throws exception if this is an IPv4 address.
     */
    IPv6Address_ get6() const {
        if (!is6)
            throw new cException("IPvXAddress: cannot return IPv4 address %s as IPv6", str().c_str());
        IPv6Address_ ret;
        ret.d[0] = d[0]; ret.d[1] = d[1]; ret.d[2] = d[2]; ret.d[3] = d[3];
        return ret;
    }

    /**
     * Set to an IPv4 address.
     */
    void set(const IPAddress& addr)  {
        is6 = false;
        d[0] = addr.getInt();
    }

    /**
     * Set to an IPv6 address.
     */
    void set(const IPv6Address_& addr)  {
        is6 = true;
        d[0] = addr.d[0]; d[1] = addr.d[1]; d[2] = addr.d[2]; d[3] = addr.d[3];
    }

    /**
     * Assignment
     */
    void set(const IPvXAddress& addr) {
        is6 = addr.is6;
        d[0] = addr.d[0];
        if (is6) {
            d[1] = addr.d[1]; d[2] = addr.d[2]; d[3] = addr.d[3];
         }
    }

    /**
     * Assignment
     */
    IPvXAddress& operator=(const IPAddress& addr) {set(addr); return *this;}

    /**
     * Assignment
     */
    IPvXAddress& operator=(const IPv6Address_& addr) {set(addr); return *this;}

    /**
     * Assignment
     */
    IPvXAddress& operator=(const IPvXAddress& addr) {set(addr); return *this;}

    /**
     * Returns the string representation of the address (e.g. "152.66.86.92")
     */
    std::string str() const {return is6 ? get6().str() : get4().str();}
    //@}

    /** name Comparison */
    //@{
    /**
     * True if the structure has not been assigned any address yet.
     */
    bool isNull() const {
        return d[0]==0 && (!is6 || (d[1]==0 && d[2]==0 && d[3]==0));
    }

//FIXME TBD: NULL ADDRESSES MUST COMPARE EQUAL!!!!!!!!!!!!!!!!!!!!

    /**
     * Returns true if the two addresses are equal
     */
    bool equals(const IPAddress& addr) const {
        return !is6 && d[0]==addr.getInt();
    }

    /**
     * Returns true if the two addresses are equal
     */
    bool equals(const IPv6Address_& addr) const {
        return is6 && d[0]==addr.d[0] && d[1]==addr.d[1] && d[2]==addr.d[2] && d[3]==addr.d[3];
    }

    /**
     * Returns true if the two addresses are equal
     */
    bool equals(const IPvXAddress& addr) const {
        return d[0]==addr.d[0] && (!is6 || (d[1]==addr.d[1] && d[2]==addr.d[2] && d[3]==addr.d[3]));
    }

    /**
     * Returns equals(addr).
     */
    bool operator==(const IPAddress& addr) const {return equals(addr);}

    /**
     * Returns !equals(addr).
     */
    bool operator!=(const IPAddress& addr) const {return !equals(addr);}

    /**
     * Returns equals(addr).
     */
    bool operator==(const IPv6Address_& addr) const {return equals(addr);}

    /**
     * Returns !equals(addr).
     */
    bool operator!=(const IPv6Address_& addr) const {return !equals(addr);}

    /**
     * Returns equals(addr).
     */
    bool operator==(const IPvXAddress& addr) const {return equals(addr);}

    /**
     * Returns !equals(addr).
     */
    bool operator!=(const IPvXAddress& addr) const {return !equals(addr);}

    /**
     * Compares two addresses.
     */
    bool operator<(const IPvXAddress& addr) const {
        if (is6!=addr.is6)
            return !is6;
        else if (!is6)
            return d[0]<addr.d[0];
        else
            return memcmp(&d, &addr.d, 16) < 0;  // this provides an ordering, though not surely the one one would expect
    }
    //@}
};

inline std::ostream& operator<<(std::ostream& os, const IPvXAddress& ip)
{
    return os << ip.str();
}

#endif



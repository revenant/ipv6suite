//
// Conversions between ipv6_addr and IPvXAddress's IPv6Address_.
// This should become redundant once IPv6 address classes get sorted out
// --Andras
//
#ifndef IPV6ADDRCONV_H
#define IPV6ADDRCONV_H

#include "ipv6_addr.h"
#include "IPvXAddress.h"

inline IPv6Address_ mkIPv6Address_(const ipv6_addr& addr) {
    IPv6Address_  ret;
    ret.d[0] = addr.extreme;
    ret.d[1] = addr.high;
    ret.d[2] = addr.normal;
    ret.d[3] = addr.low;
    return ret;
}

inline ipv6_addr mkIpv6_addr(const IPv6Address_& addr) {
    ipv6_addr ret;
    ret.extreme = addr.d[0];
    ret.high =    addr.d[1];
    ret.normal =  addr.d[2];
    ret.low =     addr.d[3];
    return ret;
}

#endif


//
// Monash University, Melbourne, Australia

/**
 *  File:    MACAddress6.cc
 *
 *  Purpose: Represention of an Ethernet MAC address
 *
 *  Author:  Eric Wu
 *
 *  Date:    08/11/2001
 */


#include "MACAddress6.h"
#include <sstream>
#include <cstdio>

using namespace std;

const char* MAC_ADDR_SEP = ":";

const MAC_address MAC_ADDRESS_UNSPECIFIED_STRUCT = {0,0};

std::ostream& operator<<( std::ostream& os, const MAC_address& src_addr)
{
  os<<hex<<
    ((src_addr.high & 0xFF0000) >> 16) << MAC_ADDR_SEP <<
    ((src_addr.high & 0xFF00) >> 8) << MAC_ADDR_SEP <<
    ((src_addr.high & 0xFF)) <<  MAC_ADDR_SEP <<
    ((src_addr.low & 0xFF0000) >> 16) <<  MAC_ADDR_SEP <<
    ((src_addr.low & 0xFF00) >> 8) <<  MAC_ADDR_SEP <<
    ((src_addr.low & 0xFF));

  return os;
}

/**
 * @todo Fix up this weird encoding so the lhs.high==rhs.high &&
 * lhs.low==rhs.low can be used instead
 *
 */

bool operator==(const MAC_address& lhs, const MAC_address& rhs)
{
  return (lhs.high & 0xFFFFFF) == (rhs.high & 0xFFFFFF) && (lhs.low & 0xFFFFFF) == (rhs.low & 0xFFFFFF);
}

bool operator!=(const MAC_address& lhs, const MAC_address& rhs)
{
  return !(lhs.high & 0xFFFFFF) == (rhs.high & 0xFFFFFF) && (lhs.low & 0xFFFFFF) == (rhs.low & 0xFFFFFF);
}

MACAddress6::MACAddress6(const char* addr) :
  mac_addr(MAC_ADDRESS_UNSPECIFIED_STRUCT)
{
  set(addr);
}

MACAddress6::MACAddress6(const MAC_address& addr) :
  mac_addr(MAC_ADDRESS_UNSPECIFIED_STRUCT)
{
  mac_addr.high = addr.high;
  mac_addr.low = addr.low;
}

MACAddress6::MACAddress6(const MACAddress6& obj)
{
  MACAddress6::operator=(obj);
}

MACAddress6& MACAddress6::operator=(const MACAddress6& obj)
{
  mac_addr = obj.mac_addr;
  return *this;
}

inline std::ostream& operator<<(std::ostream& os, MACAddress6& obj)
{
  return os << obj.stringValue();
}

void MACAddress6::set(const char* addr)
{
  stringstream is(addr);
  this->operator>>(is);
}

MACAddress6::operator MAC_address() const
{
  return mac_addr;
}

MACAddress6::operator const char*()
{
  return stringValue();
}

void MACAddress6::set(const MAC_address& addr)
{
  mac_addr.high = addr.high;
  mac_addr.low = addr.low;
}

const char* MACAddress6::stringValue(void) const
{
  // address format ABCD:ABCD
  static char output_str[32];

  if(mac_addr.high == 0 && mac_addr.low == 0)
    return "";

  //return "aa:bb:cc:dd:ee:ff";
  sprintf(output_str, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
          (mac_addr.high & 0xFF0000) >> 16,
          (mac_addr.high & 0xFF00) >> 8,
          (mac_addr.high & 0xFF),
          (mac_addr.low & 0xFF0000) >> 16,
          (mac_addr.low & 0xFF00) >> 8,
          (mac_addr.low & 0xFF));

  return output_str;
}

std::istream& MACAddress6::operator>>(std::istream& is)
{
  int octals[6] = {0, 0, 0, 0, 0, 0};
  char sep = '\0';

  try
  {
    for (int i = 0; i < 6; i ++)
    {

      is >> hex >> octals[i];
      if (is.eof() )
        break;
      is >> sep;
    }
  }
  catch (...)
  {
    cerr << "exception thrown while parsing MAC Address";
  }

  mac_addr.high = (octals[0] << 16) + (octals[1] << 8) + octals[2];
  mac_addr.low = (octals[3] << 16) + (octals[4] << 8) + octals[5];

  return is;

}

std::ostream& operator<<(std::ostream& os, const MACAddress6& obj)
{
  return os<<obj.mac_addr;
}

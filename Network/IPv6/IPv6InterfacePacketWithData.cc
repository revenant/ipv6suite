#include "IPv6InterfacePacketWithData.h"

#if !defined __IPv6INTERFACEDATA_CC
#define __IPv6INTERFACEDATA_CC

/*export*/ template<class DataType>
IPv6InterfacePacketWithData<DataType>::IPv6InterfacePacketWithData(const DataType& data, const char* src, const char* dest)
  :IPv6InterfacePacket(src, dest), _data(data)
{
  addLength(data.length());
}

/*export*/ template<class DataType>
IPv6InterfacePacketWithData<DataType>::IPv6InterfacePacketWithData(const  DataType& data,
                                               const ipv6_addr& src,
                                               const ipv6_addr& dest)
  :IPv6InterfacePacket(src, dest), _data(data)
{
  addLength(data.length());
}

template<class DataType>
IPv6InterfacePacketWithData<DataType>::IPv6InterfacePacketWithData(const IPv6InterfacePacketWithData<DataType>& src)
  :IPv6InterfacePacket(src), _data(src._data)
{}

template<class DataType>
IPv6InterfacePacketWithData<DataType>& IPv6InterfacePacketWithData<DataType>::operator=(const IPv6InterfacePacketWithData<DataType>& rhs)
{
  if (this != &rhs)
  {
    IPv6InterfacePacket::operator=(rhs);
    _data = rhs._data;
  }
  return *this;
}

template<class DataType>
std::string IPv6InterfacePacketWithData<DataType>::info()
{
  return std::string();
}

template<class DataType>
inline void IPv6InterfacePacketWithData<DataType>::insertData(const DataType& data)
{
  //setLength(sizeof(data));
  setLength(data.length());
  _data = data;
}

template<class DataType>
inline DataType& IPv6InterfacePacketWithData<DataType>::data() const
{
  return _data;
}

template<class DataType>
inline DataType IPv6InterfacePacketWithData<DataType>::removeData()
{
  setLength(0);
  return _data;
}

#endif //__ICMPv6NDMESSAGE_CC

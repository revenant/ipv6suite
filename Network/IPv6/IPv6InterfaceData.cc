#include "IPv6InterfaceData.h"

#if !defined __IPv6INTERFACEDATA_CC
#define __IPv6INTERFACEDATA_CC

/*export*/ template<class DataType>
IPv6InterfaceData<DataType>::IPv6InterfaceData(const DataType& data, const char* src, const char* dest)
  :IPv6InterfacePacket(src, dest), _data(data)
{
  addLength(data.length());
}

/*export*/ template<class DataType>
IPv6InterfaceData<DataType>::IPv6InterfaceData(const  DataType& data,
                                               const ipv6_addr& src,
                                               const ipv6_addr& dest)
  :IPv6InterfacePacket(src, dest), _data(data)
{
  addLength(data.length());
}

template<class DataType>
IPv6InterfaceData<DataType>::IPv6InterfaceData(const IPv6InterfaceData<DataType>& src)
  :IPv6InterfacePacket(src), _data(src._data)
{}

template<class DataType>
IPv6InterfaceData<DataType>& IPv6InterfaceData<DataType>::operator=(const IPv6InterfaceData<DataType>& rhs)
{
  if (this != &rhs)
  {
    IPv6InterfacePacket::operator=(rhs);
    _data = rhs._data;
  }
  return *this;
}

template<class DataType>
void IPv6InterfaceData<DataType>::info(char *buf)
{}

template<class DataType>
inline void IPv6InterfaceData<DataType>::insertData(const DataType& data)
{
  //setLength(sizeof(data));
  setLength(data.length());
  _data = data;      
}

template<class DataType>
inline DataType& IPv6InterfaceData<DataType>::data() const
{
  return _data; 
}

template<class DataType>
inline DataType IPv6InterfaceData<DataType>::removeData()
{
  setLength(0);
  return _data;
}

#endif //__ICMPv6NDMESSAGE_CC

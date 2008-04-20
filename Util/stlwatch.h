/*******************************************************************
*
*    This library is free software, you can redistribute it
*    and/or modify
*    it under  the terms of the GNU Lesser General Public License
*    as published by the Free Software Foundation;
*    either version 2 of the License, or any later version.
*    The library is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*    See the GNU Lesser General Public License for more details.
*
*
*********************************************************************/


#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif 

#if (defined OMNETPP_VERSION && OMNETPP_VERSION >= 0x0302)
#include "cstlwatch.h"
typedef cStdVectorWatcherBase cVectorWatcherBase;
#else
#ifndef _STLWATCH_H__
#define _STLWATCH_H__
#endif

#include <vector>
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <sstream>

//
// Internal class
//
class cVectorWatcherBase : public cObject
{
  public:
    cVectorWatcherBase(const char *name) : cObject(name) {}

    virtual std::string info() const;
    virtual std::string detailedInfo() const;

    virtual const char *elemTypeName() const = 0;
    virtual int size() const = 0;
    virtual std::string at(int i) const = 0;
    virtual cStructDescriptor *createDescriptor();
};


//
// Internal class
//
template<class T>
class cVectorWatcher : public cVectorWatcherBase
{
  protected:
    std::vector<T>& v;
    std::string classname;
  public:
    cVectorWatcher(const char *name, std::vector<T>& var) : cVectorWatcherBase(name), v(var) {
        classname = std::string("std::vector<")+opp_typename(typeid(T))+">";
    }
    const char *className() const {return classname.c_str();}
    virtual const char *elemTypeName() const {return opp_typename(typeid(T));}
    virtual int size() const {return v.size();}
    virtual std::string at(int i) const {std::stringstream out; out << v[i]; return out.str();}
};

template <class T>
void createVectorWatcher(const char *varname, std::vector<T>& v)
{
    new cVectorWatcher<T>(varname, v);
}


//
// Internal class
//
template<class T>
class cPointerVectorWatcher : public cVectorWatcher<T>
{
  public:
    cPointerVectorWatcher(const char *name, std::vector<T>& var) : cVectorWatcher<T>(name, var) {}
    virtual std::string at(int i) const {std::stringstream out; out << *(this->v[i]); return out.str();}
};

template <class T>
void createPointerVectorWatcher(const char *varname, std::vector<T>& v)
{
    new cPointerVectorWatcher<T>(varname, v);
}

//
// Internal class
//
template<class T>
class cListWatcher : public cVectorWatcherBase
{
  protected:
    std::list<T>& v;
    std::string classname;
    mutable typename std::list<T>::iterator it;
    mutable int itPos;
  public:
    cListWatcher(const char *name, std::list<T>& var) : cVectorWatcherBase(name), v(var) {
        itPos=-1;
        classname = std::string("std::list<")+opp_typename(typeid(T))+">";
    }
    const char *className() const {return classname.c_str();}
    virtual const char *elemTypeName() const {return opp_typename(typeid(T));}
    virtual int size() const {return v.size();}
    virtual std::string at(int i) const {
        // std::list doesn't support random access iterator and iteration is slow,
        // so we have to use a trick, knowing that Tkenv will call this function with
        // i=0, i=1, etc...
        if (i==0) {
            it=v.begin(); itPos=0;
        } else if (i==itPos+1 && it!=v.end()) {
            ++it; ++itPos;
        } else {
            it=v.begin();
            for (int k=0; k<i && it!=v.end(); k++) ++it;
            itPos=i;
        }
        if (it==v.end()) {
            return std::string("out of bounds");
        }
        return atIt();
    }
    virtual std::string atIt() const {
        std::stringstream out;
        out << (*it);
        return out.str();
    }
};

template <class T>
void createListWatcher(const char *varname, std::list<T>& v)
{
    new cListWatcher<T>(varname, v);
}


//
// Internal class
//
template<class T>
class cPointerListWatcher : public cListWatcher<T>
{
  public:
    cPointerListWatcher(const char *name, std::list<T>& var) : cListWatcher<T>(name, var) {}
    virtual std::string atIt() const {
        std::stringstream out;
        out << (**this->it);
        return out.str();
    }
};

template <class T>
void createPointerListWatcher(const char *varname, std::list<T>& v)
{
    new cPointerListWatcher<T>(varname, v);
}

//
// Internal class
//
template<class KeyT, class ValueT, class CmpT>
class cMapWatcher : public cVectorWatcherBase
{
  protected:
    std::map<KeyT,ValueT,CmpT>& m;
    mutable typename std::map<KeyT,ValueT,CmpT>::iterator it;
    mutable int itPos;
    std::string classname;
  public:
    cMapWatcher(const char *name, std::map<KeyT,ValueT,CmpT>& var) : cVectorWatcherBase(name), m(var) {
        itPos=-1;
        classname = std::string("std::map<")+opp_typename(typeid(KeyT))+","+opp_typename(typeid(ValueT))+">";
    }
    const char *className() const {return classname.c_str();}
    virtual const char *elemTypeName() const {return "struct pair<*,*>";}
    virtual int size() const {return m.size();}
    virtual std::string at(int i) const {
        // std::map doesn't support random access iterator and iteration is slow,
        // so we have to use a trick, knowing that Tkenv will call this function with
        // i=0, i=1, etc...
        if (i==0) {
            it=m.begin(); itPos=0;
        } else if (i==itPos+1 && it!=m.end()) {
            ++it; ++itPos;
        } else {
            it=m.begin();
            for (int k=0; k<i && it!=m.end(); k++) ++it;
            itPos=i;
        }
        if (it==m.end()) {
            return std::string("out of bounds");
        }
        return atIt();
    }
    virtual std::string atIt() const {
        std::stringstream out;
        out << "{" << it->first << "}  ==>  {" << it->second << "}";
        return out.str();
    }
};

template <class KeyT, class ValueT, class CmpT>
void createMapWatcher(const char *varname, std::map<KeyT,ValueT,CmpT>& m)
{
    new cMapWatcher<KeyT,ValueT,CmpT>(varname, m);
}


//
// Internal class
//
template<class KeyT, class ValueT, class CmpT>
class cPointerMapWatcher : public cMapWatcher<KeyT,ValueT,CmpT>
{
  public:
    cPointerMapWatcher(const char *name, std::map<KeyT,ValueT,CmpT>& var) : cMapWatcher<KeyT,ValueT,CmpT>(name, var) {}
    virtual std::string atIt() const {
        std::stringstream out;
        out << "{" << this->it->first << "}  ==>  {" << *(this->it->second) << "}";
        return out.str();
    }
};

template<class KeyT, class ValueT, class CmpT>
void createPointerMapWatcher(const char *varname, std::map<KeyT,ValueT,CmpT>& m)
{
    new cPointerMapWatcher<KeyT,ValueT,CmpT>(varname, m);
}


#endif // (defined OMNETPP_VERSION && OMNETPP_VERSION >= 0x0302)


//
// Internal class
//
template<class KeyT, class ValueT, class CmpT>
class cMultiMapWatcher : public cVectorWatcherBase
{
  protected:
    std::multimap<KeyT,ValueT,CmpT>& m;
    mutable typename std::multimap<KeyT,ValueT,CmpT>::iterator it;
    mutable int itPos;
    std::string classname;
  public:
    cMultiMapWatcher(const char *name, std::multimap<KeyT,ValueT,CmpT>& var) : cVectorWatcherBase(name), m(var) {
        itPos=-1;
        classname = std::string("std::multimap<")+opp_typename(typeid(KeyT))+","+opp_typename(typeid(ValueT))+">";
    }
    const char *className() const {return classname.c_str();}
    virtual const char *elemTypeName() const {return "struct pair<*,*>";}
    virtual int size() const {return m.size();}
    virtual std::string at(int i) const {
        // std::multimap doesn't support random access iterator and iteration is slow,
        // so we have to use a trick, knowing that Tkenv will call this function with
        // i=0, i=1, etc...
        if (i==0) {
            it=m.begin(); itPos=0;
        } else if (i==itPos+1 && it!=m.end()) {
            ++it; ++itPos;
        } else {
            it=m.begin();
            for (int k=0; k<i && it!=m.end(); k++) ++it;
            itPos=i;
        }
        if (it==m.end()) {
            return std::string("out of bounds");
        }
        return atIt();
    }
    virtual std::string atIt() const {
        std::stringstream out;
        out << "{" << it->first << "}  ==>  {" << it->second << "}";
        return out.str();
    }
};

template <class KeyT, class ValueT, class CmpT>
void createMultiMapWatcher(const char *varname, std::multimap<KeyT,ValueT,CmpT>& m)
{
    new cMultiMapWatcher<KeyT,ValueT,CmpT>(varname, m);
}

template<class KeyT, class ValueT, class CmpT>
class cPointerMultiMapWatcher : public cMultiMapWatcher<KeyT,ValueT,CmpT>
{
  public:
    cPointerMultiMapWatcher(const char *name, std::multimap<KeyT,ValueT,CmpT>& var) : cMultiMapWatcher<KeyT,ValueT,CmpT>(name, var) {}
    virtual std::string atIt() const {
        std::stringstream out;
        out << "{" << this->it->first << "}  ==>  {" << *(this->it->second) << "}";
        return out.str();
    }
};

template<class KeyT, class ValueT, class CmpT>
void createPointerMultiMapWatcher(const char *varname, std::multimap<KeyT,ValueT,CmpT>& m)
{
    new cPointerMultiMapWatcher<KeyT,ValueT,CmpT>(varname, m);
}

#ifndef BOOST_CIRCULAR_BUFFER_FWD_HPP
#include <boost/circular_buffer_fwd.hpp>
#endif

template <class T, class Alloc>
class CircularBufferWatcher : public cVectorWatcherBase
{
  protected:
    boost::circular_buffer<T,Alloc>& cb;
    std::string classname;
  public:
    CircularBufferWatcher(const char *name, boost::circular_buffer<T,Alloc>& var) : cVectorWatcherBase(name), cb(var) {
      classname = std::string("boost::circular_buffer<")+opp_typename(typeid(T))+">";
    }
    const char *className() const {return classname.c_str();}
    virtual const char *elemTypeName() const {return opp_typename(typeid(T));}
    virtual int size() const {return cb.size();}
    virtual std::string at(int i) const {std::stringstream out; out << cb[i]; return out.str();}
};

template<class T, class Alloc>
void createCircularBufferWatcher(const char *varname, boost::circular_buffer<T,Alloc>& cb)
{
    new CircularBufferWatcher<T, Alloc>(varname, cb);
}

#ifndef WATCH_VECTOR   /* it's already included in omnetpp-3.2 */

#define WATCH_VECTOR(v)      createVectorWatcher(#v,(v))

#define WATCH_PTRVECTOR(v)   createPointerVectorWatcher(#v,(v))

#define WATCH_LIST(v)        createListWatcher(#v,(v))

#define WATCH_PTRLIST(v)     createPointerListWatcher(#v,(v))

#define WATCH_MAP(m)         createMapWatcher(#m,(m))

#define WATCH_PTRMAP(m)      createPointerMapWatcher(#m,(m))

#endif

#define WATCH_MULTIMAP(m)   createMultiMapWatcher(#m,(m))

#define WATCH_PTRMULTIMAP(m)   createPointerMultiMapWatcher(#m,(m))

#define WATCH_RINGBUFFER(m) createCircularBufferWatcher(#m,(m))

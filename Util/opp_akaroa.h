// -*- C++ -*-
//
// Copyright (C) 2003 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file opp_akaroa.h
 * @author Johnny Lai
 * @date 07 Nov 2003
 *
 * @brief Hack to get Akaroa or OMNET random functions depending on whether
 * USE_AKAROA is defined.
 *
 */

#ifndef OPP_AKAROA_H
#define OPP_AKAROA_H 1

#ifndef CONFIG_H
#include "config.h"
#endif //CONFIG_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#if USE_AKAROA
#ifndef akaroa_H
#include <akaroa.H>
#endif //akaroa_H
#endif //USE_AKAROA

#if USE_AKAROA
#ifndef ak_distribution_H
#include <akaroa/distributions.H>
#endif //ak_distribution_H

///Returns a continuous uniform distribution
#define OPP_UNIFORM(a,b) \
    Uniform(a,b)
///Returns a discrete uniform distribution
#define OPP_UNIFORMINT(a,b) \
    UniformInt(a,b)
#else
///Returns a continuous uniform distribution
#define OPP_UNIFORM(a,b) \
    uniform(a,b)
///Returns a discrete uniform distribution
#define OPP_UNIFORMINT(a,b) \
    intuniform(a,b)
#endif //USE_AKAROA



#endif /* OPP_AKAROA_H */


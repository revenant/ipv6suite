// -*- C++ -*-
//
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
//
// This header file is included at the top of every source file that uses libcwd
// before any other header file.  It is intended to add defines that are needed
// globally and to work around Operating System dependant incompatibilities.

#ifndef SYS_H

#ifdef CWDEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __DEFS_H
#include "defs.h" //OMNETPP_VERSION
#endif
#if (defined OPP_VERSION && OPP_VERSION >= 3) || OMNETPP_VERSION >= 0x300
#include <libcwd/sys.h>
#else
#include <libcw/sysd.h>
#endif //defined OPP_VERSION && OPP_VERSION >= 3
#endif // CWDEBUG

#endif //SYS_H

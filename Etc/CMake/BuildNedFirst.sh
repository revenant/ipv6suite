#!/bin/bash
#$Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Etc/CMake/Attic/BuildNedFirst.sh,v 1.1 2005/02/09 06:15:57 andras Exp $
#
# Copyright (C) 2003 by Johnny Lai
#
# IPv6Suite is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# IPv6Suite is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#
# This script enables generation of nedc sources only by recursively looking for
# *_n.cc targets in Makefiles and calling make on all of them. Required to
# enable parallel building as nedc segfaults sometimes when make -j is invoked.

MAKEFILENAME=Makefile
DIRS=`find . -name ${MAKEFILENAME} -exec grep -l nedc {} \;|sed "s|${MAKEFILENAME}||g"`
for d in $DIRS;
do
  pushd $d &> /dev/null
  NEDCSRCS=`grep '_n.cc\:' ${MAKEFILENAME}|sed 's|\:.*$||g'`
  MSGCSRCS=`grep '_m.cc\:' ${MAKEFILENAME}|sed 's|\:.*$||g'`
  TEST=`echo -n $NEDCSRCS|sed 's|^[:blank:]+$||'`
  if [ "$TEST" = "" ]; then
      echo empty TEST
  fi
  make $NEDCSRCS $MSGCSRCS
  popd  &> /dev/null
done

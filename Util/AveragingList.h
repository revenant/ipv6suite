// Copyright (C) 2004 Monash University, Melbourne, Australia
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

/**
 * @file AveragingList.h
 * @author Steve Woon
 * @date 10 June 2004
 *
 * @brief Definition of AveragingList which stores a list of values
 * and finds the average of them.
 */

#ifndef AVERAGING_LIST_H
#define AVERAGING_LIST_H

#include<list>
#include<assert.h>

extern const int INVALID_AVERAGE;

class AveragingList
{
  public:
    AveragingList(int max = 1);
    ~AveragingList(void) {};

    void upSize(unsigned int);
    void updateAverage(double);
    void resetReadings(void);
    double getAverage(void) const;

  private:

    double average;
    // maximum number of readings
    unsigned int maxSample; 
    // store list of readings
    std::list<double> readings;
};

#endif

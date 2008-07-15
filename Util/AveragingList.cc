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
 * @file AveragingList.cc
 * @author Steve Woon
 * @date 10 June 2004
 *
 * @brief Implementation of AveragingList.
 */


#include<list>
#include"AveragingList.h"

const int INVALID_AVERAGE = -99999;

// Constructor
AveragingList::AveragingList(int max)
{
  assert(max > 0);
  maxSample = max;
  average = INVALID_AVERAGE;
}

// Re-size the number of readings to a greater size
void AveragingList::upSize(unsigned int newMax)
{
  assert(newMax >= maxSample);
  maxSample = newMax;
}
   
// Insert a new entry to the averaging list and update the average   
void AveragingList::updateAverage(double newReading)
{
  assert(readings.size() <= maxSample);
 
  // Update average and insert new reading into the list.
  if(readings.size() == maxSample)
  {
    average = ((average*maxSample)-readings.front()+newReading)/maxSample;
    readings.pop_front();
  }
  else
  {
    average = ((average*readings.size())+newReading)/(readings.size()+1);
  }
  readings.push_back(newReading);
}

// Reset the whole list
void AveragingList::resetReadings(void)
{
  average = INVALID_AVERAGE;
  readings.clear();
}

// Return the average value of readings
double AveragingList::getAverage(void) const
{
  assert(average != INVALID_AVERAGE);
  return average;
}

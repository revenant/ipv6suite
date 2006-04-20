// -*- C++ -*-
// Copyright (C) 2005 Monash University
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

/*
 * Single queue for regular 802.11 implementations
 */
#include <cqueue.h>
#include <assert.h>
#include "WESingleQueue.h"
#include "wirelessethernet.h"
#include "WirelessEtherFrame_m.h"

Register_Class(WESingleQueue);

WESingleQueue::WESingleQueue()
{
    slotTime = SLOTTIME;
    this->DIFS =::DIFS;
    CWmin = CW_MIN;
    CWmax = 1023;
    retryLimit = 8;
    maxQueueSize = -1;          // infinite

    contentionStartTime = 0;
    timeToSend = 0;
    CW = CW_MIN;
    retry = 0;
    backoffSlots = 0;
    readyFrame = NULL;
    collided = 0;
    attempted = 0;
    avgFrameSize = new cStdDev("avgFrameSize");
}

WESingleQueue::~WESingleQueue()
{
    reset();
    delete avgFrameSize;
}

// Set the maximum queue size
/*void WESingleQueue::initialise(int TMRID, int)
{
    maxQueueSize = size;
}*/

// Set the maximum queue size
void WESingleQueue::setMaxQueueSize(unsigned int size)
{
    maxQueueSize = size;
}

// Set the retry limit
void WESingleQueue::setRetryLimit(unsigned int limit)
{
    retryLimit = limit;
}

// get the number of retries
unsigned int WESingleQueue::getRetry()
{
    return retry;
}

// get the CW size
unsigned int WESingleQueue::getCW()
{
    return CW;
}

// get the number of backoffSlots
unsigned int WESingleQueue::getBackoffSlots()
{
    return backoffSlots;
}

int WESingleQueue::getQueueType()
{
    return 1;
}

int WESingleQueue::getAttempted(int queue)
{
    return attempted;
}

int WESingleQueue::getCollided(int queue)
{
    return collided;
}

double WESingleQueue::getAvgFrameSize(int queue)
{
    return avgFrameSize->mean();
}

// Get the total queue size, including the frame in readyFrame
int WESingleQueue::size()
{
    int length = queue.length();
    if (readyFrame != NULL)
        length++;
    return length;
}

// Reset queue parameters and clear contents
void WESingleQueue::reset()
{
    // Reset parameters
    retry = 0;
    contentionStartTime = 0;
    timeToSend = 0;
    backoffSlots = 0;
    CW = CWmin;

    // Remove readyFrame
    if (readyFrame != NULL)
    {
        delete readyFrame;
        readyFrame = NULL;
    }
    // Remove queue contents
    while (!queue.empty())
        delete queue.pop();
}

// Remove current readyFrame and reset parameters
void WESingleQueue::prepareNextFrame(double time)
{
    // Reset parameters
    retry = 0;
    contentionStartTime = 0;
    timeToSend = 0;
    backoffSlots = 0;
    CW = CWmin;

    // Remove current readyFrame
    if (readyFrame != NULL)
    {
        avgFrameSize->collect(readyFrame->length());
        delete readyFrame;
        readyFrame = NULL;
    }
    // Get new readyFrame
    if (!queue.empty())
    {
        attempted++;
        readyFrame = check_and_cast<WirelessEtherBasicFrame *>(queue.pop());
    }
}

// Insert frame into the queue
void WESingleQueue::insertFrame(WirelessEtherBasicFrame *frame, double time)
{
    // Insert Frame into queue if limit has not been reached
    if ((maxQueueSize < 0) || (queue.length() < maxQueueSize))
    {
        queue.insert(static_cast<cObject *>(frame));
    }
    // Remove oldest entry and insert new entry
    else
    {
        delete queue.pop();
        queue.insert(static_cast<cObject *>(frame));
    }
    // Prepare new readyFrame if there wasnt one
    if (readyFrame == NULL)
        prepareNextFrame(time);
}

// Return a copy of the current readyFrame
WirelessEtherBasicFrame *WESingleQueue::getReadyFrame()
{
    return readyFrame;
}

// Initiates tx retry of readyFrame. Returns false if retry limit reached and
// frame has been discarded
bool WESingleQueue::initiateRetry(double time)
{
    assert(readyFrame != NULL);
    retry++;
    collided++;
    timeToSend = 0;
    contentionStartTime = 0;
    // If maximum retry is reached prepare next frame to tx
    if (retry > retryLimit)
    {
        prepareNextFrame(time);
        return false;
    }
    else
    {
        attempted++;
        // Increase contention window size and reset timeToSend
        // Modify frame to reflect re-tx
        if (CW < CWmax)
            CW = (CW * 2) + 1;
        readyFrame->getFrameControl().retry = true;
    }
    return true;
}

// Retrieve access time for frame
double WESingleQueue::getTimeToSend()
{
    // Shouldnt call this if there is no readyFrame
    assert(readyFrame != NULL);
    // Calculate access time if it hasnt been calculated
    if (timeToSend == 0)
        timeToSend = calculateTimeToSend();
    return timeToSend;
}

// Function to notify when tx channel is busy to determine
// the remaining access time
void WESingleQueue::contentionInterrupted(double time)
{
    assert(timeToSend > 0);
    assert(contentionStartTime > 0);
    double timeElapsed = time - contentionStartTime;
    contentionStartTime = 0;
    // Deduct the number of slots elapsed after DIFS
    if (timeElapsed > this->DIFS)
    {
        int slotsElapsed = (int) ((timeElapsed - this->DIFS) / slotTime);
        backoffSlots -= slotsElapsed;
        timeToSend -= (slotsElapsed * slotTime);
    }
}

void WESingleQueue::startingContention(double time)
{
    contentionStartTime = time;
}


// Calculate the time to send the current readyFrame
double WESingleQueue::calculateTimeToSend()
{
    assert((CW >= CWmin) && (CW <= CWmax));
    backoffSlots = intuniform(0, CW);
    return (backoffSlots * slotTime + this->DIFS);
}

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
 * QoS queue for regular 802.11 implementations
 */
#include <iomanip>
#include <cqueue.h>
#include <assert.h>
#include "WEQoSQueue.h"
#include "wirelessethernet.h"
#include "WirelessEtherFrame_m.h"
#include "cTimerMessageCB.h"

Register_Class(WEQoSQueue);

WEQoSQueue::WEQoSQueue()
{
    slotTime = SLOTTIME;
    maxQueueSize = -1;          // infinite

    readyQSending = false;
    contentionStart = false;
    servedQ = -1;
    samplingStart = 0;

    busyStartTime = 0;
    idleStartTime = 0;

    acQ[AC_BK].AIFS = 7 * SLOTTIME + SIFS;
    acQ[AC_BE].AIFS = 7 * SLOTTIME + SIFS;
    acQ[AC_VI].AIFS = 4 * SLOTTIME + SIFS;
    acQ[AC_VO].AIFS = 2 * SLOTTIME + SIFS;

    acQ[AC_BK].CWmin = 63;
    acQ[AC_BE].CWmin = 63;
    acQ[AC_VI].CWmin = 31;
    acQ[AC_VO].CWmin = 15;

    acQ[AC_BK].CWmax = 1023;
    acQ[AC_BE].CWmax = 1023;
    acQ[AC_VI].CWmax = 127;
    acQ[AC_VO].CWmax = 63;

    acQ[AC_BK].retryLimit = 5;
    acQ[AC_BE].retryLimit = 5;
    acQ[AC_VI].retryLimit = 3;
    acQ[AC_VO].retryLimit = 3;

    for (int i = 0; i < 4; i++)
    {
        acQ[i].readyFrame = NULL;
        acQ[i].collided = 0;
        acQ[i].attempted = 0;
        acQ[i].rtCollided = 0;
        acQ[i].rtAttempted = 0;
        acQ[i].oRtCollided = 0;
        acQ[i].oRtAttempted = 0;
        acQ[i].avgFrameSize = new cStdDev("avgFrameSize");
        acQ[i].frameArrivalTime = new cStdDev("frameArrivalTime");
        acQ[i].accessTime = new cStdDev("accessTime");
        acQ[i].prevArrivalTime = 0;
        acQ[i].accessTimeStart = 0;
        acQ[i].avgFrameArrivalTime = 0;
        acQ[i].avgAccessTime = 0;
        acQ[i].bufferChecks = 0;
        acQ[i].bufferAvail = 0;
        acQ[i].lambda = 1;
        acQ[i].busySlots = 0;
        acQ[i].idleSlots = 0;
        resetParamsForQ(i);
    }
}

WEQoSQueue::~WEQoSQueue()
{
    reset();
    for (int i = 0; i < 4; i++)
    {
        delete acQ[i].avgFrameSize;
        delete acQ[i].frameArrivalTime;
        delete acQ[i].accessTime;
    }
}

void WEQoSQueue::initialise(WirelessEtherModule *module)
{
    mod = module;
}

void WEQoSQueue::setMaxQueueSize(unsigned int size)
{
    maxQueueSize = size;
}

void WEQoSQueue::setRetryLimit(unsigned int limit)
{
}

// get the number of retries
unsigned int WEQoSQueue::getRetry()
{
    assert(servedQ > 0);
    return acQ[servedQ].retry;
}

// get the CW size
unsigned int WEQoSQueue::getCW()
{
    assert(servedQ > 0);
    return acQ[servedQ].CW;
}

// get the number of backoffSlots
unsigned int WEQoSQueue::getBackoffSlots()
{
    assert(servedQ > 0);
    return acQ[servedQ].backoffSlots;
}

int WEQoSQueue::getQueueType()
{
    assert(servedQ >= 0);
    return servedQ;
}

int WEQoSQueue::getAttempted(int queue)
{
    return acQ[queue].attempted;
}

int WEQoSQueue::getCollided(int queue)
{
    return acQ[queue].collided;
}

double WEQoSQueue::getProbTxInSlot(int queue, double time, WirelessEtherModule *mod)
{
    if (acQ[queue].oRtAttempted != 0)
    {
        double pi = (double) acQ[queue].oRtCollided / (double) acQ[queue].oRtAttempted;
        // double pi = (double)(acQ[queue].collided+acQ[queue].busySlots)/(double)(acQ[queue].attempted+acQ[queue].idleSlots+acQ[queue].busySlots);
        double numerator = 2 * (1 - 2 * pi) * (1 - pi);
        double denominator = (1-2*pi)*(acQ[queue].CWmin+1) + pi*acQ[queue].CWmin*(1-pow(2*pi,(int)(acQ[queue].retryLimit-1)));
        double lambdaFactor = (1 - pi) * (1 / acQ[queue].lambda - 1);

        return (1 / (denominator / numerator + lambdaFactor));
    }
    return 0;
}

double WEQoSQueue::getLambda(int queue)
{
    return acQ[queue].lambda;
}

int WEQoSQueue::getAvgFrameSize(int queue)
{
    return (int) acQ[queue].avgFrameSize->mean();
}


// get the queue size, including the frame in readyFrame
int WEQoSQueue::size()
{
    int totalSize = 0;
    // Add up the sizes of each queue
    for (int i = 0; i < 4; i++)
    {
        totalSize += sizeOfQ(i);
    }

    return totalSize;
}

// Reset the queue, by resetting the params for each queue and emptying
// their contents
void WEQoSQueue::reset()
{
    readyQSending = false;
    contentionStart = false;
    servedQ = -1;
    acQToRetry.clear();

    for (int i = 0; i < 4; i++)
    {
        resetParamsForQ(i);
        if (acQ[i].readyFrame != NULL)
        {
            delete acQ[i].readyFrame;
            acQ[i].readyFrame = NULL;
        }
        while (!acQ[i].queue.empty())
            delete acQ[i].queue.pop();
    }
}

// Remove current readyFrame, reset parameters of served queue, and
// find the next queue to be serviced
void WEQoSQueue::prepareNextFrame(double time)
{
    // If its the end of sending the current readyFrame, retry
    // internally collided frames
    if (readyQSending)
    {
        // Retry internally collided retryFrames
        while (!acQToRetry.empty())
        {
            initiateRetryForQ(time, acQToRetry.front());
            acQToRetry.pop_front();
        }
        readyQSending = false;
    }
    prepareNextFrameForQ(time, servedQ);
    // Find the next queue to service
    servedQ = resolveCollisionsWithQ(soonestToSendQ());
    if (contentionStart && (servedQ >= 0))
    {
        assert(mod->backoffTimer->isScheduled());
        mod->reschedule(mod->backoffTimer, acQ[servedQ].contentionStartTime + acQ[servedQ].timeToSend);
    }
}

// Insert frame into the queue, taking into account the access category
void WEQoSQueue::insertFrame(WirelessEtherBasicFrame *frame, double time)
{
    // Insert Frame into queue if limit has not been reached
    if ((maxQueueSize < 0) || (acQ[frame->getAppType()].queue.length() < maxQueueSize))
    {
        acQ[frame->getAppType()].queue.insert(static_cast<cObject *>(frame));
    }
    // Remove oldest entry and insert new entry
    else
    {
        delete acQ[frame->getAppType()].queue.pop();
        acQ[frame->getAppType()].queue.insert(static_cast<cObject *>(frame));
    }
    // Prepare a readyFrame for the queue if there isnt one
    if (acQ[frame->getAppType()].readyFrame == NULL)
    {
        prepareNextFrameForQ(time, frame->getAppType());
        // If the serviced queue is not already sending, determine the
        // new queue to be serviced
        if (!readyQSending)
        {
            // Update the next queue to be serviced
            servedQ = resolveCollisionsWithQ(soonestToSendQ());
            if (contentionStart && (servedQ >= 0))
            {
                assert(mod->backoffTimer->isScheduled());
                mod->reschedule(mod->backoffTimer, acQ[servedQ].contentionStartTime + acQ[servedQ].timeToSend);
            }
        }
    }
}

// Return a copy of the readyFrame of the serviced queue
WirelessEtherBasicFrame *WEQoSQueue::getReadyFrame()
{
    // Return NULL if no queue serviced
    if (servedQ < 0)
        return NULL;
    else
        return acQ[servedQ].readyFrame;
}


// Initiates tx retry of the serviced queue's readyFrame.
// Returns false if retry limit reached and the frame is discarded
bool WEQoSQueue::initiateRetry(double time)
{
    assert(acQ[servedQ].readyFrame != NULL);
    assert(readyQSending);

    readyQSending = false;
    bool status = initiateRetryForQ(time, servedQ);

    // Retry internally collided retryFrames
    while (!acQToRetry.empty())
    {
        initiateRetryForQ(time, acQToRetry.front());
        acQToRetry.pop_front();
    }

    // Update the next queue to be serviced
    servedQ = resolveCollisionsWithQ(soonestToSendQ());
    return status;
}

// Retrieve access time for frame
double WEQoSQueue::getTimeToSend()
{
    assert(servedQ > 0);
    return acQ[servedQ].timeToSend;
}

// Function to notify when tx channel is busy to determine
// the remaining access time
void WEQoSQueue::contentionInterrupted(double time)
{
    double timeElapsed;
    // No longer contending
    contentionStart = false;

    // Update each readyFrame access time and backoff slots
    for (int i = 0; i < 4; i++)
    {
        // Check if the queue has a ready frame
        if (acQ[i].readyFrame != NULL)
        {
            assert(acQ[i].timeToSend > 0);
            timeElapsed = time - acQ[i].contentionStartTime;
            // Deduct the number of slots elapsed after AIFS
            if (timeElapsed > acQ[i].AIFS)
            {
                int slotsElapsed = (int) ((timeElapsed - acQ[i].AIFS) / slotTime);
                acQ[i].backoffSlots -= slotsElapsed;
                acQ[i].timeToSend -= (slotsElapsed * slotTime);

                // Check if remaining slot is close enough to be considered a slot
                double remSlot = (timeElapsed - acQ[i].AIFS) - (slotsElapsed * slotTime);
                assert(remSlot < slotTime);
                if ((slotTime - remSlot) < PRECERROR)
                {
                    acQ[i].backoffSlots -= 1;
                    acQ[i].timeToSend -= slotTime;
                }
            }
        }
    }
    servedQ = resolveCollisionsWithQ(soonestToSendQ());
}

// Called when contention between queues begin
void WEQoSQueue::startingContention(double time)
{
    contentionStart = true;

    // Keep track of each queues contention start time
    for (int i = 0; i < 4; i++)
    {
        // Check if the queue has a ready frame
        if (acQ[i].readyFrame != NULL)
        {
            acQ[i].contentionStartTime = time;
        }
    }
}

void WEQoSQueue::startIdleReceive(double time)
{
    for (int i = 0; i < 4; i++)
    {
        acQ[i].receiveIdleStart = time;
    }
}

void WEQoSQueue::endIdleReceive(double time)
{
    for (int i = 0; i < 4; i++)
    {
        acQ[i].receiveIdleTime += time - acQ[i].receiveIdleStart;
    }
}

void WEQoSQueue::startIdleCount(double time)
{
    idleStartTime = time;
}

void WEQoSQueue::endIdleCount(double time)
{
    for (int i = 0; i < 4; i++)
        acQ[i].idleSlots += (time - idleStartTime) / SLOTTIME;
}

void WEQoSQueue::startBusyCount(double time)
{
    busyStartTime = time;
}

void WEQoSQueue::endBusyCount(double time)
{
    for (int i = 0; i < 4; i++)
        acQ[i].busySlots += (time - busyStartTime) / SLOTTIME;
}

// Called when readyFrame is being sent (ie wins contention) to prevent
// it form being removed before send completion
void WEQoSQueue::sendingReadyFrame(double time)
{
    contentionStart = false;
    readyQSending = true;
    double timeElapsed;

    // Update time elapsed for each readyFrame in all queues except the servedQ
    for (int i = 0; i < 4; i++)
    {
        // Check if the queue has a ready frame
        if ((i != servedQ) && (acQ[i].readyFrame != NULL))
        {
            assert(acQ[i].timeToSend > 0);
            timeElapsed = time - acQ[i].contentionStartTime;
            // Deduct the number of slots elapsed after AIFS
            if (timeElapsed > acQ[i].AIFS)
            {
                int slotsElapsed = (int) ((timeElapsed - acQ[i].AIFS) / slotTime);
                acQ[i].backoffSlots -= slotsElapsed;
                acQ[i].timeToSend -= (slotsElapsed * slotTime);

                // Check if remaining slot is close enough to be considered a slot
                double remSlot = (timeElapsed - acQ[i].AIFS) - (slotsElapsed * slotTime);
                assert(remSlot < slotTime);
                if ((slotTime - remSlot) < PRECERROR)
                {
                    acQ[i].backoffSlots -= 1;
                    acQ[i].timeToSend -= slotTime;
                }
            }
        }
    }
}

// Calculate the size of a specified AC queue, including the
// ready frame
int WEQoSQueue::sizeOfQ(int AC)
{
    int length = acQ[AC].queue.length();
    if (acQ[AC].readyFrame != NULL)
        length++;
    return length;
}

// Reset the parameters for a queue, without emptying the queue
// contents
void WEQoSQueue::resetParamsForQ(int AC)
{
    acQ[AC].CW = acQ[AC].CWmin;
    acQ[AC].backoffSlots = acQ[AC].retry = 0;
    acQ[AC].contentionStartTime = acQ[AC].timeToSend = 0;
}

// Remove current readyFrame for the specified queue and
// reset its parameters
void WEQoSQueue::prepareNextFrameForQ(double time, int AC)
{
    assert(AC < 4);

    // Reset parameters
    resetParamsForQ(AC);

    // Remove readyFrame
    if (acQ[AC].readyFrame != NULL)
    {
        acQ[AC].avgFrameSize->collect(acQ[AC].readyFrame->length());
        delete acQ[AC].readyFrame;
        acQ[AC].readyFrame = NULL;
        if ((time - samplingStart) >= 1)
        {
            acQ[AC].avgFrameArrivalTime = acQ[AC].frameArrivalTime->mean();
            acQ[AC].avgAccessTime = acQ[AC].accessTime->mean();
            acQ[AC].frameArrivalTime->clearResult();
            acQ[AC].accessTime->clearResult();
            for (int i = 0; i < 4; i++)
            {
                acQ[i].oRtAttempted = (int) acQ[i].rtAttempted / (time - samplingStart);
                acQ[i].oRtCollided = (int) acQ[i].rtCollided / (time - samplingStart);
                acQ[i].rtAttempted = 0;
                acQ[i].rtCollided = 0;
            }

            if (acQ[AC].bufferChecks <= acQ[AC].bufferAvail)
                acQ[AC].lambda = 1;
            else if (acQ[AC].bufferChecks == 0)
                acQ[AC].lambda = 0;
            else
                acQ[AC].lambda = acQ[AC].bufferAvail / acQ[AC].bufferChecks;
            acQ[AC].bufferChecks = 0;
            acQ[AC].bufferAvail = 0;
            acQ[AC].lastSampledTime = time;
            samplingStart = time;
        }
        acQ[AC].accessTime->collect(time - acQ[AC].accessTimeStart);
        acQ[AC].prevArrivalTime = time;
        acQ[AC].receiveIdleTime = 0;
    }
    // Prepare next readyFrame and calculate its new access time
    if (!acQ[AC].queue.empty())
    {
        acQ[AC].frameArrivalTime->collect(time - acQ[AC].prevArrivalTime);
        int slotsElapsed = (int) ((time - acQ[AC].prevArrivalTime - acQ[AC].receiveIdleTime) / slotTime);
        if ((double) slotsElapsed < (time - acQ[AC].prevArrivalTime - acQ[AC].receiveIdleTime) / slotTime)
            slotsElapsed++;
        acQ[AC].bufferChecks += slotsElapsed;
        if ((int) ((time - acQ[AC].prevArrivalTime - acQ[AC].receiveIdleTime) / slotTime) == 0)
            acQ[AC].bufferChecks++;
        acQ[AC].bufferAvail++;
        acQ[AC].accessTimeStart = time;
        acQ[AC].attempted++;
        acQ[AC].rtAttempted++;
        acQ[AC].readyFrame = check_and_cast<WirelessEtherBasicFrame *>(acQ[AC].queue.pop());
        acQ[AC].timeToSend = calculateTimeToSendForQ(AC);
        // Set the contention start time just in case frame is
        // prepared during the contention period
        acQ[AC].contentionStartTime = time;
    }
}

// Perform retry procedure for the specified queue
bool WEQoSQueue::initiateRetryForQ(double time, int AC)
{
    assert(acQ[AC].readyFrame != NULL);
    int status = true;
    acQ[AC].retry++;
    acQ[AC].collided++;
    acQ[AC].rtCollided++;
    // Prepare the next frame if limit is reached or the frame is a broadcast
    if ((acQ[AC].retry > acQ[AC].retryLimit) || frameIsBroadcast(AC))
    {
        prepareNextFrameForQ(time, AC);
        status = false;
    }
    else
    {
        acQ[AC].attempted++;
        acQ[AC].rtAttempted++;
        // Increase contention window size and find new timeToSend
        // Modify frame to reflect re-tx
        if (acQ[AC].CW < acQ[AC].CWmax)
            acQ[AC].CW = (acQ[AC].CW * 2) + 1;
        acQ[AC].timeToSend = calculateTimeToSendForQ(AC);
        acQ[AC].readyFrame->getFrameControl().retry = true;
        // Set the contention start time just in case frame is
        // prepared during the contention period
        acQ[AC].contentionStartTime = time;
    }
    return status;
}

// Check if readyFrame for the specified queue is a broadcast frame
bool WEQoSQueue::frameIsBroadcast(int AC)
{
    assert(acQ[AC].readyFrame != NULL);
    if (acQ[AC].readyFrame->getAddress1() == MACAddress6(WE_BROADCAST_ADDRESS))
    {
        return true;
    }
    return false;
}

// Calculate the access time of a frame for the specified AC queue
double WEQoSQueue::calculateTimeToSendForQ(int AC)
{
    assert(AC < 4);
    assert((acQ[AC].CW >= acQ[AC].CWmin) && (acQ[AC].CW <= acQ[AC].CWmax));
    acQ[AC].backoffSlots = intuniform(0, acQ[AC].CW);
    return (acQ[AC].backoffSlots * slotTime + acQ[AC].AIFS);
}

// Find the queue with the smallest access time
int WEQoSQueue::soonestToSendQ()
{
    int soonestTSQ = -1;
    double soonestTime = 99999;

    for (int i = 0; i < 4; i++)
    {
        if (acQ[i].readyFrame != NULL)
        {
            // something wrong if there is ready frame but 0 timeToSend
            assert(acQ[i].timeToSend);
            if (contentionStart)
            {
                if ((acQ[i].timeToSend + acQ[i].contentionStartTime) < soonestTime)
                {
                    soonestTime = acQ[i].timeToSend + acQ[i].contentionStartTime;
                    soonestTSQ = i;
                }
            }
            else
            {
                if (acQ[i].timeToSend <= soonestTime)
                {
                    soonestTime = acQ[i].timeToSend;
                    soonestTSQ = i;
                }
            }
        }
    }
    return soonestTSQ;
}

// Assuming AC has the smallest timeToSend, returns resloved queue to send next
int WEQoSQueue::resolveCollisionsWithQ(int AC)
{
    double probSameSlot;
    if (AC < 0)
        return AC;
    assert(acQ[AC].readyFrame != NULL);
    int resolvedReadyQ = AC;
    acQToRetry.clear();

    // Resolve with higher priority queues
    for (int i = 3; i > AC; i--)
    {
        // Check if the queue has a readyFrame
        if (acQ[i].readyFrame != NULL)
        {
            double diff;
            // Find the absolute difference in timeToSend
            if (contentionStart)
            {
                diff = (acQ[i].timeToSend + acQ[i].contentionStartTime) - (acQ[AC].timeToSend + acQ[AC].contentionStartTime);
            }
            else
            {
                diff = acQ[i].timeToSend - acQ[AC].timeToSend;
            }
            // probSameSlot=1-diff/SLOTTIME;

            if (fabs(diff) < PRECERROR)
                // if( (diff<SLOTTIME) && (uniform(0,1)<probSameSlot) )
                diff = 0;

            if (diff < 0)
                assert(false);
            if (diff == 0)
            {
                if (resolvedReadyQ < i)
                {
                    acQToRetry.push_back(resolvedReadyQ);
                    resolvedReadyQ = i;
                }
                else
                {
                    acQToRetry.push_back(i);
                }
            }
        }
    }

    // Resolve with lower priority queues
    for (int i = 0; i < AC; i++)
    {
        // Check if the queue has a readyFrame
        if (acQ[i].readyFrame != NULL)
        {
            double diff;
            // Find the absolute difference in timeToSend
            if (contentionStart)
            {
                diff = (acQ[i].timeToSend + acQ[i].contentionStartTime) - (acQ[AC].timeToSend + acQ[AC].contentionStartTime);
            }
            else
            {
                diff = acQ[i].timeToSend - acQ[AC].timeToSend;
            }
            // probSameSlot=1-diff/SLOTTIME;

            if (fabs(diff) < PRECERROR)
                // if( (diff<SLOTTIME) && (uniform(0,1)<probSameSlot) )
                diff = 0;

            if (diff < 0)
                assert(false);
            if (diff == 0)
                acQToRetry.push_back(i);
        }
    }
    return resolvedReadyQ;
}

/***************************************************************************
                          RTPAVProfilePayload10Receiver.cc  -  description
                             -------------------
    begin                : Fri Sep 20 2002
    copyright            : (C) 2002 by Matthias Oppitz
    email                : matthias.oppitz@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*! \file RTPAVProfilePayload10Receiver.cc

*/

#include <audiofile.h>
#include <string.h>


#include "types.h"
#include "RTPAVProfilePayload10Receiver.h"



Define_Module_Like(RTPAVProfilePayload10Receiver, RTPPayloadReceiver);    

void RTPAVProfilePayload10Receiver::initialize() {
	_sampleWidth = 16;
	_samplingRate = 44100;
	_numberOfChannels = 2;
	RTPAVProfileSampleBasedAudioReceiver::initialize();
};


void RTPAVProfilePayload10Receiver::insertSilence(simtime_t duration) {
	// one sample is 32 bit (2 channels, both 16 bit)
	u_int32 *data;
	int numberOfSamples = (int)(duration / ((float)_samplingRate));
	data = new u_int32[numberOfSamples];
	bzero(data, numberOfSamples * 4);
	afWriteFrames(_audioFile, AF_DEFAULT_TRACK, data, numberOfSamples);
};
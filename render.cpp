/*
 * render.cpp
 *
 *  Created on: Apr 6, 2016
 *      Author: Alistair Barker
 */


#include <BeagleRT.h>
#include <cmath>
#include <Utilities.h>
#include <Midi.h>
#include "Synth.h"
#include "Scope.h"
#include "MoogLadderFilter.h"

int pot1 = 0;

float freq;
float inc = 1.00001;

BLITOscillator osc;
MoogLadderFilter filter;

Scope scope;


bool setup(BeagleRTContext *context, void *userData)
{
		scope.setup(context->audioSampleRate);

	// set frequency
	freq = 200;

			filter.setCoefficients(filterTypeLowPass, 16000, 0.5, context->audioSampleRate);

	osc.setFrequency(freq, context->audioSampleRate, oscTypeSaw);

	return true;
}


void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {

		float freqIn = map(analogReadFrame(context, n/2, pot1), 0, 0.8, 200, 20000);



		// rt_printf("Freq: %f\n", freqIn);
		 filter.setCoefficients(filterTypeLowPass, freqIn, 0.5, context->audioSampleRate);
		// freq *= inc;

		// if (freq >= 18000 || freq <= 20 )
			// inc = 1/inc;

		float out = osc.tick();

		out = filter.processSample(out);

		for(unsigned int channel = 0; channel < context->audioChannels; channel++) {
			audioWriteFrame(context, n, channel, out);
		}
	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().


void cleanup(BeagleRTContext *context, void *userData)
{
}

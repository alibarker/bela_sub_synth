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
 #include "Envelope.h"

Envelope* env;

int pot1 = 0;
int button1 = P9_12;
int pot2 = 1;


float prevFreqIn;
float inc = 1.00001;

int button1PrevState;

BLITOscillator osc;
MoogLadderFilter filter;

// Scope scope;


bool setup(BeagleRTContext *context, void *userData)
{
		// scope.setup(context->audioSampleRate);

	// set frequency

			filter.setCoefficients(filterTypeLowPass, 16000, 0.5, context->audioSampleRate);

	osc.setFrequency(200, context->audioSampleRate, oscTypeSaw);

	env = new Envelope(context->audioSampleRate);

	pinModeFrame(context, 0, button1, INPUT);
	return true;
}


void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {

		float freqIn = map(analogReadFrame(context, n/2, pot1), 0, 0.85, 200, 15000);
		float resIn = map(analogReadFrame(context, n/2, pot2), 0, 0.43, 0, 1.5);

		filter.setCoefficients(filterTypeLowPass, 0.5*(freqIn + prevFreqIn), 1, context->audioSampleRate);

		prevFreqIn = freqIn;


		int button1state = digitalReadFrame(context, n, button1);

		if (button1state == 0 && button1PrevState == 1)
		{
			env->startNote();
		} else if (button1state == 1 && button1PrevState == 0)
		{
			env->releaseNote();
		}
		// rt_printf("button1state:\t%d\n", button1state);
		button1PrevState = button1state;

		rt_printf("Freq: %f\n", freqIn);
		// // freq *= inc;

		// // if (freq >= 18000 || freq <= 20 )
		// 	// inc = 1/inc;

		float out = osc.tick();
		float envAmp = env->tick();

		// rt_printf("amp: %f\n", envAmp);

		out = filter.processSample(out) * envAmp * 0.125;

		for(unsigned int channel = 0; channel < context->audioChannels; channel++) {
			audioWriteFrame(context, n, channel, out);
		}
	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().


void cleanup(BeagleRTContext *context, void *userData)
{
	delete env;
}

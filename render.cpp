/*
 * render.cpp
 *
 *  Created on: Oct 24, 2014
 *      Author: parallels
 */


#include <BeagleRT.h>
#include <cmath>
#include <Utilities.h>
 #include <Midi.h>

// setup() is called once before the audio rendering starts.
// Use it to perform any initialisation and allocation which is dependent
// on the period size or sample rate.
//
// userData holds an opaque pointer to a data structure that was passed
// in from the call to initAudio().
//
// Return true on success; returning false halts the program.

bool setup(BeagleRTContext *context, void *userData)
{
	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.

void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		float out = 0.8f * sinf(gPhase);
		gPhase += 2.0 * M_PI * gFrequency * gInverseSampleRate;
		if(gPhase > 2.0 * M_PI)
			gPhase -= 2.0 * M_PI;

		for(unsigned int channel = 0; channel < context->audioChannels; channel++) {
			// Two equivalent ways to write this code

			// The long way, using the buffers directly:
			// context->audioOut[n * context->audioChannels + channel] = out;

			// Or using the macros:
			audioWriteFrame(context, n, channel, out);
		}
	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().

void cleanup(BeagleRTContext *context, void *userData)
{

}

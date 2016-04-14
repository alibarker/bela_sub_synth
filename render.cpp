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
#include "SensorLowPass.h"

// Pin Numbers
int octavePin = 0;
int tempoPin = 1;
int attackPin = 2;
int decayPin = 3;
int filterAttackPin = 4;
int filterDecayPin = 5;
int filterCutoffPin = 6;
int filterQPin = 7;

int startButton = P9_12;
int stopButton = P9_14;
int ledPin = P9_16;

// I/O state variables
int ledOnCount;
int ledPinStatus = 0;
float prevFreqIn;
float prevResIn;
int startButtonPrevState = 1;
int stopButtonPrevState = 1;
float prevFilterAttacks[4];

// input filters

SensorLowPass* filterFreqSmoother;
SensorLowPass* filterResSmoother;

// rhythm state variables
bool isRunning = false;

float beatPhase;
float prevBeatPhase;
int beatCount = 0;
float swingPercent;
enum Subdivision
{
	subDivisionQuarter = 0,
	subDivisionQuavers,
	subDivisionTriplets,
	subDivisionSemiquavers
};
Subdivision sub;


// pitch state variables
int globalPitch = 48;
int currentOctave = 0;
int numOctaves = 0;
int currentDirection = 1;

enum
{
	chordSingleNote = 0,
	chordMajor,
	chordMinor,
	chordDomSeventh,
};
int numNotesInChord[4];
int currentChordType;
int* chordNotes[4];


// global state variables

float Fs;
int audioFramesPerAnalogFrame;

// components
BLITOscillator osc;
MoogLadderFilter filter;
Envelope* env;
Envelope* filterEnv;


void startNextNote()
{
	// find out pitch

	int* currentChord = chordNotes[currentChordType];
	int currentNote = currentChord[beatCount - 1];
	int pitchNumber = globalPitch + currentNote + 12 * currentOctave;


	// set oscillator

	float freq = 440 * pow(2, (pitchNumber - 69)/12.0);

	osc.setFrequency(freq, Fs, oscTypeSaw);

	// trigger envelope
	env->startNote();
	filterEnv->startNote();


	rt_printf("Beat: %d\tNote: %d\t Pitch: %d\t Freq: %f\n", beatCount, currentNote, pitchNumber, freq);


	beatCount += currentDirection;
	if (beatCount > numNotesInChord[currentChordType] || beatCount < 1)
	{
		beatCount = ((beatCount + numNotesInChord[currentChordType] - 1) % numNotesInChord[currentChordType]) + 1;

		currentOctave += currentDirection;
		if (currentOctave >= numOctaves ){
			currentDirection = -1;
		}
		else if (currentOctave < 0)	{
			currentDirection = 1;
		}

	}

	


}

void initChords()
{
	numNotesInChord[chordSingleNote] = 1;
	numNotesInChord[chordMajor] = 3;
	numNotesInChord[chordMinor] = 3;
	numNotesInChord[chordDomSeventh] = 4;

	int chordNotesSingleNote[1] = {0};
	int chordNotesMajor[3] =  {0, 4, 7};
	int chordNotesMinor[3] =  {0, 3, 7};
	int chordNotesDomSeventh[4] = {0, 4, 7, 10};

	chordNotes[chordSingleNote] = (int *)malloc(1 * sizeof(int));
	memcpy(chordNotes[chordSingleNote], chordNotesSingleNote, 1 * sizeof(int));

	chordNotes[chordMajor] = (int *)malloc(numNotesInChord[chordMajor] * sizeof(int));;
	memcpy(chordNotes[chordMajor], chordNotesMajor, numNotesInChord[chordMajor] * sizeof(int));

	chordNotes[chordMinor] = (int *)malloc(numNotesInChord[chordMinor] * sizeof(int));;
	memcpy(chordNotes[chordMinor], chordNotesMinor, numNotesInChord[chordMinor] * sizeof(int));
	
	chordNotes[chordDomSeventh] = (int *)malloc(numNotesInChord[chordDomSeventh] * sizeof(int));;
	memcpy(chordNotes[chordDomSeventh], chordNotesDomSeventh, numNotesInChord[chordDomSeventh] * sizeof(int));
}

bool setup(BeagleRTContext *context, void *userData)
{
		// scope.setup(context->audioSampleRate);

	currentChordType = chordMinor;

	Fs = context->audioSampleRate;

	audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	rt_printf("audioFramesPerAnalogFrame: %d\n", audioFramesPerAnalogFrame);

	osc.setFrequency(200, context->audioSampleRate, oscTypeSaw);

	env = new Envelope(context->audioSampleRate);
	filterEnv = new Envelope(context->audioSampleRate);
	filterFreqSmoother = new SensorLowPass(4);
	filterResSmoother = new SensorLowPass(4);



	pinModeFrame(context, 0, startButton, INPUT);
	pinModeFrame(context, 0, stopButton, INPUT);

	pinModeFrame(context, 0, ledPin, OUTPUT);

	swingPercent = 0.6;
	sub = subDivisionSemiquavers;

	initChords();


	return true;
}


void render(BeagleRTContext *context, void *userData)
{

	for(unsigned int n = 0; n < context->audioFrames; n++) {


		int startButtonState = digitalReadFrame(context, n, startButton);
		int stopButtonState = digitalReadFrame(context, n, stopButton);
		
		if (startButtonState == 0 && startButtonPrevState == 1)
		{
			isRunning = true;
			beatPhase = 1.0;
			beatCount = 1;
			rt_printf("Start\n");
		}
		startButtonPrevState = startButtonState;

		if (stopButtonState == 0 && stopButtonPrevState == 1)
		{
			isRunning = false;
			rt_printf("Stop\n");
		}
		stopButtonPrevState = stopButtonState;

		if  (isRunning)
		{

			float beatsPerSample = map(analogReadFrame(context, n/2, tempoPin), 0, 0.85, 60, 180) / (60 * context->audioSampleRate);	
			numOctaves = (int) map(analogReadFrame(context, n/2, octavePin), 0, 0.85, 0, 6);
			// incrememnt beat and trigger notes if required
			beatPhase += beatsPerSample;

			if (sub == subDivisionQuavers)
			{
				if (beatPhase >= swingPercent && prevBeatPhase < swingPercent)
				{
					if (beatPhase >= swingPercent && prevBeatPhase < swingPercent)
						startNextNote();
				}
			} else if (sub == subDivisionTriplets)
			{
				if (beatPhase >= 1.0/3.0 && prevBeatPhase < 1.0/3.0)
				{
					startNextNote();
				} else if (beatPhase >= 2.0/3.0 && prevBeatPhase < 2.0/3.0)
				{
					startNextNote();
				}
			} else if (sub == subDivisionSemiquavers)
			{
				if  (beatPhase >= 1.0/4.0 && prevBeatPhase < 1.0/4.0)
				{
					startNextNote();

				} else if (beatPhase >= 2.0/4.0 && prevBeatPhase < 2.0/4.0)
				{
					startNextNote();

				} else if (beatPhase >= 3.0/4.0 && prevBeatPhase < 3.0/4.0)
				{
					startNextNote();
				}
			}

			prevBeatPhase = beatPhase;

			if (beatPhase >= 1.0)
			{
				beatPhase -= 1.0;
				ledOnCount = 0;
				ledPinStatus = 1;

				startNextNote();
			}

		
			// set led and turn off after 100ms
			// digitalWriteFrame(context, n, ledPin, ledPinStatus);
			if (ledPinStatus == 1)
			{
				ledOnCount++;
				if( ledOnCount >= 0.1 * context->digitalSampleRate){
					ledPinStatus = 0;
				}
			}

			// get oscillator output
			float out = osc.tick();

			float attack, decay, freqIn, resIn, filterAttack, filterDecay;

			// apply envelope
			if(!(n % audioFramesPerAnalogFrame)) {

				attack = map(analogReadFrame(context, n/2, attackPin), 0, 0.85, 0.01, 0.5);
				decay = map(analogReadFrame(context, n/2, decayPin), 0, 0.85, 0.01, 0.5);

				env->setParameters(attack, decay, 0, 0);



				// Set Filter coefficients

				filterAttack = map(analogReadFrame(context, n/2, filterAttackPin), 0, 0.85, 0.01, 0.5);
				filterDecay = map(analogReadFrame(context, n/2, filterDecayPin), 0, 0.85, 0.01, 0.5);

				filterEnv->setParameters(filterAttack, filterDecay, 0, 0);

				// rt_printf("EnvAmp: %f\n", filterEnvAmp);


				freqIn = map(analogReadFrame(context, n/2, filterCutoffPin), 0, 0.85, 20, 15000);
				resIn = map(analogReadFrame(context, n/2, filterQPin), 0, 0.85, 0, 1.2);

				freqIn = filterFreqSmoother->processSample(freqIn);
				resIn = filterResSmoother->processSample(resIn);

			}

			float envAmp = env->tick();
			float filterEnvAmp = filterEnv->tick();

			// freqIn = filterFreqSmoother->processSample(freqIn);
			// resIn = filterResSmoother->processSample(resIn);

			filter.setCoefficients(filterTypeLowPass, freqIn * (1 + filterEnvAmp), resIn, context->audioSampleRate);
			// filter.setCoefficients(filterTypeLowPass, freqIn, resIn, context->audioSampleRate);

			// rt_printf("Freq: \t%f, Res: \t%f, n: \t%d\n", freqIn * (1 + filterEnvAmp), resIn, n);

			// apply filter
			out = filter.processSample(out) * envAmp * 0.5;
			// out = out * envAmp * 0.5;

			// write to both channels
			for(unsigned int channel = 0; channel < context->audioChannels; channel++) {
				audioWriteFrame(context, n, channel, out);
			}
		}
	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().


void cleanup(BeagleRTContext *context, void *userData)
{
	delete env;
	delete filterEnv;
}

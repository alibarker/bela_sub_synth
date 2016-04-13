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
int globalPitch = 60;
int currentOctave = 0;
int numOctaves = 0;

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

// components
BLITOscillator osc;
MoogLadderFilter filter;
Envelope* env;
Envelope* filterEnv;


void startNextNote()
{
	// find out pitch
	beatCount++;
	if (beatCount > numNotesInChord[currentChordType])
		beatCount = 1;

	int* currentChord = chordNotes[currentChordType];
	int currentNote = currentChord[beatCount];
	int pitchNumber = globalPitch + currentNote + 12 * currentOctave;

	// set oscillator

	float freq = 440 * pow(2, (pitchNumber - 69)/12.0);

	osc.setFrequency(freq, Fs, oscTypeSaw);

	// trigger envelope
	env->startNote();
	filterEnv->startNote();

	rt_printf("Note: %d\t Pitch: %d\t Freq: %f\n", currentNote, pitchNumber, freq);

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

	currentChordType = chordSingleNote;

	Fs = context->audioSampleRate;


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

			// apply envelope

			float attack = map(analogReadFrame(context, n/2, attackPin), 0, 0.85, 0.01, 0.5);
			float decay = map(analogReadFrame(context, n/2, decayPin), 0, 0.85, 0.01, 0.5);

			env->setParameters(attack, decay, 0, 0);

			float envAmp = env->tick();


			// Set Filter coefficients

			float filterAttack = map(analogReadFrame(context, n/2, filterAttackPin), 0, 0.85, 0.01, 0.5);
			float filterDecay = map(analogReadFrame(context, n/2, filterDecayPin), 0, 0.85, 0.01, 0.5);

			filterEnv->setParameters(filterAttack, filterDecay, 0, 0);
			float filterEnvAmp = filterEnv->tick();

			// rt_printf("EnvAmp: %f\n", filterEnvAmp);


			float freqIn = map(analogReadFrame(context, n/2, filterCutoffPin), 0, 0.85, 20, 15000);
			float resIn = map(analogReadFrame(context, n/2, filterQPin), 0, 0.85, 0, 1.2);

			freqIn = filterFreqSmoother->processSample(freqIn);
			resIn = filterResSmoother->processSample(resIn);

			filter.setCoefficients(filterTypeLowPass, freqIn * (1 + filterEnvAmp), resIn, context->audioSampleRate);


			// apply filter
			out = filter.processSample(out) * envAmp * 0.5;

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

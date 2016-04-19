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
#include "MoogLadderFilter.h"
#include "Envelope.h"
#include "SensorLowPass.h"
#include <cstdlib>
 #include <cstring>
 #include <vector>
 #include "UdpClient.h"

// Pin Numbers
int octavePin = 0;
int tempoPin = 1;
int attackPin = 2;
int decayPin = 3;
int filterAttackPin = 4;
int filterDecayPin = 5;
int filterCutoffPin = 6;
int filterQPin = 7;

int holdPin = P9_16;
int startButton = P9_12;
int stopButton = P9_14;
int ledPin = P8_09;

// I/O state variables
int ledOnCount;
int ledPinStatus = 0;
float prevFreqIn;
float prevResIn;
int startButtonPrevState = 1;
int stopButtonPrevState = 1;
float prevFilterAttacks[4];
int holdButtonPrevState = 1;

int holdModeOn = 0;

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

int numNotesHeld = 0;
bool notesHeld[128];
std::vector<int> notes;


// global state variables

float Fs;
int audioFramesPerAnalogFrame;
int midiPort = 0;
enum {kVelocity, kNoteOn, kNoteNumber};


// components
BLITOscillator osc;
MoogLadderFilter filter;
Envelope* env;
Envelope* filterEnv;
Midi midi;


void startNextNote()
{
	// find out pitch
	if (notes.size() > 0)
	{
		beatCount = ((beatCount + notes.size() - 1) % notes.size()) + 1;
      	std::vector<int>::iterator iter = notes.begin() + beatCount - 1;

		int currentNote = *iter;

		rt_printf("currentNote: %d\n",  currentNote);

		int pitchNumber = currentNote + 12 * currentOctave;


		// set oscillator

		float freq = 440 * pow(2, (pitchNumber - 69)/12.0);

		osc.setFrequency(freq, Fs, oscTypeSaw);

		// trigger envelope
		env->startNote();
		filterEnv->startNote();


		rt_printf("Beat: %d\tOctave: %d\tNote: %d\t Pitch: %d\t Freq: %f\n", beatCount, currentOctave, currentNote, pitchNumber, freq);

		beatCount += currentDirection;
		if (beatCount > notes.size() || beatCount < 1)
		{

			beatCount = ((beatCount + notes.size() - 1) % notes.size()) + 1;

			if (numOctaves > 0)
			{
				currentOctave += currentDirection;
				if (currentOctave > numOctaves)
				{
					currentOctave = numOctaves;
					currentDirection = -1;
				} else if (currentOctave < 0)
				{
					currentOctave = 0;
					currentDirection = + 1;
				}
			} else {
				currentOctave = 0;
			}

		}
	}
}

float gFreq, gVelocity, gPhaseIncrement, gIsNoteOn;

void midiMessageCallback(MidiChannelMessage message, void* arg){
	
	if(message.getType() == kmmNoteOn){

		if (message.getDataByte(1) > 0)
		{
			notesHeld[message.getDataByte(0)] = true;
			numNotesHeld++;

			notes.push_back(message.getDataByte(0));

		} else if(message.getDataByte(1) == 0 && holdModeOn == 0)
		{
			for (std::vector<int>::iterator i = notes.begin(); i != notes.end(); ++i)
			{

				if (*i == message.getDataByte(0))
				{
					notes.erase(i);
					break;
				}
			}
		}

	if(arg != NULL){
			for (std::vector<int>::iterator i = notes.begin(); i != notes.end(); ++i)
			{
				rt_printf("%d ", *i);
			}
			rt_printf("\n");
	}

		// gFreq = powf(2, (message.getDataByte(0)-69)/12.0f) * 440;
		// gVelocity = message.getDataByte(1);
		// gPhaseIncrement = 2 * M_PI * gFreq / Fs;
		// gIsNoteOn = gVelocity > 0;
		// rt_printf("v0:%f, ph: %6.5f, gVelocity: %d\n", gFreq, gPhaseIncrement, gVelocity);
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

	// MIDI

	midi.readFrom(midiPort);
	midi.writeTo(midiPort);
	midi.enableParser(true);
	midi.setParserCallback(midiMessageCallback, &midiPort);
	if(context->analogFrames == 0) {
		rt_printf("Error: this example needs the analog I/O to be enabled\n");
		return false;
	}

	for (int i = 0; i < 128; ++i)
	{
		notesHeld[i] = false;
	}

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
	pinModeFrame(context, 0, holdPin, INPUT);

	pinModeFrame(context, 0, ledPin, OUTPUT);

	swingPercent = 0.6;
	sub = subDivisionSemiquavers;

	initChords();


	return true;
}

void clearNotes()
{

}


void render(BeagleRTContext *context, void *userData)
{

	for(unsigned int n = 0; n < context->audioFrames; n++) {


		int startButtonState = digitalReadFrame(context, n, startButton);
		int stopButtonState = digitalReadFrame(context, n, stopButton);
		int holdButtonState = digitalReadFrame(context, n, holdPin);

		if (startButtonState == 0 && startButtonPrevState == 1)
		{
			isRunning = true;
			beatPhase = 1.0;
			beatCount = 1;
			printf("Start\n");

		}
		startButtonPrevState = startButtonState;

		if (stopButtonState == 0 && stopButtonPrevState == 1)
		{
			isRunning = false;
			rt_printf("Stop\n");
		}
		stopButtonPrevState = stopButtonState;

		if (holdButtonState == 0 && holdButtonPrevState == 1)
		{
			if (holdModeOn == 0)
			 {
			 	holdModeOn = 1;
			 } else if (holdModeOn == 1)
			 { 
			 	holdModeOn = 0;
			 	notes.clear();
			 }
			rt_printf("Hold state: %d\n", holdModeOn);
		}

		holdButtonPrevState = holdButtonState;

		digitalWriteFrame(context, n, ledPin, holdModeOn);

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


			freqIn = map(analogReadFrame(context, n/2, filterCutoffPin), 0, 1, 20, 15000);
			resIn = map(analogReadFrame(context, n/2, filterQPin), 0, 1, 0, 1);

			freqIn = filterFreqSmoother->processSample(freqIn);
			resIn = filterResSmoother->processSample(resIn);

		}
		float envAmp = env->tick();
		float filterEnvAmp = filterEnv->tick();
			
		if (freqIn!=prevFreqIn || resIn != prevResIn)
		{
			filter.setCoefficients(filterTypeLowPass, freqIn  * (1 + filterEnvAmp), resIn, context->audioSampleRate);
		}
		prevFreqIn = freqIn; prevResIn = resIn;



		// freqIn = filterFreqSmoother->processSample(freqIn);
		// resIn = filterResSmoother->processSample(resIn);

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

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().


void cleanup(BeagleRTContext *context, void *userData)
{
	delete env;
	delete filterEnv;
}

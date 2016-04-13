/*
 *   Envelope.h
 *
 *  Created on: Apr 6, 2016
 *      Author: Alistair Barker
 */

enum State {
	stateOff= 0,
	stateAttack,
	stateDecay,
	stateSustain,
	stateRelease
};


class Envelope
{
public:
	Envelope(float Fs)
	{

		state = stateOff;
		sampleRate = Fs;
		setParameters(0.1, 0.1, 0.8, 0.1);
		currentAmplitude = 0.0;

		rt_printf("Fs: %f\n", sampleRate);
	}
	~Envelope(){}

	float tick();

	void setParameters(float a, float d, float s, float r)
	{
		// attack is linear, decay/release is exponential
		attackInc = 1/(a*sampleRate);
		decayInc = pow(exp(-1/d), 1/sampleRate);
		sustain = s;
		releaseInc = pow(exp(-1/r), 1/sampleRate);

// rt_printf("sampleRate: %f\t attackInc: %f\t, decayInc: %f\t, sustain: %f\t, releaseInc: %f\n", sampleRate, attackInc, decayInc, sustain, releaseInc);

	}

	void startNote()
	{
		state = stateAttack;
		rt_printf("Note on\n");
	}
	void releaseNote()
	{
		state = stateRelease;
				rt_printf("Note off\n");
 
	}

	State getState()
	{
		return state;
	}
	
private:
	State state;

	double attackInc, decayInc, sustain, releaseInc;

	float currentAmplitude;

	float sampleRate;

};

float Envelope::tick()
{
	switch(state) {
		case stateOff:
			currentAmplitude = 0.0;
			break;

		case stateAttack:
			currentAmplitude += attackInc;
			if (currentAmplitude >= 1){
				state = stateDecay;
				rt_printf("Decay\n");
			}
			break;

		case stateDecay:
			currentAmplitude *= decayInc;
			if (currentAmplitude <= sustain){
				state = stateSustain;
				rt_printf("Sustain\n");
			}
			break;

		case stateRelease:
			currentAmplitude *= releaseInc;
			if (currentAmplitude <= 0.0001)
			{
				currentAmplitude = 0.0;
				state = stateOff;
				rt_printf("End\n");
			}
			break;

		case stateSustain:
			currentAmplitude = sustain;
			break;

		}

	return currentAmplitude;

};
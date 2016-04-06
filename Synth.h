/*
 * synth.g
 *
 *  Created on: Apr 6, 2016
 *      Author: Alistair Barker
 */

#include <cmath>

enum OscType {
		oscTypeIT = 0,
		oscTypeSaw ,
		oscTypeSquare
		};

class BLITOscillator {
public:

	BLITOscillator() 
	{
		phase = 0.0;
		isActive = false;
	}
	~BLITOscillator(){}

	float tick()
	{
		
		if (isActive)
		{			
			float den = sin(phase);
			float out;

			if (den == 0.0) {
				out = 1.0;
			} else		{
				out = sin(M * phase) / (M * den);
			}

	  		out += state - C2;
	 		state = out * 0.9;

			phase += inc;

			if (phase >=  M_PI)
				phase -=  M_PI;
		}

		return state;
	}

	void setFrequency(float frequency, float Fs, OscType t)
	{
		P = Fs / frequency;
		M = 2 * floor(P/2) + 1;
		C2 = 1/P;
 		inc =  M_PI/P;

		type = t;

		// rt_printf("P: %f\t M: %d\t inc: %f\t C2: %f\n", P, M, inc, C2);

		isActive = true;
	}

private:

	float sincm(float x)
	{
		float temp;
		float den = sin(M_PI * x / M);
		if (den == 0)
		{
			temp = 1.0;
		} else
		{
			temp = sin(M_PI * x) / (M * sin(M_PI * x / M));
		}

		return temp;

	}


	int M;
	float P;
	float phase;
	float inc;
	float C2;
	float state;
	bool isActive;
	OscType type;
};
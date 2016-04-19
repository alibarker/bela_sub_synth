/*
 * synth.g
 *
 *  Created on: Apr 6, 2016
 *      Author: Alistair Barker
 */

#include <cmath>
 #include <cfloat>

enum OscType {
		oscTypeSaw  = 0,
		oscTypeSquare,
		oscTypeSine
		};

class BLITOscillator {
public:

	BLITOscillator() 
	{
		phase = 0.0;
		isActive = false;
		lastBlitOut = 0.0;
		dcBlockerState = 0.0;
		previousOut = 0.0;
	}
	~BLITOscillator(){}

	float tick()
	{
		
		if (isActive)
		{	

			float den;
			float temp;
			float out;


			switch (type)	{	
			case oscTypeSaw:

				den = sin(phase);

				if (fabs(den) <= FLT_MIN) {
					temp = A;
				} else		{
					temp = sin(M * phase) / (P * den);
				}

		  		temp += state - C2;
		 		state = temp * 0.995;

				phase += inc;

				if (phase >=  M_PI)
					phase -=  M_PI;

				out = temp;

				break;

			case oscTypeSquare:

				temp = lastBlitOut;
				den = sin(phase);
				if (fabs(den) < FLT_MIN) {
					if (phase < 0.1f || phase > M_PI * 2 - 0.1f )
						lastBlitOut = A;
					else
						lastBlitOut = -A;
					
				} else	{
					lastBlitOut =  sin( M * phase );
    				lastBlitOut /= P * den;

				}
				lastBlitOut += temp;

				state = lastBlitOut - dcBlockerState + 0.999 * state;
  				dcBlockerState = lastBlitOut;

				phase += inc;

				if (phase >=  2 * M_PI)
					phase -=  2 * M_PI;
				
				out =  state;
				break;

			case oscTypeSine:
				state = sin(phase);

				phase += inc;

				if (phase >=  2 * M_PI)
					phase -=  M_PI;
				out = state;
				break;
			}

			return out;
		}

		return 0.0;
	}

	void setFrequency(float frequency, float Fs, OscType t)
	{

		type = t;

		switch(type) {
		case oscTypeSaw:
			P = Fs / frequency;
			M = 2 * floor(P/2) + 1;
			C2 = 1/P;
	 		inc =  M_PI/P;
	 		A = M/P;

	 		break;
	 	
	 	case oscTypeSquare:
	 		P = 0.5 * Fs / frequency;
	 		inc = M_PI/P;	
			M = 2 * (floor(P/2) + 1);
			A = M/P;
			break;


		case oscTypeSine:
			P  = Fs/frequency;
			inc = M_PI/P;
	 		break;

 		}

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
	float A;
	float phase;
	float inc;
	float C2;
	float state;
	bool isActive;
	float lastBlitOut;
	float dcBlockerState;
	float previousOut;
	OscType type;
};
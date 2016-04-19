/*
 *   MoodLadderFiler.cpp
 *
 *  Created on: Apr 6, 2016
 *      Author: Alistair Barker
 */

#include "MoogLadderFilter.h"


MoogLadderFilter::MoogLadderFilter()
{

	for (int i = 0; i < 5; i++)
	{
		state[i] = prevState[i] = 0.0;
	}

// oversampling filter cofficients, from MATLAB

resamplingCoefficients[0] = 0.00506031712484500;
resamplingCoefficients[1] = -3.25061549073449e-18;
resamplingCoefficients[2] = -0.0419428794313447;
resamplingCoefficients[3] = 1.32104344910468e-17;
resamplingCoefficients[4] = 0.288484826302638;
resamplingCoefficients[5] = 0.496795472007725;
resamplingCoefficients[6] = 0.288484826302638;
resamplingCoefficients[7] = 1.32104344910468e-17;
resamplingCoefficients[8] = -0.0419428794313447;
resamplingCoefficients[9] = -3.25061549073449e-18;
resamplingCoefficients[10] = 0.00506031712484485;

}


float MoogLadderFilter::processSample(float in)
{

	float in2[2] = {in, 0.0};
	float out[2] = {0.0, 0.0};

	antiImageFilter(in2[0]);
	antiImageFilter(in2[1]);

	for (int i = 0; i < 2; ++i)
	{
	 	float u = in2[i] - 4 * resonance * (prevState[4] - Gcomp*in2[i]);

	 	state[0] = tanh (u);
	 	state[1] = h0 * state[0] + h1 * prevState[0] + (1-g) * prevState[1];
	 	state[2] = h0 * state[1] + h1 * prevState[1] + (1-g) * prevState[2];
	 	state[3] = h0 * state[2] + h1 * prevState[2] + (1-g) * prevState[3];
	 	state[4] = h0 * state[3] + h1 * prevState[3] + (1-g) * prevState[4];

	 	out[i] = state[4];

	 	for (int i = 0; i < 5; i++)
		{
			prevState[i] = state[i];
		}	
	}

antiAliasFilter(out[0]);
antiAliasFilter(out[1]);

	return out[0];

}

void MoogLadderFilter::setCoefficients(int filterType, float cutoff, float r, float sampleRate)
{

	if (cutoff >= 15000)
		cutoff == 15000;

	if (r >= 1)
		r == 1;

	float sampleRate2 = sampleRate * 2;
	g = 2 * M_PI * cutoff / sampleRate2;
	h0 = g / 1.3;
	h1 = g * 0.3 / 1.3;

	resonance = r;
	Gcomp = 0.5;
}

float MoogLadderFilter::antiImageFilter(float in) {

	float out = in * resamplingCoefficients[0];

	for (int i = 1; i <= 10; ++i)
	{
		out += antiImageFilterState[(antiImagePointer - i + 10) % 10] * resamplingCoefficients[i];

		antiImageFilterState[antiImagePointer] = in;

		antiImagePointer++;
		if (antiImagePointer >= 10)
			antiImagePointer = 0;
	}

	return out;
}

float MoogLadderFilter::antiAliasFilter(float in) {

	float out = in * resamplingCoefficients[0];

	for (int i = 1; i <= 10; ++i)
	{
		out += antiAliasFilterState[(antiAliasPointer - i + 10) % 10] * resamplingCoefficients[i];

		antiAliasFilterState[antiImagePointer] = in;

		antiImagePointer++;
		if (antiImagePointer >= 10)
			antiImagePointer = 0;
	}

	return out;
}
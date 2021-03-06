/*
 *   MoodLadderFiler.h
 *
 *  Created on: Apr 6, 2016
 *      Author: Alistair Barker
 */

 #include <cmath>
#include <BeagleRT.h>

enum FilterType {
    filterTypeLowPass = 0,
    filterTypeHighPass
};

class MoogLadderFilter
{
public:
	MoogLadderFilter();
	~MoogLadderFilter() {}

    void setCoefficients(int filterType, float cutoff, float resonance, float sampleRate);
    float processSample(float sample);
    void reset();

private:

	float antiImageFilter(float in);
	float antiAliasFilter(float in);


	float state[5];
	float prevState[5];

	float antiImageFilterState[10];
	int antiImagePointer;
	float antiAliasFilterState[10];
	int antiAliasPointer;

	float resamplingCoefficients[11];


	float Gcomp;
	float g, h0, h1;

	float resonance;
};
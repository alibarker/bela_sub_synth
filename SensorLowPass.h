/*
 *   SensorLowPass.h
 *
 *  Created on: Apr 13, 2016
 *      Author: Alistair Barker
 */

 class SensorLowPass
 {
 public:
 	SensorLowPass(int M)
 	{
 		filterOrder = M;
 		prevSamples = new float[filterOrder];
 		writePointer = 0;
 	}
 	~SensorLowPass()
 	{
 		delete prevSamples;
 	}

 	float processSample(float input)
 	{
 		float output = input/filterOrder;

 		for (int i = 0; i < filterOrder; ++i)
 		{
 			output += prevSamples[ (writePointer - i + filterOrder)  ] / filterOrder;
 		}

 		prevSamples[writePointer] = input;

 		writePointer++;
 		if (writePointer >= filterOrder)
 		{
 			writePointer = 0;
 		}
 		return output;
 	}
 	
 private:
 	float* prevSamples;
 	int filterOrder;
 	int writePointer;
 };
/***** peakDetection.h *****/
/*
 * Written for ECS7012U Music and 
 * Audio Programming, for the Bela platform.
 *
 * Alex Richardson 2020
 */

#ifndef PEAKDETECTION_H
#define PEAKDETECTION_H

#include <vector>
#include <numeric>

// A peak detection algorithm
// outputData must be an int array of equal size to inputData

float mean(std::vector<float> vec){
	float sum = std::accumulate(std::begin(vec), std::end(vec), 0.0);
	return sum / vec.size();
}

float standardDeviation(std::vector<float> vec){
	float m = mean(vec); // Obtain mean
	
	float temp = 0;
	for(int i = 0; i < vec.size(); i++){ // Sum the squares of the deviations from the mean
		temp += (vec[i] - m) * (vec[i] - m);
	}
	float stdDev = sqrt(temp / (vec.size() - 1)); // Divide by number of samples -1, then sqrt to get standard deviation
	return stdDev;
}

void detectPeaks(int inputSize, float* inputData, int* outputData){

	// Smoothing coefficient
	int lag = 5; 
	
	// Theshold for signal in standard deviations from the mean.
	float signalThreshold = 20;
	
	// Between 0 and 1
	float influence = 0;
	
	
	// If the input is too short, return an empty output array
	if(inputSize <= lag + 2){
		for(int i = 0; i < inputSize; i++){
			outputData[i] = 0;
		}
		return;
	}
	
	std::vector<float> filteredY(inputSize, 0.0); // Use a vector to allow dynamic declaration at runtime
	std::vector<float> avgFilter(inputSize, 0.0);
	std::vector<float> stdFilter(inputSize, 0.0);
	std::vector<float> subVecStart(inputData[0], inputData[0] + lag);
	
	for(int i = lag + 1; i < inputSize; i++){
		if(std::abs(inputData[i] - avgFilter[i-1]) > signalThreshold * stdFilter[i-1]){
			if(inputData[i] > 0.1){
				outputData[i] = 1; // Positive signal
			}
			else{
				outputData[i] = 0;
			}
			// Reduce influence
			filteredY[i] = influence * inputData[i] + (1-influence) * filteredY[i-1];
		}
		else{
			outputData[i] = 0; // No signal
		}
		
		// Adjust filters
		std::vector<float> subVec(filteredY.begin() + i - lag, filteredY.begin() + i); // Temporary vector to store the last lag entries for calculations
		avgFilter[i] = mean(subVec); // Take mean of last lag values
		stdFilter[i] = standardDeviation(subVec); // Take standard deviations of last lag values
	}
}

#endif // PEAKDETECTION_H
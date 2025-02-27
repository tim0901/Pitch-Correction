/***** hps.h *****/
/*
 * Written for ECS7012U Music and 
 * Audio Programming, for the Bela platform.
 *
 * Alex Richardson 2020
 */

#ifndef HPS_H
#define HPS_H

#include <math.h>

#include "peakDetection.h"

// A harmonic product spectrum used for pitch detection

class HPS{
public:
	
	HPS(int size, int sr):bufferSize(size * 0.5), sampleRate(sr), HPSSize(floor(size/6)){ // Constructor, to be called in setup()
		amplitudeSpectrum = (float*)malloc(bufferSize * sizeof(float));
		twoSigma = (float*) malloc (bufferSize * sizeof(float));
		threeSigma = (float*) malloc (HPSSize * sizeof(float));
		productSpectrum = (float*) malloc (HPSSize * sizeof(float));
		frequencyStep = (float)sampleRate / (float)(size); // Calculate the frequency step for each 
		detectedPeaks = (int*)malloc(bufferSize * sizeof(int));
	}
	
	~HPS(){ // Destructor
		free(amplitudeSpectrum);
		free(twoSigma);
		free(threeSigma);
		free(productSpectrum);
		free(detectedPeaks);
		rt_printf("HPS deleted.\n");
	}
	
	// Import data from a ne10 FFT frequency spectrum
	inline void importSpectrum(ne10_fft_cpx_float32_t* spectrum){
		
		// We only care about the first half as it is symmetric for real signals
		for(int i = 0; i < bufferSize; i++){
			amplitudeSpectrum[i] = sqrt((spectrum[i].r * spectrum[i].r) + (spectrum[i].i * spectrum[i].i)); // Square the two components, then store the square root of their sum as the amplitude
		}
	}
	
	// Calculate the HPS
	void calculate();
	
	// Find the peak in the product spectrum
	int returnPeakLocation();
	
	void exportHPS(std::string fileName, bool includeIntermediaries = false);
	
	// Return an estimate of the exact frequency of the incoming signal
	float estimateFundamentalFrequency(int peakBin = 0);
	
private:
	float* amplitudeSpectrum;
	float* twoSigma;
	float* threeSigma;
	float* productSpectrum;
	const int bufferSize;
	const int sampleRate;
	const int HPSSize;
	float frequencyStep;
	int* detectedPeaks;
};

// Calculate the HPS
void HPS::calculate(){
	for(int i = 0; i < HPSSize; i++){
		twoSigma[i] = amplitudeSpectrum[i*2]; // Find every other value 
		threeSigma[i] = amplitudeSpectrum[i*3]; // Find every third value
		productSpectrum[i] = amplitudeSpectrum[i] * twoSigma[i] * threeSigma[i];
	}
}

// Find the peak in the product spectrum
int HPS::returnPeakLocation(){
	
	int peakLocation = 0;
	int peakAmplitude = 0; 
	
	// Ignore values below 50Hz as they're noisy
	int lowerLimit = ceil(50.0 / frequencyStep);
	
	detectPeaks(HPSSize, productSpectrum, detectedPeaks);
	
	for(int i = lowerLimit; i < HPSSize; i++){
		if(detectedPeaks[i] == 1){
			if(amplitudeSpectrum[i] > peakAmplitude){
				peakLocation = i;
				peakAmplitude = amplitudeSpectrum[i];
			}
		}
	}
	
	return peakLocation;
}

// Return an estimate of the exact frequency of the incoming signal
float HPS::estimateFundamentalFrequency(int peakBin){
	if(peakBin == 0){
		peakBin = this->returnPeakLocation();
	}
	
	// Quadratic interpolation
	
	// Find the amplitudes of the bins either side of the peak.
	float alpha = amplitudeSpectrum[peakBin-1];
	float beta  = amplitudeSpectrum[peakBin];
	float gamma = amplitudeSpectrum[peakBin+1];
	
	float relativePeakLocation = 0.5*(alpha - gamma)/(alpha - 2*beta + gamma); // Estimate where the peak is within the bins
	
	float frequencyEstimation = (relativePeakLocation + peakBin) * frequencyStep; // Use this estmated location to calculate the frequency
	
	// We can't make accurate estimations below 50Hz so these are ignored
	// This also helpfully filters out lots of noise
	if(frequencyEstimation < 50){
		return 0;
	}
	
	return frequencyEstimation;
}

void HPS::exportHPS(std::string fileName, bool includeIntermediaries){
	// Writes the HPS to a text file
	
	rt_printf("Saving spectrum.\n");
	
	// Stream for writing to file
	std::ofstream ofs;
	
	// Open file
	ofs.open(fileName);
	
	// Write to file
	if(includeIntermediaries){
		for(int i = 0; i < HPSSize; i++){
			ofs << frequencyStep * (float)i << " " << amplitudeSpectrum[i] << " " << twoSigma[i] << " " << threeSigma[i] << " " << productSpectrum[i] << "\n";
		}
	}
	else{
		for(int i = 0; i < HPSSize; i++){
			ofs << frequencyStep * (float)i << " " << productSpectrum[i] << "\n";
		}
	}
	
	// Close stream
	ofs.close();
	
	rt_printf("Save complete.\n");
	
}
#endif //HPS_H
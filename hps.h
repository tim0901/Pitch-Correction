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

// A harmonic product spectrum used for pitch detection

class HPS{
public:
	
	HPS(int size, int sr):bufferSize(size * 0.5), sampleRate(sr), HPSSize(floor(bufferSize/6)){ // Constructor, to be called in setup()
		amplitudeSpectrum = (float*)malloc(bufferSize * sizeof(float));
		twoSigma = (float*) malloc (0.5 * bufferSize * sizeof(float));
		threeSigma = (float*) malloc (HPSSize* sizeof(float));
		productSpectrum = (float*) malloc (HPSSize * sizeof(float));
		frequencyStep = (float)sampleRate / (float)(bufferSize * 2); // Calculate the frequency step for each 
	}
	
	~HPS(){ // Destructor
		free(amplitudeSpectrum);
		free(twoSigma);
		free(threeSigma);
		free(productSpectrum);
		rt_printf("HPS deleted.\n");
	}
	
	// Import data from a ne10 FFT frequency spectrum
	inline void importSpectrum(ne10_fft_cpx_float32_t* spectrum){
		
		// We only care about the first half as it is symmetric for real signals
		for(int i = 0; i < 0.5*bufferSize; i++){
			amplitudeSpectrum[i] = sqrt((spectrum[i].r * spectrum[i].r) + (spectrum[i].i * spectrum[i].i)); // Square the two components, then store the square root of their sum as the amplitude
		}
	}
	
	// Calculate the HPS
	void calculate();
	
	// Find the peak in the product spectrum
	int returnPeakLocation();
	
	void exportHPS(std::string fileName, bool includeIntermediaries = false);
	
	// Return an estimate of the exact frequency of the incoming signal
	float estimateFundamentalFrequency();
	
private:
	float* amplitudeSpectrum;
	float* twoSigma;
	float* threeSigma;
	float* productSpectrum;
	const int bufferSize;
	const int sampleRate;
	const int HPSSize;
	float frequencyStep;
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
	int peakMagnitude = 0; 
	
	// Ignore values below 50Hz as they're noisy
	int lowerLimit = ceil(50.0 / frequencyStep);
	
	for(int i = lowerLimit; i < HPSSize; i++){// Find largest magnitude in the array
		if(productSpectrum[i] > peakMagnitude){
			peakLocation = i;
			peakMagnitude = productSpectrum[i];
		}
	}
	
	return peakLocation;
}

// Return an estimate of the exact frequency of the incoming signal
float HPS::estimateFundamentalFrequency(){
	
	int peakBin = this->returnPeakLocation();
	
	// Quadratic interpolation
	
	// Find the amplitudes of the bins either side of the peak.
	float alpha = amplitudeSpectrum[peakBin-1];
	float beta  = amplitudeSpectrum[peakBin];
	float gamma = amplitudeSpectrum[peakBin+1];
	
	float relativePeakLocation = 0.5*(alpha - gamma)/(alpha - 2*beta + gamma); // Estimate where the peak is within the bins
	
	float frequencyEstimation = (relativePeakLocation + peakBin) * frequencyStep; // Use this estmated location to calculate the frequency
	
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
/***** phaseVocoder.h *****/
/*
 * Written for ECS7012U Music and 
 * Audio Programming, for the Bela platform.
 *
 * Alex Richardson 2020
 */

#ifndef PHASEVOCODER_H
#define PHASEVOCODER_H

// Multiply two complex numbers together
void complexMultiply(float realA, float complexA, float realB, float complexB, float* output);

// A phase vocoder for adjusting the pitch of incoming audio
class phaseVocoder{
public:
	phaseVocoder(int s, int hS, int sr):size(s), hopSize(hS), sampleRate(sr){ // Constructor
		frequencyStep = (float)sampleRate / (float)(size); // Calculate the frequency step
		inverseFrequencyStep = 1/frequencyStep; // Cache the inverse for efficiency
		cachedPhaseShift = (float*) malloc (2 * sizeof(float));
		memset(cachedPhaseShift, 0, 2 * sizeof(float));
		complexMultiplied = (float*) malloc ( 2 * sizeof(float));
		memset(complexMultiplied, 0, 2 * sizeof(float));
	}
	~phaseVocoder(){ // Destructor
		free(cachedPhaseShift);
		free(complexMultiplied);
	}
	
	// Shift the peak at the given location
	void shiftFrequency(ne10_fft_cpx_float32_t* frequencySpectrum, int peakBin, float currentFrequency, float desiredFrequency);
	
private:
	float frequencyStep;
	float inverseFrequencyStep;
	const int size;
	const int hopSize;
	const int sampleRate;
	float* cachedPhaseShift;
	float* complexMultiplied;
};

// Shift the peak at the given location
void phaseVocoder::shiftFrequency(ne10_fft_cpx_float32_t* frequencySpectrum, int peakBin, float currentFrequency, float desiredFrequency){
	
	// Ensure bin is valid
	if(peakBin == 0){
		return;
	}
	
	// Find the frequency shift that must be performed
	// The number of bins we need to shift by
	float frequencyShift = (desiredFrequency - currentFrequency) * inverseFrequencyStep; // Amount that the frequency needs to be shifted by
	
	// The phase of the sample will need to be corrected to remove artifacting
	// The phase shift is cumulated between FFTs
	if(cachedPhaseShift[0] == 0 && cachedPhaseShift[1] == 0){
		cachedPhaseShift[0] = cos(frequencyShift * hopSize);
		cachedPhaseShift[1] = sin(frequencyShift * hopSize);
	}
	else{
		complexMultiply(cachedPhaseShift[0], cachedPhaseShift[1], cos(frequencyShift*hopSize), sin(frequencyShift*hopSize), cachedPhaseShift);
		while(cachedPhaseShift[0] > M_PI*hopSize){ // Wrap phase
			cachedPhaseShift[0] -= 2*M_PI*hopSize;
		}
		while(cachedPhaseShift[1] > M_PI*hopSize){
			cachedPhaseShift[1] -= 2*M_PI*hopSize;
		}
	}
	
	// Use linear interpolation to shift the peak
	for(int i = peakBin-2; i < peakBin+3; i++){
		frequencySpectrum[i].r = (ne10_float32_t)(1.0 - frequencyShift) * frequencySpectrum[i].r + (ne10_float32_t)frequencyShift * frequencySpectrum[i+1].r;
		frequencySpectrum[i].i = (ne10_float32_t)(1.0 - frequencyShift) * frequencySpectrum[i].i + (ne10_float32_t)frequencyShift * frequencySpectrum[i+1].i;

		// Correct the phase by multiplying the corrected frequency spectrum by the phase shift
		complexMultiply(frequencySpectrum[i].r, frequencySpectrum[i].i, (ne10_float32_t)cachedPhaseShift[0], (ne10_float32_t)cachedPhaseShift[1], complexMultiplied);
		frequencySpectrum[i].r = (ne10_float32_t)complexMultiplied[0];
		frequencySpectrum[i].i = (ne10_float32_t)complexMultiplied[1];
	}
	
	// Mirror the spectrum
	for(int i = 0; i < size/2; i++){
		frequencySpectrum[size-i].r = frequencySpectrum[i].r;
		frequencySpectrum[size-i].i = frequencySpectrum[i].i;
	}
	
}


// Multiply two complex numbers together
void complexMultiply(float realA, float complexA, float realB, float complexB, float* output){
	
	float ac = realA * realB;
	float bidi = complexA * complexB;
	float bci = complexA * realB;
	float adi = realA * complexB;
	
	output[0] = ac - bidi;
	output[1] = bci + adi;
}


#endif // PHASEVOCODER_H
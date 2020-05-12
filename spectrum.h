/***** spectrum.h *****/
/*
 * Alex Richardson 2020
 */

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "fftContainer.h"

void generateFrequencySpectrum(FFTContainer* fft, std::string fileName){
	
	// Writes the amplitude spectrum to a text file
	
	rt_printf("Saving spectrum.\n");
	
	// Stream for writing to file
	std::ofstream ofs;
	
	// Open file
	ofs.open(fileName);
	float frequencyIncrement = (float)fft->sampleRate / ((float)fft->size); // The central frequency found in a particular bin
	
	ofs << 0 << " " << sqrt((fft->frequencyDomain[0].r * fft->frequencyDomain[0].r) + (fft->frequencyDomain[0].i * fft->frequencyDomain[0].i)) << "\n"; // DC - must not be doubled
	
	for(int i = 1; i < 0.5*fft->size; i++){ // Real spectra are perfectly symmetric - only need half of the elements. Remaining have their amplitudes doubled
		ofs << frequencyIncrement * (float)i << " " << 2 * sqrt((fft->frequencyDomain[i].r * fft->frequencyDomain[i].r) + (fft->frequencyDomain[i].i * fft->frequencyDomain[i].i)) << "\n";
	}
	
	// Close stream
	ofs.close();
	
	rt_printf("Save complete.\n");
}

#endif //SPECTRUM_H
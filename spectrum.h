/***** spectrum.h *****/
/*
 * Written for ECS7012U Music and 
 * Audio Programming, for the Bela platform.
 *
 * Alex Richardson 2020
 */
 
#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "fftContainer.h"

void generateFrequencySpectrum(ne10_fft_cpx_float32_t* frequencyDomain, int sampleRate, int size,  std::string fileName){
	
	// Writes the amplitude spectrum to a text file
	
	rt_printf("Saving spectrum.\n");
	
	// Stream for writing to file
	std::ofstream ofs;
	
	// Open file
	ofs.open(fileName);
	float frequencyIncrement = (float)sampleRate / ((float)size); // The central frequency found in a particular bin
	
	ofs << 0 << " " << sqrt((frequencyDomain[0].r * frequencyDomain[0].r) + (frequencyDomain[0].i * frequencyDomain[0].i)) << "\n"; // DC - must not be doubled
	
	for(int i = 1; i < 0.5*size; i++){ // Real spectra are perfectly symmetric - only need half of the elements. Remaining have their amplitudes doubled
		ofs << frequencyIncrement * (float)i << " " << 2 * sqrt((frequencyDomain[i].r * frequencyDomain[i].r) + (frequencyDomain[i].i * frequencyDomain[i].i)) << "\n";
	}
	
	// Close stream
	ofs.close();
	
	rt_printf("Save complete.\n");
}



#endif //SPECTRUM_H
/***** fftContainer.h *****/
/*
 * Alex Richardson 2020
 */
 
#ifndef FFTCONTAINER_H
#define FFTCONTAINER_H

// The fourier xfm arrays are encapsulated here for convenience
struct FFTContainer{
	FFTContainer(int s, int sr):size(s), sampleRate(sr){ // Constructor
		// Allocate memory for FFT of length size
		timeDomainIn  = (ne10_fft_cpx_float32_t*) NE10_MALLOC (size * sizeof(ne10_fft_cpx_float32_t));
		timeDomainOut = (ne10_fft_cpx_float32_t*) NE10_MALLOC (size * sizeof(ne10_fft_cpx_float32_t));
		frequencyDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC (size * sizeof(ne10_fft_cpx_float32_t));
		cfg = ne10_fft_alloc_c2c_float32_neon(size);
		
		// Set timeDomainOut to zero so that the first BUFFER_SIZE samples don't bug out
		memset(timeDomainOut, 0, size * sizeof(ne10_fft_cpx_float32_t));
	}
	~FFTContainer(){
		NE10_FREE(timeDomainIn);
		NE10_FREE(timeDomainOut);
		NE10_FREE(frequencyDomain);
		NE10_FREE(cfg);	
		rt_printf("FFTContainer deleted.\n");
	}
	
	ne10_fft_cpx_float32_t* timeDomainIn; // Array of input data
	ne10_fft_cpx_float32_t* frequencyDomain;
	ne10_fft_cpx_float32_t* timeDomainOut; // Array of processed audio 
	ne10_fft_cfg_float32_t cfg; // FFT configuration structure
	
	int size; // Number of bins of the FFT
	int sampleRate; // Sample rate of the incoming signal for frequency analysis
	
};

#endif // FFTCONTAINER_H
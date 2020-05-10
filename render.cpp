/*
 * assignment3
 * ECS7012 Music and Audio Programming
 *
 * This code runs on the Bela embedded audio platform (bela.io).
 */


#include <Bela.h>
#include <libraries/ne10/NE10.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <fstream>

#include "circularBuffer.h"
#include "fftContainer.h"
#include "spectrum.h"

circularBuffer** gInputBuffers; // The circular buffers used for storing input data
circularBuffer** gOutputBuffers;

float** gReturnBuffers;

#define BUFFER_SIZE 4096 // Number of samples to be stored in the buffer - accounting for 2-channel audio

int gHopCounter = 0;
int gHopSize = 4096;
int gWindowSize = 4096;

int gAudioChannels = 0;

float* gOut;

int gSpectrumTimer = 0;

// Thread for FFT processing
AuxiliaryTask gFFTTask;

// Predeclaration
void processAudio(void* arg);


FFTContainer** gFFTs;

float hanningWindow(int entry){
	return 0.5-(0.5*cos((2*M_PI*entry)/(BUFFER_SIZE-1)));
}

bool setup(BelaContext *context, void *userData)
{
	
	// Ensure ne10 loaded properly
	if(ne10_init() != NE10_OK){
		rt_printf("Failed to init Ne10.");
		return false;
	}
	
	// For output
	gOut = new float;
	*gOut = 0;
	
	gAudioChannels = context->audioInChannels; // Required to pass value to secondary thread
	
	// Create circular buffers for input storage - one per audio channel
	gInputBuffers = (circularBuffer**) malloc (context->audioInChannels * sizeof(circularBuffer*));
	gOutputBuffers = (circularBuffer**) malloc (context->audioInChannels * sizeof(circularBuffer*));
	
	// Allocate return buffers for transferring audio from the circular buffers
	gReturnBuffers = (float**) malloc (context->audioInChannels * sizeof(float*));
	
	// We need one FFT per audio channel
	gFFTs = (FFTContainer**)malloc(context->audioInChannels * sizeof(FFTContainer*));
	
	// Allocate memory per audio channel
	for(int channel = 0; channel < context->audioInChannels; channel++){
		gInputBuffers[channel] = new circularBuffer(BUFFER_SIZE);
		gOutputBuffers[channel] = new circularBuffer(gWindowSize + gHopSize);
		gReturnBuffers[channel] = (float*) malloc (BUFFER_SIZE * sizeof(float));
		gFFTs[channel] = new FFTContainer(BUFFER_SIZE, context->audioSampleRate);
	}
	
	gFFTTask = Bela_createAuxiliaryTask(processAudio, 94, "bela-process-fft");
	
	rt_printf("Setup complete.\n");
	
	return true;
}


void processAudio(void *arg){


	// For each audio channel
	for(int channel = 0; channel < gAudioChannels; channel++){
		// Return gWindowSize samples from the buffers
		gInputBuffers[channel]->returnValues(gWindowSize, &gReturnBuffers[channel]);
		
		// Pass data to ne10 time domain 
		for(int i = 0; i < gWindowSize; i++){
			gFFTs[channel]->timeDomainIn[i].r = (ne10_float32_t)(gReturnBuffers[channel][i] * hanningWindow(i));
			gFFTs[channel]->timeDomainIn[i].i = 0;
		}
				
		// Calculate FFT
		ne10_fft_c2c_1d_float32_neon(gFFTs[channel]->frequencyDomain, gFFTs[channel]->timeDomainIn, gFFTs[channel]->cfg, 0);
		
		if(gSpectrumTimer == 5){// First spectrum will be empty
			generateFrequencySpectrum(gFFTs[0], "spectrum.txt");
		}
		gSpectrumTimer++;
		
		// Calculate inverse FFT
		ne10_fft_c2c_1d_float32_neon(gFFTs[channel]->timeDomainIn, gFFTs[channel]->frequencyDomain, gFFTs[channel]->cfg, 1);
		
		// Insert processed values into output buffers
		for(int i = 0; i < BUFFER_SIZE; i++){
			gOutputBuffers[channel]->insert(gFFTs[channel]->timeDomainIn[i].r);
		}
	}
}

void render(BelaContext *context, void *userData)
{	
	// For each audio frame
	for(unsigned int n = 0; n < context->audioFrames; n++){
		
		// For each audio channel
		for(int channel = 0; channel < context->audioInChannels; channel++){
			
			// Read samples from audio input header
			float in = audioRead(context, n, channel);
			// and store them in circular buffers - one per channel
			gInputBuffers[channel]->insert(in);
			
			// Read the next values from gOutputBuffers
			gOutputBuffers[channel]->returnValues(1, &gOut); 
			
			// And write them to the output
			audioWrite(context, n, channel, *gOut);
		}
		
		gHopCounter++; 
		// Only process FFT after gHopSize samples
		if(gHopCounter >= gHopSize){
			
			Bela_scheduleAuxiliaryTask(gFFTTask); // Process on auxiliary thread
			gHopCounter = 0; // Reset counter
		}
		
	}
	
}

void cleanup(BelaContext *context, void *userData)
{
	for(int channel = 0; channel < context->audioInChannels; channel++){
		delete gFFTs[channel];
		delete gInputBuffers[channel];
		delete gOutputBuffers[channel];
		free(gReturnBuffers[channel]);
	}
	free(gFFTs);
	free(gInputBuffers);
	free(gOutputBuffers);
	free(gReturnBuffers);
	
	delete gOut;
	
}
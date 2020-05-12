/*
 * Assignment 3: Pitch Detection
 * ECS7012 Music and Audio Programming
 *
 * This code runs on the Bela embedded audio platform (bela.io).
 *
 * Alex Richardson 2020
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
#include "button.h"
#include "hps.h"

button *gSpectrumButton; // The button used to export a spectrum. 

circularBuffer** gInputBuffers; // The circular buffers used for storing input data
circularBuffer** gOutputBuffers;


#define BUFFER_SIZE 16384// Number of samples to be stored in the buffer - accounting for 2-channel audio
int gCachedInputBufferPointers[2] = {0}; // Cached input buffer write pointers.

int gHopCounter = 0; 
int gWindowSize = 8192; // Size of window
int gHopSize = gWindowSize/2; // How far between hops

int gAudioChannels = 0; // Used to store the number of audio channels to be passed to the auxiliary task

float* gHanningWindow; // The Hanning window

int gSpectrumTimer = 0; // For spectrum output

// Thread for FFT processing
AuxiliaryTask gFFTTask;

// FFT containers, defined in fftContainer.h
FFTContainer** gFFTs;

// Harmonic product spectrums for finding fundamental frequency
HPS** gHPSs;

// Predeclaration
void processAudio(void* arg);

bool setup(BelaContext *context, void *userData)
{
	
	// Ensure ne10 loaded properly
	if(ne10_init() != NE10_OK){
		rt_printf("Failed to init Ne10.");
		return false;
	}
	
	gSpectrumButton = new button(context, 1); // Init button
	
	gAudioChannels = context->audioInChannels; // Required to pass value to secondary thread
	
	// Create circular buffers for input/output storage - one per audio channel
	gInputBuffers = (circularBuffer**) malloc (context->audioInChannels * sizeof(circularBuffer*));
	gOutputBuffers = (circularBuffer**) malloc (context->audioInChannels * sizeof(circularBuffer*));
	
	// We need one FFT per audio channel
	gFFTs = (FFTContainer**)malloc(context->audioInChannels * sizeof(FFTContainer*));
	
	// Harmonic product spectra for finding the fundamental frequency (pitch) of the incoming signal, one per channel
	gHPSs = (HPS**)malloc(context->audioInChannels * sizeof(HPS*));
	
	// Allocate memory per audio channel
	for(int channel = 0; channel < context->audioInChannels; channel++){
		gInputBuffers[channel] = new circularBuffer(BUFFER_SIZE);
		gOutputBuffers[channel] = new circularBuffer(BUFFER_SIZE);
		gOutputBuffers[channel]->setWritePointer(gHopSize);
		gFFTs[channel] = new FFTContainer(gWindowSize, context->audioSampleRate);
		gHPSs[channel] = new HPS(gWindowSize, context->audioSampleRate);
	}
	
	// Prepopulate Hanning window array for efficiency
	gHanningWindow = (float*) malloc (gWindowSize * sizeof(float));
	for(int i = 0; i < gWindowSize; i++){
		gHanningWindow[i] = 0.5-(0.5*cos((2*M_PI*(float)i)/((float)gWindowSize-1.0)));
	}
	
	// Set up auxiliary task
	gFFTTask = Bela_createAuxiliaryTask(processAudio, 94, "bela-process-fft");
	
	rt_printf("Setup complete.\n");
	
	return true;
}

// Process the audio. This is handled by an auxiliary thread
void processAudio(void *arg){

	// For each channel
	for(int channel = 0; channel < gAudioChannels; channel++){
		
		// Shift read pointer starting position to gWindowSize samples behind the last input
		gInputBuffers[channel]->setReadPointer(gCachedInputBufferPointers[channel] - gWindowSize);
		
		// Load inputs into timerDomain
		for(int i = 0; i < gWindowSize; i++){
			gFFTs[channel]->timeDomainIn[i].r = (ne10_float32_t)gInputBuffers[channel]->returnNextElement() * gHanningWindow[i];
			gFFTs[channel]->timeDomainIn[i].i = 0;
		}
	}
	
	for(int channel = 0; channel < gAudioChannels; channel++){
				
		// Calculate FFT
		ne10_fft_c2c_1d_float32_neon(gFFTs[channel]->frequencyDomain, gFFTs[channel]->timeDomainIn, gFFTs[channel]->cfg, 0);
		
		// ---- Frequency domain processing ---- //
		
		// Output a .txt frequency spectrum when the button is pressed (low) 
		// Will overwrite files with the same name
		// Can cause problems to the audio when used
		if(!gSpectrumButton->returnState()){
			generateFrequencySpectrum(gFFTs[0], "spectrum.txt");
		}
		
		// Use harmonic product spectrum to find the fundamental frequency of the incoming sound
		gHPSs[channel]->importSpectrum(gFFTs[channel]->frequencyDomain);
		gHPSs[channel]->calculate();
		float fundamentalFrequency = gHPSs[channel]->estimateFundamentalFrequency();
		
		// Output a .txt file conatining the HPS when the button is pressed (low)
		// Will overwrite files with the same name
		if(!gSpectrumButton->returnState()){
			gHPSs[0]->exportHPS("HPS.txt");
			rt_printf("%f\n", fundamentalFrequency);
		}
		
		// Calculate inverse FFT to bring the processed audio back to the time domain
		ne10_fft_c2c_1d_float32_neon(gFFTs[channel]->timeDomainOut, gFFTs[channel]->frequencyDomain, gFFTs[channel]->cfg, 1);
	}
	
	for(int channel = 0; channel < gAudioChannels; channel++){
		
		// Add timeDomainOut into the output buffer. Add to any existing values to account for hop overlap
		for(int n = 0; n < gWindowSize; n++) {
			gOutputBuffers[channel]->insertAndAdd(gFFTs[channel]->timeDomainOut[n].r);
		}
		// CircularBuffer automatically iterates the write pointer every time an element is added
		// So we need to pull it back by (gWindowSize-gHopSize) elements to ensure correct hop size
		gOutputBuffers[channel]->setWritePointer(gOutputBuffers[channel]->returnWritePointer() - (gWindowSize - gHopSize)); 
	}
	
}

void render(BelaContext *context, void *userData)
{	
	// Update button state
	gSpectrumButton->updateState(context);
	
	// For each audio frame
	for(unsigned int n = 0; n < context->audioFrames; n++){
		
		// For each audio channel
		for(int channel = 0; channel < context->audioInChannels; channel++){
			
			// Read samples from audio input header
			float in = audioRead(context, n, channel);
			
			// and store them in circular buffers - one per channel
			gInputBuffers[channel]->insert(in);
		}
		
		for(int channel = 0; channel < context->audioInChannels; channel++){
			
			// Read the next values from the output buffers
			float out = gOutputBuffers[channel]->returnAndEmptyNextElement();
			
			// And write them to the output
			audioWrite(context, n, channel, out);
		}
		
		gHopCounter++; 
		// Only process FFT after gHopSize samples
		if(gHopCounter >= gHopSize){
			
			// Cache input buffer write pointers for auxiliary thread
			for(int channel = 0; channel < context->audioInChannels; channel++){
				gCachedInputBufferPointers[channel] = gInputBuffers[channel]->returnWritePointer();
			}
			Bela_scheduleAuxiliaryTask(gFFTTask); // Process audio on auxiliary thread
			
			gHopCounter = 0; // Reset hop counter
		}
		
	}
	
}

void cleanup(BelaContext *context, void *userData)
{
	for(int channel = 0; channel < context->audioInChannels; channel++){
		delete gHPSs[channel];
		delete gFFTs[channel];
		delete gInputBuffers[channel];
		delete gOutputBuffers[channel];
	}
	free(gHPSs);
	free(gFFTs);
	free(gInputBuffers);
	free(gOutputBuffers);
	free(gHanningWindow);
	
	delete gSpectrumButton;
}
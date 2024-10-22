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
#include "compareNotes.h"
#include "phaseVocoder.h"

button *gSpectrumButton; // The button used to export a spectrum. 
button *gDisableButton;
button *gScaleButton;

circularBuffer** gInputBuffers; // The circular buffers used for storing input data
circularBuffer** gOutputBuffers;

#define BUFFER_SIZE 16384// Number of samples to be stored in the buffer - accounting for 2-channel audio
int gCachedInputBufferPointers[2] = {0}; // Cached input buffer write pointers.

int gHopCounter = 0; 
int gWindowSize = 4096; // Size of window
int gHopSize = gWindowSize/4; // How far between hops

int gAudioChannels = 0; // Used to store the number of audio channels to be passed to the auxiliary task

float* gHanningWindow; // The Hanning window

// Temporary storage for storing the unprocessed frequency domain when exporting a spectrum
ne10_fft_cpx_float32_t* oldFrequencyDomain;

// Thread for FFT processing
AuxiliaryTask gFFTTask;

// FFT containers, defined in fftContainer.h
FFTContainer** gFFTs;

// Harmonic product spectrums for finding fundamental frequency
HPS** gHPSs;

// The fundamental frequency for each channel
float gFundamentalFrequencies[2] = {0};

// The phase vocoder used to shift the frequency peak
phaseVocoder** gPhaseVocoders;

enum{ // Available scales for note comparisons
	PENTATONIC = 0,
	C_MAJOR = 1,
	C_MINOR = 2
};

int gScale = PENTATONIC; // Which scale should be used?

int gScaleTimer = 0; // How long has it been since the scale has been changed?

// Predeclaration
void processAudio(void* arg);

bool setup(BelaContext *context, void *userData)
{
	
	// Ensure ne10 loaded properly
	if(ne10_init() != NE10_OK){
		rt_printf("Failed to init Ne10.");
		return false;
	}
	
	gSpectrumButton = new button(context, 1); // Init buttons
	gDisableButton = new button(context, 2);
	gScaleButton = new button(context, 3);
	
	gAudioChannels = context->audioInChannels; // Required to pass value to secondary thread
	
	// Create circular buffers for input/output storage - one per audio channel
	gInputBuffers = (circularBuffer**) malloc (context->audioInChannels * sizeof(circularBuffer*));
	gOutputBuffers = (circularBuffer**) malloc (context->audioInChannels * sizeof(circularBuffer*));
	
	// We need one FFT per audio channel
	gFFTs = (FFTContainer**)malloc(context->audioInChannels * sizeof(FFTContainer*));
	
	// Harmonic product spectra for finding the fundamental frequency (pitch) of the incoming signal, one per channel
	gHPSs = (HPS**)malloc(context->audioInChannels * sizeof(HPS*));
	
	// Phase vocoders for shifting the frequency peaks
	gPhaseVocoders = (phaseVocoder**)malloc(context->audioInChannels * sizeof(phaseVocoder*));
	
	// Allocate memory per audio channel
	for(int channel = 0; channel < context->audioInChannels; channel++){
		gInputBuffers[channel] = new circularBuffer(BUFFER_SIZE);
		gOutputBuffers[channel] = new circularBuffer(BUFFER_SIZE);
		gOutputBuffers[channel]->setWritePointer(gHopSize);
		gFFTs[channel] = new FFTContainer(gWindowSize, context->audioSampleRate);
		gHPSs[channel] = new HPS(gWindowSize, context->audioSampleRate);
		gPhaseVocoders[channel] = new phaseVocoder(gWindowSize, gHopSize, context->audioSampleRate);
	}
	
	// Temporary storage for storing the unprocessed frequency domain when exporting a spectrum
	// Otherwise it is overwritten by the phase vocoder before it finishes writing to the file
	oldFrequencyDomain = (ne10_fft_cpx_float32_t*) malloc (gWindowSize * sizeof(ne10_fft_cpx_float32_t));
	
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
		
		
		if(gDisableButton->isPressed() == false){ // Disable processing if button 2 is pressed
			
			// Output a .txt frequency spectrum when the button is pressed (low) 
			// Will overwrite files with the same name
			// Can cause problems to the audio when used
			if(gSpectrumButton->isPressed() && channel == 0){
				
				// Store frequencyDomain so that it isn't overwritten before output is complete
				for(int i = 0; i < gWindowSize; i++){
					oldFrequencyDomain[i].r = gFFTs[0]->frequencyDomain[i].r;
					oldFrequencyDomain[i].i = gFFTs[0]->frequencyDomain[i].i;
				}
				generateFrequencySpectrum(oldFrequencyDomain, gFFTs[0]->sampleRate, gFFTs[0]->size, "frequency_spectrumOld.txt");
			}
			
			// Use harmonic product spectrum to find the fundamental frequency of the incoming sound
			gHPSs[channel]->importSpectrum(gFFTs[channel]->frequencyDomain);
			gHPSs[channel]->calculate();
			int peakBin = gHPSs[channel]->returnPeakLocation();
			float fundamentalFrequency = gHPSs[channel]->estimateFundamentalFrequency(peakBin);
			
			// Only update gFundamentalFrequencies if the output is valid
			if(fundamentalFrequency != 0){
				gFundamentalFrequencies[channel] = fundamentalFrequency;
				//rt_printf("%f\n", gFundamentalFrequencies[channel]); // For monitoring
			}
			
			// Find the note that's closest to the fundamental frequency
			float desiredNote = compareNotes(gScale, gFundamentalFrequencies[channel]);
			if(fundamentalFrequency != 0){
				rt_printf("Fundamental frequency:%f Desired note:%f\n", gFundamentalFrequencies[channel], desiredNote); // For monitoring
			}
			
			// Output a .txt file conatining the HPS when the button is pressed (low)
			// Will overwrite files with the same name
			// Can cause problems to the audio when used
			if(gSpectrumButton->isPressed() && channel == 0){
				gHPSs[0]->exportHPS("HPSBefore.txt");
			}
			
			// Shift the peak towards the desired note
			gPhaseVocoders[channel]->shiftFrequency(gFFTs[channel]->frequencyDomain, peakBin, gFundamentalFrequencies[channel], desiredNote);
			
			// Output a .txt frequency spectrum when the button is pressed (low) 
			// Will overwrite files with the same name
			// Can cause problems to the audio when used
			if(gSpectrumButton->isPressed()  && channel == 0){
				generateFrequencySpectrum(gFFTs[0]->frequencyDomain, gFFTs[0]->sampleRate, gFFTs[0]->size, "frequency_spectrum.txt");
			}
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
	// Update button states
	gSpectrumButton->updateState(context);
	gDisableButton->updateState(context);
	gScaleButton->updateState(context);
	
	// Update scale used for pitch correction if scale button is pressed
	if(gScaleButton->isPressed() && gScaleTimer > 2000){
		if(gScale == C_MINOR){
			gScale = PENTATONIC;
			rt_printf("PENTATONIC\n");
		}
		else if(gScale == C_MAJOR){
			gScale = C_MINOR;
			rt_printf("C_MINOR\n");
		}
		else if(gScale == PENTATONIC){
			gScale = C_MAJOR;
			rt_printf("C_MAJOR\n");
		}
		gScaleTimer = 0;
	}
	if(gScaleTimer < 2001){
		gScaleTimer++;
	}
	
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
		delete gPhaseVocoders[channel];
		delete gHPSs[channel];
		delete gFFTs[channel];
		delete gInputBuffers[channel];
		delete gOutputBuffers[channel];
	}
	free(gPhaseVocoders);
	free(gHPSs);
	free(gFFTs);
	free(gInputBuffers);
	free(gOutputBuffers);
	free(gHanningWindow);
	free(oldFrequencyDomain);
	
	delete gDisableButton;
	delete gSpectrumButton;
}
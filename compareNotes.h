/***** compareNotes.h *****/
/*
 * Written for ECS7012U Music and 
 * Audio Programming, for the Bela platform.
 *
 * Alex Richardson 2020
 */
 
#ifndef COMPARENOTES_H
#define COMPARENOTES_H

// Predefined scales of note frequencies used for comparisons
float scales[3][87] = {
		{55, 58.27, 61.74, 65.41, 69.3, 73.42, 77.78,
			 82.41, 87.31, 92.5, 98, 103.83, 110, 116.54, 123.47,
			 130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185, 196,
			 207.65, 220, 233.08, 246.94, 261.63, 277.18, 293.66,
			 311.13, 329.63, 349.23, 369.99, 392, 415.3, 440, 466.16,
			 493.88, 523.25, 554.37, 587.33, 622.25, 659.25, 698.46,
			 739.99, 783.99, 830.61, 880, 932.33, 987.77, 1046.5,
			 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98,
			 1567.98, 1661.22, 1760, 1864.66, 1975.53, 2093, 2217.46,
			 2349.32, 2489.02, 2637.02, 2793.83, 2959.96, 3135.96,
			 3322.44, 3520, 3729.31, 3951.07, 4186.01, 4434.92, 4698.63,
			 4978.03, 5274.04, 5587.65, 5919.91, 6271.93, 6644.88, 7040,
			 7458.62, 7902.13},
		{55, 61.74, 65.41, 73.42, 82.41, 87.31, 98, 110, 123.47, 130.81,
			 146.83, 164.81, 174.61, 196, 220, 246.94, 261.63, 293.66, 
			 329.63, 349.23, 392, 440, 493.88, 523.25, 587.33, 659.25, 
			 698.46, 783.99, 880, 987.77, 1046.5, 1174.66, 1318.51, 
			 1396.91, 1567.98, 1760, 1975.53, 2093, 2349.32, 2637.02, 
			 2793.83, 3135.96, 3520, 3951.07, 4186.01, 4698.63, 
			 5274.04, 5587.65, 6271.93, 7040, 7902.13},
		{58.27, 65.41, 73.42, 77.78, 87.31, 98, 103.83, 116.54, 130.81,
			 146.83, 155.56, 174.61, 196, 207.65, 233.08, 261.63, 293.66,
			 311.13, 349.23, 392, 415.3, 466.16, 523.25, 587.33, 622.25,
			 698.46, 783.99, 830.61, 932.33, 1046.5, 1174.66, 1244.51,
			 1396.91, 1567.98, 1661.22, 1864.66, 2093, 2349.32, 2489.02,
			 2793.83, 3135.96, 3322.44, 3729.31, 4186.01, 4698.63,
			 4978.03,5587.65, 6271.93, 6644.88, 7458.62}
};

// Finds the closest note in the scale to the provided frequency
float compareNotes(int scale, float frequency){
	for(int i = 1; i < 87; i++){
		if(frequency < scales[scale][i]){
			if((frequency - scales[scale][i-1]) < (scales[scale][i] - frequency)){ // Is frequency closer to the note above or below?
				return scales[scale][i-1];
			}
			else{
				return scales[scale][i];
			}
		}
	}
	
	return 0;
}


#endif //COMPARENOTES_H
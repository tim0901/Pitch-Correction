/***** circularBuffer.h *****/

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <cstdint>

// A circular buffer

class circularBuffer{
public:
	//circularBuffer(){} // Default constructor
	~circularBuffer(){ // Destructor
		rt_printf("Circular Buffer destructor called.\n");
		if(buffer){
			free(buffer);
		}
		
	}
	
	circularBuffer(int bufSize):bufferSize(bufSize){ // Constructor
		buffer = (float*)malloc(bufferSize * sizeof(float)); // Initialize array
		bufferWritePointer = 0; 
		bufferReadPointer = 0;
	}
	
	// Return the oldest n values inserted into the buffer using the provided array.
	inline void returnValues(int numberOfValues, float** returnArray){
		
		/*
		if(sizeof(returnArray) / sizeof(float) != numberOfValues){ // Check the return array is properly sized
			rt_printf("Wrong array size\n");
			return;
		}*/
		
		for(int i = 0; i < numberOfValues; i++){
			(*returnArray)[i] = buffer[bufferReadPointer]; // Retrieve corresponding entry in the buffer and add to the array, ensuring the oldest entries are the lower elements. 
			buffer[bufferReadPointer] = 0; // Empty entry in buffer
			
			bufferReadPointer++; // Iterate and wrap bufferReadPointer
			if(bufferReadPointer >= bufferSize){
				bufferReadPointer = 0;
			}
		}
	}
	
	// Insert a sample into the buffer
	inline void insert(float entry){
		buffer[(bufferWritePointer)] += entry;
		bufferWritePointer++;
		if(bufferWritePointer >= bufferSize){
			bufferWritePointer = 0;
		}
	}
	
private:
	int bufferSize;
	int bufferWritePointer;
	int bufferReadPointer;
	float* buffer; 
};

#endif //CIRCULARBUFFER_H
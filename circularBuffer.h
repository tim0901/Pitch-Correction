/***** circularBuffer.h *****/
/*
 * Alex Richardson 2020
 */
 
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <cstdint>

// A circular buffer

class circularBuffer{
public:
	circularBuffer(){} // Default constructor
	
	~circularBuffer(){ // Destructor
		if(buffer){
			free(buffer);
		}
		rt_printf("Circular Buffer deleted.\n");
	}
	
	circularBuffer(int bufSize):bufferSize(bufSize){ // Constructor
		buffer = (float*)malloc(bufferSize * sizeof(float)); // Initialize array
		bufferWritePointer = 0; 
	}
	
	// Returns the desired element from the array
	inline float returnElement(int element){
		if(element > bufferSize){
			element = (element + bufferSize)%bufferSize;
		}
		return buffer[element];
	}
	
	// Returns the desired element from the array and empties the buffer element
	inline float returnAndEmptyElement(int element){
		if(element > bufferSize){
			element = (element + bufferSize)%bufferSize;
		}
		float temp = buffer[element];
		buffer[element] = 0.0;
		return temp;
	}
	
	// Manually change the write pointer - used for offsetting in setup
	inline void setWritePointer(int element){
		bufferWritePointer = element;
	}
	
	// Returns the write pointer
	inline int returnWritePointer(){
		return bufferWritePointer;
	}
	
	// Insert a sample into the buffer
	inline void insert(float entry){
		buffer[(bufferWritePointer)] = entry;
		bufferWritePointer++;
		if(bufferWritePointer >= bufferSize){
			bufferWritePointer = 0;
		}
	}
	
	// Add a sample to whatever exists in a given buffer element
	inline void insertAndAdd(float entry){
		buffer[(bufferWritePointer)] += entry;
		bufferWritePointer++;
		if(bufferWritePointer >= bufferSize){
			bufferWritePointer = 0;
		}
	}
	
private:
	int bufferSize;
	int bufferWritePointer;
	float* buffer; 
};

#endif //CIRCULARBUFFER_H
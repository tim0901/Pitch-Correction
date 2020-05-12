/***** circularBuffer.h *****/
/*
 * Alex Richardson 2020
 */
 
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

// A circular buffer

class circularBuffer{
public:
	
	circularBuffer(int bufSize):bufferSize(bufSize){ // Constructor
		buffer = (float*)malloc(bufferSize * sizeof(float)); // Initialize array
	}
	
	~circularBuffer(){ // Destructor
		if(buffer){
			free(buffer);
		}
		rt_printf("Circular Buffer deleted.\n");
	}
	
	// Return the desired element from the array
	inline float returnElement(int element){
		
		// Ensure element is within bounds
		while(element < 0){
			element += bufferSize;
		}
		element = element%bufferSize;
		
		return buffer[element];
	}
	
	// Return the next sequential element according to bufferReadPointer
	inline float returnNextElement(){
		float temp = this->returnElement(bufferReadPointer);
		
		// Iterate and wrap bufferReadPointer
		bufferReadPointer++;
		if(bufferReadPointer >= bufferSize){
			bufferReadPointer = 0;
		}
		return temp;
	}
	
	// Return the desired element from the array and empty the buffer element
	inline float returnAndEmptyElement(int element){
		
		float temp = this->returnElement(element);
		buffer[element] = 0.0;
		return temp;
	}
	
	// Return the next sequential element according to bufferReadPointer and empty the buffer element
	inline float returnAndEmptyNextElement(){
		float temp = this->returnAndEmptyElement(bufferReadPointer);
		
		// Iterate and wrap bufferReadPointer
		bufferReadPointer++;
		if(bufferReadPointer >= bufferSize){
			bufferReadPointer = 0;
		}
		return temp;
	}
	
	// Manually change the read pointer
	inline void setReadPointer(int element){
		while(element < 0){
			element += bufferSize;
		}
		bufferReadPointer = element % bufferSize;
	}
	
	// Manually change the write pointer
	inline void setWritePointer(int element){
		while(element < 0){
			element += bufferSize;
		}
		bufferWritePointer = element % bufferSize;
	}
	
	// Returns the size of the buffer
	inline int size(){
		return bufferSize;
	}
	
	// Return the value of the read pointer
	inline int returnReadPointer(){
		return bufferReadPointer;
	}
	
	// Return the value of the write pointer
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
	const int bufferSize;
	int bufferReadPointer = 0;
	int bufferWritePointer = 0;
	float* buffer; 
};

#endif //CIRCULARBUFFER_H
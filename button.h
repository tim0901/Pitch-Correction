/***** button.h *****/
/*
 * Written for ECS7012U Music and 
 * Audio Programming, for the Bela platform.
 *
 * Alex Richardson 2020
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <Bela.h>


// Helper class for buttons, to keep things tidy
// Uses a state machine to debounce the input
class button{
public:
	button(BelaContext *context, int pin):pinNumber(pin){ // Constructor
		// Set LED pin to an output
		pinMode(context, 0, pinNumber, INPUT);
	}
	~button(){ // Destructor
		rt_printf("Button deleted\n");
	}
	
	// Update stored information
	// Should be called once per call to render. Can specify audioFrame if desired
	void updateState(BelaContext *context, int audioFrame = 0);
	
	// Return the current state of the button. FALSE == pressed
	bool returnState(){
		return buttonState;
	}
	
	// Return the previous state of the button. FALSE == pressed
	bool returnPreviousState(){
		return previousButtonState;
	}
	
private:
	const int pinNumber; // The Bela pin the button is connected to
	bool buttonState; // Most recent state of the button
	bool previousButtonState; // Previous state of the button
	
	// State machine states
	enum {
		kStateOpen = 0,
		kStateJustClosed,
		kStateClosed,
		kStateJustOpen
	};
	
	int debounceState = kStateOpen; // Current state of the state machine
	int debounceCounter = 0;
	int debounceInterval = 882;	// 20 ms
	
};

void button::updateState(BelaContext *context, int audioFrame){
	
	// Read the button state
  	buttonState = digitalRead(context, audioFrame, pinNumber);
  	
  	// For each state, look for the appropriate inputs and take 
  	// the appropriate actions on transition to a new state
  	if(debounceState == kStateOpen) {
  		// Button is not pressed
  		// Input: look for switch closure
  		if(buttonState == 0){
  			debounceState = kStateJustClosed;
		}
   	}
   	else if(debounceState == kStateJustClosed) {
   		// Button was just pressed, wait for debounce
   		// Input: run counter, wait for timeout
   		if(debounceCounter < debounceInterval){
   			debounceCounter++;
   		}
   		else{
   			debounceCounter = 0;
   			debounceState = kStateClosed;
   		}
   	}
   	else if(debounceState == kStateClosed) {
   		// Button is pressed, could be released anytime
   		// Input: look for switch opening
   		if(buttonState ==1){
   			debounceState = kStateJustOpen;
   		}
   	}
   	else if(debounceState == kStateJustOpen) {
   		// Button was just released, wait for debounce
   		// Input: run counter, wait for timeout
   		if(debounceCounter < debounceInterval){
   			debounceCounter++;
   		}
   		else{
   			debounceCounter = 0;
   			debounceState = kStateOpen;
   		}
   	}

   	// Update the previous button state
   	previousButtonState = buttonState;
}

#endif //BUTTON_H
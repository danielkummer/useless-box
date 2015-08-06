#ifndef DebounceButton_h
#define DebounceButton_h

#include "Arduino.h"

class DebounceButton
{
  public: 
    DebounceButton(byte pin);
    int read();
    boolean on();
  private:
    byte _pin;       
    
    int _state;             // the current reading from the input pin
    int _lastState;   // the previous reading from the input pin

  // the following variables are long's because the time, measured in miliseconds,
  // will quickly become a bigger number than can be stored in an int.
    long _lastDebounceTime ;  // the last time the output pin was toggled
    long _debounceDelay ;    // the debounce time; increase if the output flicker
};

#endif

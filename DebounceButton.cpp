#include "DebounceButton.h"

DebounceButton::DebounceButton(byte pin) 
{
  _pin = pin;
  _lastState = LOW;
  _lastDebounceTime = 0;
  _debounceDelay = 50;
  
  pinMode(_pin, INPUT);  
}

boolean DebounceButton::on() 
{
  return read() == HIGH;
}

int DebounceButton::read() 
{
  int reading = digitalRead(_pin);
  
  if (reading != _lastState) {
    // reset the debouncing timer
    _lastDebounceTime = millis();
  } 
  
  if ((millis() - _lastDebounceTime) > _debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != _state) {
      _state = reading;      
    }
  }
  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  _lastState = reading;
  
  return reading;  
}

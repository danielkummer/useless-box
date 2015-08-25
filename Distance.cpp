#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Distance.h" 

Distance::Distance() {}

void Distance::attach(int pin, int read_time) {
  this->pin = pin;
  this->read_time = read_time;
}

void Distance::attach(int pin) {
  attach(pin, 200);
}

/** 
 *  Note: This method blocks for the specified read_time amount
 */
boolean Distance::detect(int threshold) {
  int currentDistance;  
  delay(1200);    //wait to stabilize sensor readings after opening the door

  // set the timer and read the sensor     
  lastDistance = analogRead(pin);
  timer = millis();

  // wait for movement that exceeds threshold or if timer expires,
  while(millis() - timer <= this->read_time) {
    currentDistance = analogRead(pin);
  
    //Does the current distance deviate from the last distance by more than the threshold?        
    if ((currentDistance > lastDistance + threshold || currentDistance < lastDistance - threshold) || currentDistance > 300) {
      return true;
    }
    lastDistance = currentDistance;
  }
  return false;           
}


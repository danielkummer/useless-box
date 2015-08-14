#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Distance.h" 

Distance::Distance() {}

void Distance::attach(int pin, int threshold) {
  this->pin = pin;
  this->threshold = threshold;
}

void Distance::attach(int pin) {
  attach(pin, 200);
}

boolean Distance::detect()
{
  int currentDistance;
  
    delay(1200);    //wait to stabilize sensor readings after opening the door

     // set the timer and read the sensor
     
     lastDistance= analogRead(pin);
     timer= millis();

     // wait for movement that exceeds threshold or if timer expires (5 sec),
     while(millis() - timer <= 5000) {
        currentDistance = analogRead(pin);

        //Does the current distance deviate from the last distance by more than the threshold?        
        if ((currentDistance > lastDistance + threshold || currentDistance < lastDistance - threshold) || currentDistance > 300) {
          return true;
        }
        lastDistance = currentDistance;
     }
     return false;           
}


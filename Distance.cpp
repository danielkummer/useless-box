#include "Distance.h"


Distance::Distance(byte pin)
{
  Distance(pin, 200);
}

Distance::Distance(byte pin, int threshold)
{
  _pin = pin;
  _threshold = threshold;   
}


boolean Distance::detect()
{
  int currentDistance;
  
    delay(1200);    //wait to stabilize sensor readings after opening the door

     // set the timer and read the sensor
     
     _lastDistance= analogRead(_pin);
     _timer= millis();

     // wait for movement that exceeds threshold or if timer expires (5 sec),
     while(millis() - _timer <= 5000) {
        currentDistance = analogRead(_pin);

        //Does the current distance deviate from the last distance by more than the threshold?        
        if ((currentDistance > _lastDistance + _threshold || currentDistance < _lastDistance - _threshold) || currentDistance > 300) {
          return true;
        }
        _lastDistance = currentDistance;
     }
     return false;           
}


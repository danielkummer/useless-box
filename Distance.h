#ifndef Distance_h
#define Distance_h

#include "Arduino.h"

class Distance
{
  public: 
    Distance(byte pin);
    Distance(byte pin, int threshold);
    boolean detect();    
  private:
    byte _pin;
    int _lastDistance;
    int _threshold;
    unsigned long _timer;
};

#endif

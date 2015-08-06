#ifndef MotorControl_h
#define MotorControl_h

#include "Arduino.h"

class MotorControl
{
  public: 
    MotorControl(byte directionPin, byte throttlePin);
    void forward(int speed);
    void backward(int speed);
    void halt();
  private:
    byte _directionPin;
    byte _throttlePin;         
};

#endif

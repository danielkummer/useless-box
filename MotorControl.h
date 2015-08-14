#ifndef MotorControl_h
#define MotorControl_h

#include <inttypes.h>

class MotorControl
{
  public: 
    MotorControl();
    void attach(int directionPin, int throttlePin);
    void forward(int speed);
    void backward(int speed);
    void halt();   
  protected:
    uint8_t directionPin;
    uint8_t throttlePin;         
};

#endif

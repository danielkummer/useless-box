#include "MotorControl.h"


MotorControl::MotorControl(byte directionPin, byte throttlePin)
{
  _directionPin = directionPin;
  _throttlePin = throttlePin;
  pinMode(directionPin, OUTPUT);  
}

void MotorControl::forward(int speed)
{
  digitalWrite(_directionPin, HIGH); //Establishes LEFT direction of Channel B  
  analogWrite(_throttlePin, speed);   //Spins the motor on Channel B at half speed
}

void MotorControl::backward(int speed)
{
  digitalWrite(_directionPin, LOW); //Establishes LEFT direction of Channel B  
  analogWrite(_throttlePin, speed);   //Spins the motor on Channel B at half speed
}

void MotorControl::halt()
{
  analogWrite(_throttlePin, 0);   //Spins the motor on Channel B at half speed
}


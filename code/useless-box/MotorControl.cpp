#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "MotorControl.h" 

MotorControl::MotorControl() {}

void MotorControl::forward(int speed){
  //digitalWrite(directionPin, HIGH); //Establishes LEFT direction of Channel B  
  //analogWrite(throttlePin, speed);   //Spins the motor on Channel B at half speed
}

void MotorControl::attach(int directionPin, int throttlePin) {
  this->directionPin = directionPin;
  this->throttlePin = throttlePin;
  pinMode(directionPin, OUTPUT);  
}

void MotorControl::backward(int speed)
{
  //digitalWrite(directionPin, LOW); //Establishes LEFT direction of Channel B  
  //analogWrite(throttlePin, speed);   //Spins the motor on Channel B at half speed
}

void MotorControl::halt()
{
  analogWrite(throttlePin, 0);   //Spins the motor on Channel B at half speed
}


#include "ServoControl.h"

ServoControl::ServoControl(byte pin,  byte startPos, byte endPos, byte pausePos) 
{
  _startPos = startPos;
  _endPos = endPos;
  _pausePos = pausePos;
  
  _defaultInterval = 3;
  _defaultSpeed = 15;

  //just to be on the safe side...
  _customPos = _startPos;
  
  _servo.attach(pin);
  _servo.write(_startPos);   
}

void ServoControl::move(Direction direction) 
{
  move(direction, _defaultInterval, _defaultInterval);
}

void ServoControl::move(Direction direction, int interval, int speed) 
{
  switch(direction) {
    case START_END:
      move(_startPos, _endPos, interval, speed);
      break;
    case START_PAUSE:
      move(_startPos, _pausePos, interval, speed);  
      break;        
    case END_START:
      move(_endPos, _startPos, interval, speed);
      break;
    case END_PAUSE:
      move(_endPos, _pausePos, interval, speed);
      break;
    case PAUSE_START:  
      move(_pausePos, _startPos, interval, speed);
      break;
    case PAUSE_END:
      move(_pausePos, _endPos, interval, speed);
      break;
    case CUSTOM_START:
      move(_customPos, _startPos, interval, speed);
      break;
    case CUSTOM_END:
      move(_customPos, _endPos, interval, speed);
      break;
    default: 
      Serial.println("unknown direction");
    break;  
  } 
}


void ServoControl::move(byte from, byte to, int interval, int speed) 
{
  byte pos = 0;
  if(from > to) {
    for(pos = from; pos >= to; pos -= interval) {
      _servo.write(pos);
      delay(speed);
    }      
  } else {
    for(pos = from; pos < to; pos += interval) {
      _servo.write(pos);
      delay(speed);
    }
  }  
}

void ServoControl::setTo(byte position)
{ 
  _customPos = position;
  _servo.write(position);
}

byte ServoControl::getEndPos()
{
  return _endPos;
}


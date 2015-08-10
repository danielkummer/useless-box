#include "ServoControl.h"

ServoControl::ServoControl(byte pin,  byte startPos, byte endPos, byte pausePos) 
{
  _startPos = startPos;
  _endPos = endPos;
  _pausePos = pausePos;
  
  _defaultInterval = 3;
  _defaultSpeed = 30;

  //just to be on the safe side...
  _customPos = _startPos;
  
  _servo.attach(pin);
  Serial.print("Start position ");
  Serial.println(_startPos);
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
  Serial.print("Move from ");
  Serial.print(from);
  Serial.print(" to ");
  Serial.print(to);  
  Serial.println("");

// Check the last command we gave to the servo
  int current_degree = _servo.read();
  //last_write = current_degree;

  while (current_degree != to) {
    if (current_degree < to) {
      current_degree++;
    }
    else {
      current_degree--;
    }

    _servo.write(current_degree);
    delay(speed);    
  }
  
  /*byte pos = 0;
  if(from > to) {
    for(pos = from; pos >= to; pos -= interval) {
      Serial.print("Write: ");
      Serial.println(pos);
      _servo.write(pos);
      delay(speed);
    }      
  } else {
    for(pos = from; pos < to; pos += interval) {
      Serial.print("Write: ");
      Serial.println(pos);
      _servo.write(pos);
      delay(speed);
    }
  } */ 
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


void ServoControl::goHome()
{
  //TODO read servo position and goto start gracefully
  byte currentPos = _servo.read();
  move(currentPos, _startPos, 3, 15);
}


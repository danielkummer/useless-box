#ifndef ServoControl_h
#define ServoControl_h

#include "Arduino.h"
#include <Servo.h>



enum Direction {
  START_END,
  START_PAUSE,
  END_START,
  END_PAUSE,
  PAUSE_START,
  PAUSE_END,
  CUSTOM_START,
  CUSTOM_END  
};

class ServoControl
{
  public: 
    ServoControl(byte pin, byte startPos, byte endPos, byte pausePos);
    void move(Direction direction);
    void move(Direction direction, int interval, int speed);    
    void setTo(byte position);    
    void move(byte from, byte to, int interval, int pause);
    byte getEndPos();
  private:
    Servo _servo;
    byte _startPos;
    byte _endPos;
    byte _pausePos;
    byte _customPos;
    int _defaultInterval;
    int _defaultSpeed;
    
    
};

#endif

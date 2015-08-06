#ifndef Moves_h
#define Moves_h

#include "Arduino.h"
#include "ServoControl.h"
#include "MotorControl.h"
#include "Distance.h"

class Moves
{
  public: 
    Moves(ServoControl* doorServo, ServoControl* handServo, ServoControl* flagServo, Distance* distance, MotorControl* motor, boolean randomize);
    void next();
    void goHome();
    void switchOff();
    void pauseBetween();
    void crazyDoor();
    void crazySlow();
    void move4(); //todo rename
    void move5(); //todo rename
    void matrix(); //todo rename
    void sneak(); //todo rename
    void driveAway();
    void whiteFlag();
  private:
    byte _selectedMove;
    byte _maxMove;  
    boolean _randomize;
    ServoControl*  _doorServo;
    ServoControl*  _handServo;
    ServoControl*  _flagServo;
    Distance*      _distance;
    MotorControl*  _motor;
    
    byte selectNextMove();
    void waveFlag(int times);    
};

#endif

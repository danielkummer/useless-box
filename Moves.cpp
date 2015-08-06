#include "Moves.h"

Moves::Moves(ServoControl* doorServo, ServoControl* handServo, ServoControl* flagServo, Distance* distance, MotorControl* motor, boolean randomize)
{
  _doorServo = doorServo;
  _handServo = handServo;  
  _flagServo = flagServo;
  _distance = distance;
  _randomize = randomize;  
  _motor = motor;
  
  _selectedMove = 0;
  _maxMove = 10;
}

/* - other cases:
 
 * - close door when user comes to near
 * - turn off repeatedly
 * - find out other cases
 * - shy : turn off, then close door a bit when user comes near, then drive away slowly, then open door again, do this 2 times then turn off
 * 
 */

void Moves::next()
{
  Serial.println("Doing move no. " + _selectedMove);  
  switch(_selectedMove) {
    case 1: pauseBetween(); break;
    case 2: crazyDoor(); break;
    case 3: crazySlow(); break;
    case 4: move4(); break;    
    case 5: move5();  break;    
    case 6: matrix();  break;    
    case 7: sneak();  break;    
    case 8: driveAway(); break;    
    case 9: whiteFlag(); break;        
    default: switchOff(); break;
  }  
  
  _selectedMove = selectNextMove();    
}

byte Moves::selectNextMove() {
  if(_randomize) {
    _selectedMove = random(0, _maxMove);
  } else {
    if(_selectedMove < _maxMove) {
      _selectedMove++;      
    } else {
      _selectedMove = 0;
    }
  }  
}



void Moves::switchOff()
{
  _doorServo->move(START_END);      
  _handServo->move(START_END);
  _handServo->move(END_START);  
  _doorServo->move(END_START);
}

void Moves::pauseBetween() 
{
  _doorServo->move(START_END);
  delay(800);
  _handServo->move(START_PAUSE);
  delay(1000);
  _handServo->move(PAUSE_END);
  _handServo->move(END_START);
  _doorServo->move(END_START);    
}

void Moves::crazyDoor()
{
  _doorServo->move(START_END);
  _doorServo->move(END_START, 5, 15);
  _doorServo->move(START_PAUSE);
  _doorServo->move(PAUSE_START, 15, 15);
  delay(700);
  _doorServo->move(START_END);
  delay(700);
  _doorServo->move(END_START, 5, 15);
  
  //now switch off
  _doorServo->move(START_END, 8, 15);
  _handServo->move(START_END);
  _handServo->move(END_START);
  _doorServo->move(END_START, 15, 15);  
}

void Moves::crazySlow()
{
  _doorServo->move(START_END, 1, 30);
  _handServo->move(START_END, 1, 30);
  _handServo->move(END_START, 1, 30);  
  _doorServo->move(END_PAUSE, 1, 30);
  _doorServo->move(PAUSE_END, 4, 15);
}

void Moves::move4()
{
  //m7anika7anika
  _doorServo->move(PAUSE_END, 3, 15);
  _handServo->move(START_PAUSE, 1, 15);
  delay(800);
  _doorServo->move(END_PAUSE, 3, 15);
  _doorServo->move(PAUSE_END, 3, 15);

  _handServo->setTo(40);
  delay(1000);
  _handServo->move(CUSTOM_END, 4, 15);
  delay(1000);
  _handServo->move(END_START, 4, 15);
  _doorServo->move(END_START, 1, 15);
}

void Moves::move5()
{
  //m3alla2
  _doorServo->move(START_END, 3, 15);
  _handServo->move(START_END, 4, 15);
  _doorServo->move(END_PAUSE,3,15);
  _doorServo->move(PAUSE_END,3,15);
  _doorServo->move(END_PAUSE,3,15);
  _doorServo->move(PAUSE_END,3,15);
  _handServo->move(END_START, 4, 15);
  _doorServo->move(END_START,3,15);  
}

void Moves::matrix()
{
  _doorServo->move(START_END, 3, 15);
  _handServo->move(START_PAUSE, 4, 15);  
  _handServo->move(PAUSE_END, 1, 30);
  delay(300);
  _handServo->move(END_START, 4, 10);
  _doorServo->move(END_START, 3, 15);  
}

void Moves::sneak()
{
  _doorServo->move(START_PAUSE, 1, 15);
  delay(2000);
  _handServo->move(START_PAUSE, 1, 30);
  delay(500);
  _doorServo->move(PAUSE_END, 4, 15);
  delay(100);
  //it gets quite custom here...
  _handServo->move(40,90,4,15);
  delay(500);
  _handServo->move(90,70,4,15);
  delay(100);
  _handServo->move(70,90,4,15);
  delay(100);
  _handServo->move(90,70,4,15);
  delay(100);
  _handServo->move(70, _handServo->getEndPos(), 4, 15);
  _handServo->move(END_START, 4, 15);
  _doorServo->move(END_START, 3, 15);
}

void Moves::driveAway()
{
  //this move will open the door and wait for movement, if detected near the switch,
   //the box will move first to the Right, and then to the Left, and finally the hand moves and turns off the switch
  _doorServo->move(START_END, 3, 15);
  delay(1200);
  if(_distance->detect()) {
    //todo drive away
    _motor->forward(100);
    delay(300);
    _motor->halt();
  }
  delay(100);
  _handServo->move(START_END, 4, 15);
  _handServo->move(END_START, 4, 15);
  _doorServo->move(END_START, 3, 15);
}

void Moves::whiteFlag() 
{
  _doorServo->move(START_END, 3, 15);  
  waveFlag(5);
  _handServo->move(START_END, 4, 15);
  _handServo->move(END_START, 4, 15);

  if(_distance->detect()) {
    waveFlag(7);
  }
  
  _doorServo->move(END_START, 3, 15); 
}

void Moves::waveFlag(int times)
{
  byte i;
  _flagServo->move(START_PAUSE, 3, 15);
  delay(100);
  for (i = 0; i < times; i++) {
    _flagServo->move(PAUSE_END, 4, 15);
    _flagServo->move(END_PAUSE, 4, 15); 
  }
  delay(100);
  _flagServo->move(PAUSE_START, 3, 15);
}



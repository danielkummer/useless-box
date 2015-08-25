#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "ServoControl.h"

#ifdef ENABLE_PRINT
  #define Sprintln(a) (Serial.println(a))
  #define Sprint(a) (Serial.print(a))
#else   
  #define Sprintln(a)
  #define Sprint(a)
#endif

#define MECANIC_SPEED_PER_DEGREE 5 // 2 msec to move 1 degree #define MECANIC_SPEED_PER_DEGREE 2 // 2 msec to move 1 degree

ServoControl::ServoControl(char name[]) {
  this->name = name;
  this->current_speed = 0;
  this->pos_home = 15; //set the home position to 15 deg, just an arbitary value, must be set in the attack method
  this->servo = Servo();
  this->is_interruptable = true;
}

void ServoControl::attach(Bounce* bouncer, int pin, int pos_home, int pos_max) {
  this->pin = pin;
  this->pos_home = pos_home;
  this->pos_max = pos_max;
  this->bouncer = bouncer;
  this->servo.write(pos_home);
  this->servo.attach(pin);
}

void ServoControl::interruptable(bool interruptable) {
  this->is_interruptable = interruptable;
}

bool ServoControl::move(int degree, int speed) {
  this->current_speed = speed;
  return move(degree);
}

bool ServoControl::move(int degree) {

  bool interrupted = false;

  Sprint(F("Moving "));
  Sprint(F(this->name));
  Sprint(F(" to position : "));
  Sprint(F(degree));
  Sprintln(F("deg."));

  bouncer->update();
  int switch_value_before_move = bouncer->read();

  // Check the last command we gave to the servo
  int current_degree = servo.read();
  last_write = current_degree;

  // And then moves ! degree by degree, checking the switch position each time
  //while (current_degree != degree) {
  while (current_degree != degree) {
    if (current_degree < degree) {
      current_degree++;
    } else {
      current_degree--;
    }
    servo.write(current_degree);
    delay(MECANIC_SPEED_PER_DEGREE + this->current_speed);

    // Read switch again in loop so we can react if something changes
    bouncer->update();
    int current_value = bouncer->read();
    if (this->is_interruptable && current_value != switch_value_before_move) {
      //a bit unlucky but i can't really rely on the arm switch of position, so I'm going for a "soft" trigger here
      if (strcmp(this->name, "arm") != 0 || current_degree < 160) {        
        Sprint(this->name);
        Sprint(F(" : "));
        Sprintln(F("/!\\ Interrupted - The switch was operated while I was moving !"));
        interrupted = true;
        break;
      }      
    }
  }
  return interrupted;
}

uint8_t ServoControl::getLastWrite() {
  return this->last_write;
}

void ServoControl::waitAndDetatch() {
  if (this->servo.read() == pos_home && this->is_home == false) {
    Sprint(F("Powering off the "));
    Sprint(this->name);
    Sprintln(F(" servo ..."));
    int time_to_wait = abs(this->last_write - pos_home) * (this->current_speed + MECANIC_SPEED_PER_DEGREE);
    delay(time_to_wait);
    this->is_home = true;
    servo.detach();
  }
}

bool ServoControl::isHome() {
  return this->is_home;  
}

void ServoControl::isHome(bool home) {
  this->is_home = home;
}

void ServoControl::setHome(int pos_home) {  
  this->pos_home = pos_home;
}

void ServoControl::reattach() {
  servo.attach(pin);
}


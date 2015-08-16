#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "ServoControl.h" 

#define MECANIC_SPEED_PER_DEGREE 2 // 2 msec to move 1 degree #define MECANIC_SPEED_PER_DEGREE 2 // 2 msec to move 1 degree

ServoControl::ServoControl(char name[]) {
  this->name = name;
  this->current_speed = 0;
  this->pos_home = 15; //set the home position to 15 deg, just an arbitary value, must be set in the attack method
  this->servo = Servo();
}

void ServoControl::attach(Bounce* bouncer, int pin, int pos_home) {  
  this->pin = pin;
  this->pos_home = pos_home;
  this->bouncer = bouncer;  
  this->servo.write(pos_home);
  this->servo.attach(pin);
    
}

bool ServoControl::move(int degree, int speed) {
  this->current_speed = speed;
  return move(degree);
}

bool ServoControl::move(int degree) {

  bool interrupted = false;
  
  Serial.print("Moving ");
  Serial.print(this->name);
  Serial.print("to position : ");
  Serial.print(degree);
  Serial.println("deg.");
  
  
  bouncer->update();
  int first_switch_value = bouncer->read();

  // Check the last command we gave to the servo
  int current_degree = servo.read();
  last_write = current_degree;

  // And then moves ! degree by degree, checking the switch position each time
  while (current_degree != degree) {
    if (current_degree < degree) {
      current_degree++;
    }
    else {
      current_degree--;
    }

    servo.write(current_degree);
    delay(MECANIC_SPEED_PER_DEGREE + this->current_speed);

    // Read switch again in loop so we can react if something changes
    bouncer->update();
    int current_value = bouncer->read();
    if (current_value != first_switch_value && this->name) {
      if(strcmp(this->name, "arm") != 0 ) {
      Serial.print(this->name);
      Serial.print(" : ");
      Serial.println("/!\\ Interrupted - The switch was operated while I was moving !");
      interrupted = true;
      }
      break;
    }
  }

  return interrupted;
}

uint8_t ServoControl::getLastWrite() {
  return this->last_write;  
}

void ServoControl::waitAndDetatch() {
  if (this->servo.read() == pos_home && this->is_home == false) {
    Serial.print("Powering off the" );  
    Serial.print(this->name);
    Serial.println(" servo ...");
    int time_to_wait = abs(this->last_write-pos_home)*(this->current_speed + MECANIC_SPEED_PER_DEGREE);
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

void ServoControl::reattach() {
  servo.attach(pin);
}


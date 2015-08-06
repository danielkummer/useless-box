#include <Servo.h>
#include "ServoControl.h"
#include "MotorControl.h"
#include "DebounceButton.h"
#include "Distance.h"
#include "Moves.h"

DebounceButton button(2);
ServoControl door(3, 120, 2, 100);
ServoControl hand(5, 15, 135, 110);
ServoControl flag(6, 60, 160, 140);
MotorControl motor(13, 11);
Distance distance(3);

Moves moves(&door, &hand, &flag, &distance, &motor, false);

void setup() {
  Serial.begin(9600);
}

void loop() {
  
  if (button.on()) {
    Serial.println("switch: on - start the game");
    moves.next();
  }
}





#include <Bounce2.h>

#include <Servo.h>
#include "ServoControl.h"
#include "MotorControl.h"

//#include "DebounceButton.h"
#include "Distance.h"
#include "Moves.h"

//DebounceButton button(2);
ServoControl door(5, 120, 2, 100);
ServoControl hand(8, 15, 135, 110);
ServoControl flag(6, 60, 160, 140);
MotorControl motor(13, 11);
Distance distance(3);


Bounce  bouncer  = Bounce(); 

Moves moves(&door, &hand, &flag, &distance, &motor, false);

void setup() {
  pinMode( 2 ,INPUT);
  bouncer.attach( 2 ); 
  bouncer.interval(5);
  Serial.begin(9600);

  door.goHome();
  hand.goHome();
  flag.goHome();
}

void loop() {

// Update the switch value.
  bouncer.update();
  int val = bouncer.read();
  
  if (val == HIGH) {
  //if (button.on()) {
    Serial.println("switch: on - start the game");
    moves.next();
    delay(1000);
  }
}





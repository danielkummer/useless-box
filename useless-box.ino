#include <Servo.h>
#include <Bounce2.h>

#include "MotorControl.h"
#include "Distance.h"
#include "ServoControl.h"

#define NUM_BEHAVIOURS 13

/*
 * Servo positions
 */ 
#define MIN_ARM_DEG 15
#define MAX_ARM_DEG 180
#define POS_HOME_ARM MIN_ARM_DEG
#define POS_SWITCH_ARM MAX_ARM_DEG
#define POS_ARM_PEEK (MAX_ARM_DEG - 40)
#define POS_ARM_CHECK_NINJA (MIN_ARM_DEG + 20)
#define POS_ARM_NEAR_SWITCH (MIN_ARM_DEG + 40)

#define MIN_DOOR_DEG 2
#define MAX_DOOR_DEG 120
#define POS_HOME_DOOR MIN_DOOR_DEG
#define POS_DOOR_MEDIUM_OPEN (MAX_DOOR_DEG - 20)
#define POS_DOOR_FULL_OPEN MAX_DOOR_DEG
#define POS_DOOR_CHECK_NINJA (MIN_DOOR_DEG + 40)

#define MIN_FLAG_DEG 60
#define MAX_FLAG_DEG 160
#define POS_FLAG_HIDDEN MIN_FLAG_DEG
#define POS_FLAG_RAISED MAX_FLAG_DEG
#define POS_FLAG_TILTED (MAX_FLAG_DEG - 40)

// the number of msec (approx.) for the servo to go one degree further
#define MECANIC_SPEED_PER_DEGREE 2 // 2 msec to move 1 degree
// the delay between the arm hitting the switcha and the switch reporting a change in value 
#define MECANIC_DELAY_SWITCH 50 // 50 msec to acknowledge a hit

const int arm_servo_pin = 8;
const int door_servo_pin = 6;
const int flag_servo_pin = 5;

const int direction_pin = 13;
const int throttle_pin = 11;

const int distance_pin = 3;

const int box_switch  = 2;
Bounce bouncer = Bounce();
int activated = LOW;

int randBehaviour = 1;  // This is used to choose the next behaviour 
long randCheck = 1;     // Random check when not activated
boolean hasAlreadyChecked = true;
boolean interrupted = false; // Is set to true when the switch has been changed while doing something. Allows for a quick check in the loop() for what we should do next

int lastDistance = 0;
int threshold = 200;
unsigned long timer;

MotorControl motor = MotorControl();
Distance distance = Distance();
ServoControl newArm = ServoControl();
ServoControl newDoor = ServoControl();
ServoControl newFlag = ServoControl();


void setup() {  
  pinMode(box_switch, INPUT);
  pinMode(direction_pin, OUTPUT);  
  
  bouncer.attach(box_switch);

  motor.attach(direction_pin, throttle_pin);
  distance.attach(2);
  newArm.attach(&bouncer, arm_servo_pin, POS_HOME_ARM);
  newDoor.attach(&bouncer, door_servo_pin, POS_HOME_DOOR);
  newFlag.attach(&bouncer, flag_servo_pin, POS_FLAG_HIDDEN);

  Serial.begin(9600);
  Serial.println("Started.");
  
  //arm.attach(arm_servo_pin);
  //door.attach(door_servo_pin);
  //flag.attach(flag_servo_pin);
  
  newArm.move(POS_HOME_ARM);
  newDoor.move(POS_HOME_DOOR);
  newFlag.move(POS_FLAG_HIDDEN);
   
  randomSeed(analogRead(0));
}
// A "soft" delay function
// (This function can be interrupted by manual switch change)
void softDelay(int msec) {

  Serial.print("Delaying for ");
  Serial.print(msec);
  Serial.println(" msec...");
  
  // Update the switch value.
  bouncer.update();
  int val = bouncer.read();

  // Inits a counters
  long time_counter = 0;

  // And wait... msec by msec, checking the switch position each time
  do {
    delay(1);
    time_counter++;

    bouncer.update();
    int current_value = bouncer.read();

    if (current_value != val) {
     Serial.println("/!\\ Interrupted - The switch was operated while I was waiting on purpose !");
      interrupted = true;
      break;
    }
  } 
  while(time_counter <= msec);

}

boolean detect()
{
  int currentDistance;
  
    delay(1200);    //wait to stabilize sensor readings after opening the door

     // set the timer and read the sensor
     
     lastDistance= analogRead(distance_pin);
     timer= millis();

     // wait for movement that exceeds threshold or if timer expires (5 sec),
     while(millis() - timer <= 5000) {
        currentDistance = analogRead(distance_pin);

        //Does the current distance deviate from the last distance by more than the threshold?        
        if ((currentDistance > lastDistance + threshold || currentDistance < lastDistance - threshold) || currentDistance > 300) {
          return true;
        }
        lastDistance = currentDistance;
     }
     return false;           
}


/* ------------------------------- */
/* ---- STANDARD BEHAVIOURS ------ */
/* ------------------------------- */
void flipSwitch(int msec = 0) { 
  //current_arm_speed = msec; 
  newArm.move(POS_SWITCH_ARM, 0); 
  delay(MECANIC_DELAY_SWITCH); // Wait for the switch to acknowledge before returning in the loop
}

// Go near the arm, but not as close as to push the switch, and then retracts a bit
void tryFail(int msec = 5) {  
  //current_arm_speed = msec; 
  newArm.move(POS_ARM_NEAR_SWITCH, msec);
  newArm.move(POS_ARM_NEAR_SWITCH + 7, msec); 
}

// Open the lid stealthly. User does not see me. I'm a NINJA !
void goStealthCheck() {  
  //current_arm_speed = 30;   
  newArm.move(POS_ARM_CHECK_NINJA, 30);
  newArm.move(POS_ARM_NEAR_SWITCH + 7, 30);  
}

void openDoorNija() {
  //current_door_speed = 30; 
  newDoor.move(POS_DOOR_CHECK_NINJA, 30);
}

void openDoorMedium(int msec = 0) {
  //current_door_speed = msec;
  newDoor.move(POS_DOOR_MEDIUM_OPEN, 0);
  delay(MECANIC_DELAY_SWITCH);
}

void openDoorFull(int msec = 0) {
  //current_door_speed = msec;
  newDoor.move(POS_DOOR_FULL_OPEN, 0);
  delay(MECANIC_DELAY_SWITCH);
}

// Ok, back home now
void backHome() { 
  //current_arm_speed = 0; 
  newArm.move(POS_HOME_ARM, 0); 
}

// Open the lid to see out
void goCheck(int msec = 10) {  
  //current_arm_speed = msec;   
  newArm.move(POS_ARM_PEEK, msec); 
}

void closeDoor() {
  //current_door_speed = 0;
  newDoor.move(POS_HOME_DOOR, 0);
}
void raiseFlag(int msec = 0) {
  //current_flag_speed = msec;
  newFlag.move(POS_FLAG_RAISED, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void tiltFlag(int msec = 0) {
  //current_flag_speed = msec;
  newFlag.move(POS_FLAG_TILTED, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void hideFlag(int msec = 0) {
  //current_flag_speed = msec;
  newFlag.move(POS_FLAG_HIDDEN, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void waveFlag(int times)
{
  byte i;
  raiseFlag();  
  delay(100);
  for (i = 0; i < times; i++) {    
    tiltFlag();
    raiseFlag();
  }
  delay(100);
  hideFlag();
}

/**
 * interruptable methods - use these in switch statement
 */


// Go out and flip that switch the user just
void goFlipThatSwitch(int msec = 0) { 
  if (!interrupted) openDoorFull(msec);
  if (!interrupted) flipSwitch(msec);
  if (!interrupted) closeDoor();
}

 
void driveAway()
{
  if (!interrupted) openDoorMedium();
  if (!interrupted) delay(1200);
  if(!interrupted && detect()) {
    if (!interrupted) motor.forward(100);
    if (!interrupted) delay(300);
    if (!interrupted) motor.halt();
  }
  if (!interrupted) delay(100);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();  
}


void whiteFlag() 
{
  //is_flag_home = false;
  newFlag.isHome(false);
  
  if (!interrupted) openDoorFull();
  if (!interrupted) waveFlag(5);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) delay(2000);
  if(!interrupted && detect()) {
    if (!interrupted) waveFlag(7);
  }  
  if (!interrupted) closeDoor();
}




// Check once, wait, then flip the switch
void check() {
  if (!interrupted) openDoorFull();
  if (!interrupted) goCheck();
  if (!interrupted) softDelay(1000);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}


// Check, return back home, then flip
void checkReturn() {
  if (!interrupted) openDoorFull();
  if (!interrupted) goCheck();
  if (!interrupted) softDelay(1500);
  if (!interrupted) backHome();
  if (!interrupted) softDelay(800);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}


// Multi tries. At the end, succeeds...
void multiTry() {
  if (!interrupted) openDoorFull();
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) backHome();
  if (!interrupted) softDelay(500);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}

// Check once slowly, return, check again, return to stealth position, then flip
void checkCheckReturn() {
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(1500);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(1000);
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(800);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}

void afraid() {
  if (!interrupted) openDoorFull();
  if (!interrupted) tryFail(0);  
  if (!interrupted) goCheck(0);
  if (!interrupted) closeDoor();  
  if (!interrupted) softDelay(1500);
  if (!interrupted) openDoorMedium();
  if (!interrupted) goFlipThatSwitch(5);
  if (!interrupted) closeDoor();
}

// #OhWait 
void ohWait() {
  if (!interrupted) openDoorFull();
  if (!interrupted) tryFail(0);
  if (!interrupted) backHome();
  if (!interrupted) openDoorMedium();
  if (!interrupted) softDelay(700);
  if (!interrupted) goCheck(2); // Woops. Forgot something ?
  if (!interrupted) softDelay(1000);
  if (!interrupted) goFlipThatSwitch(15);
  if (!interrupted) closeDoor();
}

void matrix() {
  if (!interrupted) openDoorFull();
  if (!interrupted) goCheck();  
  if (!interrupted) flipSwitch(30);
  if (!interrupted) delay(300);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();  
}

void crazyDoor() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) closeDoor();
  if (!interrupted) delay(700);
  if (!interrupted) openDoorFull();    
  if (!interrupted) delay(700);  
  if (!interrupted) closeDoor();  
  if (!interrupted) goFlipThatSwitch(15);  
}

void crazySlow() {
  if (!interrupted) goFlipThatSwitch(30);  
}

void loop() {
  // Update the switch position
  bouncer.update();
  activated = bouncer.read();
  
  if (activated == HIGH) {

    Serial.println("Who turned me on?");

    newArm.isHome(false);
    newDoor.isHome(false);
    
    newArm.reattach();
    newDoor.reattach();
    newFlag.reattach();
       
    // Find a new behaviour for next time
    randBehaviour = random(1, NUM_BEHAVIOURS);
    
    switch(randBehaviour) { 
      case 1: goFlipThatSwitch(); break;
      case 2: check(); break;
      case 3: multiTry(); break;
      case 4: checkReturn(); break;
      case 5: checkCheckReturn(); break;
      case 6: afraid(); break;
      case 7: ohWait(); break;
      case 8: driveAway();break;
      case 9: whiteFlag(); break;
      case 10: matrix(); break;
      case 11: crazyDoor(); break;
      case 12: crazySlow(); break;        
      default: goFlipThatSwitch(); break;
    }

    interrupted = false; // Now that I have finished, I can rest.
    hasAlreadyChecked = false; // We can check once after that

    } else if (newArm.isHome() == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Going back home");
    backHome();
        
  } else if (newDoor.isHome() == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Closing door");
    closeDoor();
        
  } else if (newFlag.isHome() == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Hiding flag");
    hideFlag();
        
  } else if ( randCheck < 5 && hasAlreadyChecked == false) {      
      // We only check once after an activation
      hasAlreadyChecked = true;

      newArm.isHome(false);
      //is_arm_home = false;      
      
      Serial.println("Random check, baby. ");      
      
      newArm.reattach();
      newDoor.reattach();      
      
      if (!interrupted) goCheck(5);
      if (!interrupted) softDelay(1000);      
  }

  newArm.waitAndDetatch();
  newDoor.waitAndDetatch();
  newFlag.waitAndDetatch();

  // Random number to check sometimes
  randCheck = random(1, 1000000);

}

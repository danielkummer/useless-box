#include <Servo.h>
#include <Bounce2.h>

#include "MotorControl.h"
#include "Distance.h"
#include "ServoControl.h"

#define NUM_BEHAVIOURS 13

/*
 * Servo positions
 */ 
#define MIN_ARM_DEG 35
#define MAX_ARM_DEG 180
#define POS_HOME_ARM MIN_ARM_DEG
#define POS_SWITCH_ARM MAX_ARM_DEG
#define POS_ARM_PEEK (MIN_ARM_DEG + 50)
#define POS_ARM_CHECK_NINJA (MIN_ARM_DEG + 20)
#define POS_ARM_NEAR_SWITCH (MAX_ARM_DEG - 70)
//(MIN_ARM_DEG + 40)

#define MIN_DOOR_DEG 160
#define MAX_DOOR_DEG 40
#define POS_HOME_DOOR MIN_DOOR_DEG
#define POS_DOOR_MEDIUM_OPEN (MIN_DOOR_DEG - 90)
#define POS_DOOR_FULL_OPEN MAX_DOOR_DEG
#define POS_DOOR_CHECK_NINJA (MIN_DOOR_DEG - 80)

#define MIN_flag_DEG 60
#define MAX_flag_DEG 160
#define POS_flag_HIDDEN MIN_flag_DEG
#define POS_flag_RAISED MAX_flag_DEG
#define POS_flag_TILTED (MAX_flag_DEG - 40)

// the number of msec (approx.) for the servo to go one degree further
#define MECANIC_SPEED_PER_DEGREE 2 // 2 msec to move 1 degree
// the delay between the arm hitting the switcha and the switch reporting a change in value 
#define MECANIC_DELAY_SWITCH 50 // 50 msec to acknowledge a hit

const int arm_servo_pin = 9;
const int door_servo_pin = 6;
const int flag_servo_pin = 5;

const int direction_pin = 13;
const int throttle_pin = 11;

const int distance_pin = A1;

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
ServoControl arm = ServoControl("arm");
ServoControl door = ServoControl("door");
ServoControl flag = ServoControl("flag");


void setup() {  
  pinMode(box_switch, INPUT);
  pinMode(direction_pin, OUTPUT);  
  pinMode(distance_pin,INPUT);
  
  Serial.begin(9600);
  Serial.println("Started."); 
  Serial.println("Initializing positions.");

  bouncer.attach(box_switch);
  motor.attach(direction_pin, throttle_pin);
  distance.attach(2);
  arm.attach(&bouncer, arm_servo_pin, POS_HOME_ARM);
  door.attach(&bouncer, door_servo_pin, POS_HOME_DOOR);
  flag.attach(&bouncer, flag_servo_pin, POS_flag_HIDDEN);

  
   
  Serial.println("Initializing random number . different behaviour every start."); 
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
     
     lastDistance = analogRead(distance_pin);
     timer = millis();
     
     // wait for movement that exceeds threshold or if timer expires (5 sec),
     while(millis() - timer <= 5000) {
        currentDistance = analogRead(distance_pin);
             Serial.print("currentDistance:" );
              Serial.print(currentDistance);
              Serial.print(" lastDistance: ");
              Serial.println(lastDistance);
              delay(500);


        

        //Does the current distance deviate from the last distance by more than the threshold?        
        if ((currentDistance > lastDistance + threshold || currentDistance < lastDistance - threshold)) {
          Serial.print("Detected! currentDistance:" );
          Serial.print(currentDistance);
          Serial.print(" lastDistance: ");
          Serial.println(lastDistance);
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
  arm.move(POS_SWITCH_ARM, msec); 
  delay(MECANIC_DELAY_SWITCH); // Wait for the switch to acknowledge before returning in the loop
}

// Go near the arm, but not as close as to push the switch, and then retracts a bit
void tryFail(int msec = 5) {  
  arm.move(POS_ARM_NEAR_SWITCH, msec);
  arm.move(POS_ARM_NEAR_SWITCH + 7, msec); 
}

// Open the lid stealthly. User does not see me. I'm a NINJA !
void goStealthCheck() {    
  arm.move(POS_ARM_CHECK_NINJA, 30);
  arm.move(POS_ARM_NEAR_SWITCH + 7, 30);  
}

void openDoorNija() {
  door.move(POS_DOOR_CHECK_NINJA, 30);
}

void openDoorMedium(int msec = 0) {
  door.move(POS_DOOR_MEDIUM_OPEN, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void openDoorFull(int msec = 0) {
  door.move(POS_DOOR_FULL_OPEN, msec);
  delay(MECANIC_DELAY_SWITCH);
}

// Ok, back home now
void backHome() { 
  arm.move(POS_HOME_ARM, 0); 
}

// Open the lid to see out
void goCheck(int msec = 10) {  
  arm.move(POS_ARM_PEEK, msec); 
}

void closeDoor(int msec = 0) {
  door.move(POS_HOME_DOOR, msec);
  delay(MECANIC_DELAY_SWITCH);
}
void raiseflag(int msec = 0) {
  flag.move(POS_flag_RAISED, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void tiltflag(int msec = 0) {
  flag.move(POS_flag_TILTED, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void hideflag(int msec = 0) {

  flag.move(POS_flag_HIDDEN, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void waveflag(int times)
{
  byte i;
  raiseflag();  
  softDelay(100);
  for (i = 0; i < times; i++) {    
    tiltflag();
    softDelay(7);
    raiseflag();
  }
  softDelay(100);
  hideflag();
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
  if (!interrupted) motor.forward(100);
  if (!interrupted) softDelay(500);
  if(!interrupted && detect()) {
    Serial.println("Detected movement!");
    if (!interrupted) motor.forward(100);
    if (!interrupted) softDelay(500);
    if (!interrupted) motor.halt();
  }
   softDelay(100);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();  
}


void whiteflag() 
{
  //is_flag_home = false;
  flag.isHome(false);
  
  if (!interrupted) openDoorFull();
  if (!interrupted) waveflag(5);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) backHome();
  if (!interrupted) softDelay(2000);
  if(!interrupted && detect()) {
    if (!interrupted) waveflag(7);
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
void checkCheckReturn() { //not ok
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
  if (!interrupted) backHome();
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
  if (!interrupted) softDelay(300);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();  
}

void crazyDoor() {
  if (!interrupted) openDoorMedium(7);
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(700);
  if (!interrupted) openDoorFull();    
  if (!interrupted) softDelay(700);  
  if (!interrupted) closeDoor();  
  if (!interrupted) openDoorFull();      
  if (!interrupted) goFlipThatSwitch(15);
  if (!interrupted) backHome(); 
  if (!interrupted) closeDoor();
}

void crazySlow() { //ok
  if (!interrupted) openDoorFull(30);
  if (!interrupted) goFlipThatSwitch(30);  
  if (!interrupted) closeDoor(30);
}

void adjustDistance() {
if(detect()) {
    waveflag(7);
  }    
}


void loop() {
  // Update the switch position
  bouncer.update();
  activated = bouncer.read();
  
  if (arm.isHome() == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Going back home");
    backHome();
        
  } else if (door.isHome() == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Closing door");
    closeDoor();
        
  } else if (flag.isHome() == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Hiding flag");
    hideflag();
        
  } else if ( randCheck < 5 && hasAlreadyChecked == false) {      
      // We only check once after an activation
      hasAlreadyChecked = true;

      arm.isHome(false);
      //is_arm_home = false;      
      
      Serial.println("Random check, baby. ");      
      
      arm.reattach();
      door.reattach();      
      
      if (!interrupted) goCheck(5);
      if (!interrupted) softDelay(1000);      
  } else if (activated == HIGH) {

    Serial.println("Who turned me on?");

    arm.isHome(false);
    door.isHome(false);
    
    arm.reattach();
    door.reattach();
    flag.reattach();
       
    // Find a new behaviour for next time
    randBehaviour = random(1, NUM_BEHAVIOURS);
    Serial.print("Starting behaviour: ");
    Serial.println(randBehaviour);
    switch(randBehaviour) { 
      case 1: 
        Serial.println("1: Normal flip");
        goFlipThatSwitch(); break;
      case 2: 
        Serial.println("2: Check");  
        check(); break;
      case 3: 
        Serial.println("3: Multi try");
        multiTry(); break;
      case 4: 
        Serial.println("4: Check and return");
        checkReturn(); break;
      case 5: 
        Serial.println("5: Check, check and return");
        checkCheckReturn(); break;
      case 6: 
        Serial.println("6: Afraid");
        afraid(); break;
      case 7: 
        Serial.println("7: Oh, wait");
        ohWait(); break;
      case 8: 
        Serial.println("8: Drive away");
        driveAway();break;
      case 9:
        Serial.println("9: White flag");
        whiteflag(); break;
      case 10: 
        Serial.println("10: Matrix");
        matrix(); break;
      case 11: 
        Serial.println("11: Crazy door");
        crazyDoor(); break;
      case 12: 
        Serial.println("12: Crazy slow");
        crazySlow(); break;        
      default: 
        Serial.println("Default");
        goFlipThatSwitch(); break;
    }

    interrupted = false; // Now that I have finished, I can rest.
    hasAlreadyChecked = false; // We can check once after that

    } 

  arm.waitAndDetatch();
  door.waitAndDetatch();
  flag.waitAndDetatch();

  // Random number to check sometimes
  randCheck = random(1, 1000000);

}

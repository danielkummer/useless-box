#include <Servo.h>
#include <Bounce2.h>

#include "MotorControl.h"
#include "Distance.h"
#include "ServoControl.h"

/*
 * Servo positions
 */ 
#define MIN_ARM_DEG 35
#define MAX_ARM_DEG 165
#define POS_ARM_HOME MIN_ARM_DEG
#define POS_SWITCH_ARM MAX_ARM_DEG
#define POS_ARM_PEEK (MIN_ARM_DEG + 70)
#define POS_ARM_CHECK_NINJA (MIN_ARM_DEG + 20)
#define POS_ARM_NEAR_SWITCH (MAX_ARM_DEG - 50)

#define MIN_DOOR_DEG 100
#define MAX_DOOR_DEG 40
#define POS_DOOR_HOME MIN_DOOR_DEG
#define POS_DOOR_MEDIUM_OPEN (MIN_DOOR_DEG - 97)
#define POS_DOOR_FULL_OPEN MAX_DOOR_DEG
#define POS_DOOR_CHECK_NINJA (MIN_DOOR_DEG - 80)

#define MIN_flag_DEG 60
#define MAX_flag_DEG 160
#define POS_flag_HIDDEN MIN_flag_DEG
#define POS_flag_RAISED MAX_flag_DEG
#define POS_flag_TILTED (MAX_flag_DEG - 40)

// the number of msec (approx.) for the servo to go one degree further
#define MECANIC_SPEED_PER_DEGREE 2  // 2 msec to move 1 degree
// the delay between the arm hitting the switcha and the switch reporting a change in value 
#define MECANIC_DELAY_SWITCH 50     // 50 msec to acknowledge a hit

#define IMPATIENT_INTERVAL_THRESHOLD 5  //times the interval must be triggered before impatient mode is activated
#define IMPATIENT_INTERVAL_TIME 15000
#define NUM_MAX_IMPATIENT_ACTION 5      //number of impatient actions - while the box stays in impatient mode

#define NUM_BEHAVIOURS 14

// Pins
const int armServoPin = 9;
const int doorServoPin = 6;
const int flagServoPin = 5;
const int directionPin = 13;
const int throttlePin = 11;
const int distancePin = A1;
const int boxSwitchPin  = 2;

// Operation variables
Bounce bouncer = Bounce();
int activated = LOW;

int randBehaviour = 1;            // This is used to choose the next behaviour 
long randCheck = 1;               // Random check when not activated
boolean hasAlreadyChecked = true; // Set when box has checked the switch (peeked at it)
boolean interrupted = false;      // Is set to true when the switch has been changed while doing something. Allows for a quick check in the loop() for what we should do next

int lastDistance = 0;             // Last measured distance
int threshold = 200;              // Distance threshold
unsigned long timer;              // Distance timer

// Impatient mode variables
int impatientCount = 0;           // How many impatient actions were done
int impatientThresholdCount = 0;  // How may times was the switch operated in short intervals
boolean impatient = false;        // is the box impatient
unsigned long activatedTimestamp; // When was the box last activated

MotorControl motor = MotorControl();
Distance distance = Distance();
ServoControl arm = ServoControl("arm");
ServoControl door = ServoControl("door");
ServoControl flag = ServoControl("flag");


void setup() {  
  pinMode(boxSwitchPin, INPUT);
  pinMode(directionPin, OUTPUT);  
  pinMode(distancePin,INPUT);
  
  Serial.begin(9600);
  Serial.println("Started."); 
  Serial.println("Initializing positions.");

  bouncer.attach(boxSwitchPin);
  motor.attach(directionPin, throttlePin);
  distance.attach(2);
  arm.attach(&bouncer, armServoPin, POS_ARM_HOME);
  door.attach(&bouncer, doorServoPin, POS_DOOR_HOME);
  flag.attach(&bouncer, flagServoPin, POS_flag_HIDDEN);
 
  Serial.println("Initializing random seed."); 
  randomSeed(analogRead(0));      
}
// A "soft" delay function - This function can be interrupted by manual switch change
void softDelay(int msec) {
  Serial.print("Delaying for ");
  Serial.print(msec);
  Serial.println(" msec..."); 
  bouncer.update();
  int val = bouncer.read();
  long time_counter = 0;  
  do {
    delay(1);
    time_counter++;
    bouncer.update();
    int current_value = bouncer.read();
    if (current_value != val) {
      Serial.println("/!\\[Soft Delay] Interrupted - The switch was operated while I was waiting on purpose !");
      interrupted = true;
      break;
    }
  } while(time_counter <= msec);
}

boolean detect() {
  int currentDistance;
  delay(1200);    //wait to stabilize sensor readings after opening the door 
  lastDistance = analogRead(distancePin);
  timer = millis();
  // wait for movement that exceeds threshold or if timer expires (5 sec),
  while(millis() - timer <= 5000) {
    currentDistance = analogRead(distancePin);
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
  interrupted = arm.move(POS_SWITCH_ARM, msec); 
  delay(MECANIC_DELAY_SWITCH); // Wait for the switch to acknowledge before returning in the loop
}

// Go near the arm, but not as close as to push the switch, and then retracts a bit
void tryFail(int msec = 5) {  
  interrupted = arm.move(POS_ARM_NEAR_SWITCH, msec);
  interrupted = arm.move(POS_ARM_NEAR_SWITCH + 7, msec); 
}

// Open the lid stealthly. User does not see me. I'm a NINJA !
void goStealthCheck() {    
  interrupted = arm.move(POS_ARM_CHECK_NINJA, 30);
  interrupted = arm.move(POS_ARM_NEAR_SWITCH + 7, 30);  
}

void openDoorNija() {
  interrupted = door.move(POS_DOOR_CHECK_NINJA, 30);
}

void openDoorMedium(int msec = 0) {
  interrupted = door.move(POS_DOOR_MEDIUM_OPEN, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void openDoorFull(int msec = 0) {
  interrupted = door.move(POS_DOOR_FULL_OPEN, msec);
  delay(MECANIC_DELAY_SWITCH);
}

// Ok, back home now
void backHome() { 
  int homePosArm = impatient ? POS_ARM_PEEK : POS_ARM_HOME;
  arm.move(homePosArm, 0); 
}

// Open the lid to see out
void goCheck(int msec = 10) {  
  interrupted = arm.move(POS_ARM_PEEK, msec); 
}

void closeDoor(int msec = 0) {
  int homePos = impatient ? POS_DOOR_MEDIUM_OPEN : POS_DOOR_HOME;  
  interrupted = door.move(homePos, msec);
  delay(MECANIC_DELAY_SWITCH);
}
void raiseflag(int msec = 0) {
  interrupted = flag.move(POS_flag_RAISED, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void tiltflag(int msec = 0) {
  interrupted = flag.move(POS_flag_TILTED, msec);
  delay(MECANIC_DELAY_SWITCH);
}

void hideflag(int msec = 0) {
  interrupted = flag.move(POS_flag_HIDDEN, msec);  
  delay(MECANIC_DELAY_SWITCH);
}

void waveflag(int times)
{
  byte i;
  if (!interrupted) raiseflag();  
  if (!interrupted) softDelay(100);
  for (i = 0; i < times; i++) {    
    if (!interrupted) tiltflag();
    if (!interrupted) softDelay(50);
    if (!interrupted) raiseflag();
  }
  if (!interrupted) softDelay(100);
  if (!interrupted) hideflag();
}

/**
 * interruptable methods - use these in switch statement
 */

// Go out and flip that switch the user just
void goFlipThatSwitch(int msec = 0) { 
  if (!interrupted) openDoorMedium(msec);
  if (!interrupted) flipSwitch(msec);
}

void driveAway()
{
  if (!interrupted) openDoorMedium();
  if (!interrupted) motor.forward(100);
  delay(2000);  
  /*if(!interrupted && detect()) {
    Serial.println("Detected movement!");
    if (!interrupted) motor.forward(100);
    if (!interrupted) softDelay(300);
    if (!interrupted) motor.halt();
  }*/  
  if (!interrupted) motor.halt();
  if (!interrupted) goFlipThatSwitch();  
}

void whiteflag() 
{
  //is_flag_home = false;
  flag.isHome(false);  
  if (!interrupted) openDoorFull();
  if (!interrupted) waveflag(7);
  if (!interrupted) softDelay(500);
  if (!interrupted) goFlipThatSwitch();   
}

// Check once, wait, then flip the switch
void check() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck();
  if (!interrupted) softDelay(1000);
  if (!interrupted) goFlipThatSwitch();
}

// Check, return back home, then flip
void checkReturn() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck();
  if (!interrupted) softDelay(1500);
  if (!interrupted) backHome();
  if (!interrupted) softDelay(800);
  if (!interrupted) goFlipThatSwitch();
}

// Multi tries. At the end, succeeds...
void multiTry() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) backHome();
  if (!interrupted) softDelay(500);
  if (!interrupted) goFlipThatSwitch();
}

// Check once slowly, return, check again, return to stealth position, then flip
void checkCheckReturn() { //not ok
  if (!interrupted) openDoorNija();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(1500);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck();
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(1000);
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(800);
  if (!interrupted) goFlipThatSwitch();
}

void afraid() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail(0);  
  if (!interrupted) goCheck(0);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(1500);
  if (!interrupted) openDoorMedium();
  if (!interrupted) goFlipThatSwitch(5);
}

// #OhWait 
void ohWait() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail(0);
  if (!interrupted) backHome();
  if (!interrupted) openDoorMedium();
  if (!interrupted) softDelay(700);
  if (!interrupted) goCheck(2); // Woops. Forgot something ?
  if (!interrupted) softDelay(1000);
  if (!interrupted) goFlipThatSwitch(15);
}

void matrix() {
  if (!interrupted) openDoorMedium();
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
  if (!interrupted) goFlipThatSwitch(30);  
}

void flappingAround() {
  if (!interrupted) openDoorMedium(7);
  if (!interrupted) closeDoor();  
  if (!interrupted) openDoorMedium(7);      
  if (!interrupted) closeDoor();  
  if (!interrupted) openDoorMedium(7);      
  if (!interrupted) goFlipThatSwitch();  
}

void vibrating() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail(0);  
  if (!interrupted) tryFail(0);
  if (!interrupted) tryFail(0);
  if (!interrupted) tryFail(0);
  if (!interrupted) tryFail(0);
  if (!interrupted) tryFail(0);
  if (!interrupted) tryFail(0);
  if (!interrupted) backHome();
  if (!interrupted) goFlipThatSwitch();  
}

void moveBackAndForth() {
  
  if (!interrupted) motor.forward(100);
  if (!interrupted) softDelay(500); 
  if (!interrupted) motor.backward(100);
  if (!interrupted) softDelay(500); 
  if (!interrupted) motor.forward(100);  
  if (!interrupted) softDelay(200); 
  if (!interrupted) motor.backward(100);
  if (!interrupted) softDelay(200); 
  /*if(!interrupted && detect()) {
    Serial.println("Detected movement!");
    if (!interrupted) motor.forward(100);
    if (!interrupted) softDelay(300);
    if (!interrupted) motor.halt();
  }*/  
  if (!interrupted) motor.halt();
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) goFlipThatSwitch();  
}

void loop() {
  // Update the switch position
  bouncer.update();
  activated = bouncer.read();

  //reset impatient mode after NUM_MAX_IMPATIENT_ACTION times;
  if(impatient && impatientCount >= NUM_MAX_IMPATIENT_ACTION) { 
    Serial.println("Getting tired, leaving impatient mode...");
    impatient = false;
    impatientCount = 0;
    arm.reattach();
    door.reattach();
    impatientThresholdCount = 0;    
    door.setHome(POS_DOOR_HOME);
    door.isHome(false);
    arm.setHome(POS_ARM_HOME);    
    arm.isHome(false);
  }  
  
  if (flag.isHome() == false) {    
    Serial.println("Hiding flag");
    hideflag();        
  } else if (arm.isHome() == false) {    
    Serial.println("Going back home");
    backHome();        
  } else if (door.isHome() == false) {
    Serial.println("Closing door");
    closeDoor();     
  } else if (randCheck < 5 && hasAlreadyChecked == false) {            
    hasAlreadyChecked = true;
    arm.isHome(false);         
    Serial.println("Random check, baby. ");            
    arm.reattach();
    door.reattach();            
    if (!interrupted) goCheck(5);
    if (!interrupted) softDelay(1000);      
  }  else if (activated == HIGH) {
    Serial.println("Who turned me on?");
    arm.isHome(false);
    door.isHome(false);
    
    arm.reattach();
    door.reattach();
    flag.reattach();
       
    // Find a new behaviour for next time
    if(!impatient) {         
      int newBehaviour;
      do {
        newBehaviour = random(1, NUM_BEHAVIOURS);      
      } while (!newBehaviour == randBehaviour );
      randBehaviour = newBehaviour;    
    }

    if(impatient) {
      Serial.print("I'm impatient right now - for the "); 
      randBehaviour = 1; //trigger default behaviour
      impatientCount++;
      Serial.print(impatientCount);
      Serial.println(" time!");
    }
    //increase impatient trigger count if box is operated repeatedly in interval
    if (!impatient && (millis() - activatedTimestamp <= IMPATIENT_INTERVAL_TIME)) {      
      impatientThresholdCount++;
      Serial.print("Getting impatient... ");
      Serial.print(impatientThresholdCount);
      Serial.print(" of ");
      Serial.println(IMPATIENT_INTERVAL_THRESHOLD);
      Serial.print("Time difference is: ");
      Serial.println(millis() - activatedTimestamp);
    }
    if (!impatient && impatientThresholdCount >= IMPATIENT_INTERVAL_THRESHOLD ) {
      Serial.println("Alright, thats it! I'm getting impatient!");
      impatient = true;     
      arm.setHome(POS_ARM_PEEK);      
      door.setHome(POS_DOOR_MEDIUM_OPEN);            
      delay(100);
    }

    //use specific moves in impatient mode 
    if (impatient) {
      //define "special" moves
      switch(impatientCount) {
        case NUM_MAX_IMPATIENT_ACTION - 2:
          randBehaviour = 9; break;
          break;
        case NUM_MAX_IMPATIENT_ACTION - 1: 
          randBehaviour = 101; break;
        case NUM_MAX_IMPATIENT_ACTION:
          randBehaviour = 100; break;
          break;
        default:
          randBehaviour = 1;  
      }      
    }
    
    Serial.print("Starting behaviour: [");
    Serial.print(randBehaviour);
    Serial.println("]");
    switch(randBehaviour) { 
      case 1: 
        Serial.println("Normal flip");
        goFlipThatSwitch(); break;
      case 2: 
        Serial.println("Check");  
        check(); break;
      case 3: 
        Serial.println("Multi try");
        multiTry(); break;
      case 4: 
        Serial.println("Check and return");
        checkReturn(); break;
      case 5: 
        Serial.println("Check, check and return");
        checkCheckReturn(); break;
      case 6: 
        Serial.println("Afraid");
        afraid(); break;
      case 7: 
        Serial.println("Oh, wait");
        ohWait(); break;
      case 8: 
        Serial.println("Crazy slow");
        crazySlow(); break;        
      case 9: 
        Serial.println("Matrix");
        matrix(); break;
      case 10: 
        Serial.println("Crazy door");
        crazyDoor(); break;
      case 11: 
        Serial.println("Drive away");
        driveAway(); break;
      case 12:
        Serial.println("White flag");
        whiteflag(); break;   
      case 13:
        Serial.println("Back and Forth");
        moveBackAndForth(); break;
      case 100:
        Serial.println("Vibrating");      
        vibrating(); break;  
      case 101:
        Serial.println("Flapping door around");
        flappingAround(); break;      
      default: 
        Serial.println("Default");
        goFlipThatSwitch(); break;   
      } 
      interrupted = false; 
      hasAlreadyChecked = false;      
      activatedTimestamp = millis();
  } 
  arm.waitAndDetatch();
  door.waitAndDetatch();
  flag.waitAndDetatch();  
  randCheck = random(1, 100000);  
}

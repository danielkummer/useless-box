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

#define MIN_DOOR_DEG 90
#define MAX_DOOR_DEG 38
#define POS_DOOR_HOME MIN_DOOR_DEG
#define POS_DOOR_MEDIUM_OPEN (POS_DOOR_HOME - 30)
#define POS_DOOR_FULL_OPEN MAX_DOOR_DEG
#define POS_DOOR_CHECK_NINJA (POS_DOOR_HOME - 70)
#define POS_DOOR_FLAP POS_DOOR_HOME - 20

#define MIN_FLAG_DEG 60
#define MAX_FLAG_DEG 160
#define POS_flag_HIDDEN MIN_FLAG_DEG
#define POS_flag_RAISED MAX_FLAG_DEG
#define POS_flag_TILTED (MAX_FLAG_DEG - 40)

// the number of msec (approx.) for the servo to go one degree further
#define MECANIC_SPEED_PER_DEGREE 2  // 2 msec to move 1 degree
// the delay between the arm hitting the switcha and the switch reporting a change in value 
#define MECANIC_DELAY_SWITCH 70     // 70 msec to acknowledge a hit

#define IMPATIENT_INTERVAL_THRESHOLD 5  //times the interval must be triggered before impatient mode is activated
const long IMPATIENT_INTERVAL_TIME = 20000;
#define NUM_MAX_IMPATIENT_ACTION 7      //number of impatient actions - while the box stays in impatient mode

#define NUM_BEHAVIOURS 13

// Pins
const int armServoPin = 9;
const int doorServoPin = 6;
const int flagServoPin = 5;
const int directionPin = 13;
const int throttlePin = 11;
const int distancePin = A1;
const int boxSwitchPin  = 2;
const int debugPin = 4;

// Operation variables
Bounce bouncer = Bounce();
int activated = LOW;

int selectedBehaviour = 1;            // This is used to choose the next behaviour 
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


Distance distance = Distance();
MotorControl motor = MotorControl();
ServoControl arm = ServoControl("arm");
ServoControl door = ServoControl("door");
ServoControl flag = ServoControl("flag");


/**
 * Simple blink method for debug purposes
 */
void debugBlink(int times = 3, int wait = 200) {
  int i;
  for (i = 0; i < times; i++) {
    digitalWrite(debugPin, HIGH);
    delay(wait);
    digitalWrite(debugPin, LOW);  
    delay(wait);
  }  
}


void setup() {  
  pinMode(boxSwitchPin, INPUT);
  pinMode(directionPin, OUTPUT);  
  pinMode(distancePin,INPUT);

  pinMode(debugPin, OUTPUT);  
  
  Serial.begin(9600);
  Serial.println("Started."); 
  Serial.println("Initializing positions.");

  bouncer.attach(boxSwitchPin);
  motor.attach(directionPin, throttlePin);
  distance.attach(2);
  arm.attach(&bouncer, armServoPin, POS_ARM_HOME);
  door.attach(&bouncer, doorServoPin, POS_DOOR_HOME);
  flag.attach(&bouncer, flagServoPin, POS_flag_HIDDEN);
  activatedTimestamp = IMPATIENT_INTERVAL_TIME + 1;
 
  Serial.println("Initializing random seed.");

  
  //debug - I'm able to detect arduino resets like this...
  debugBlink(10, 100); 
  randomSeed(analogRead(0));      
}

/**
 * A "soft" delay function - This function can be interrupted by manual switch change.
 * If interrupted the method returns early and sets the interruped state variable to true.
 */
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

/**
 * Detect movement inside a given threshold
 */
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

/**
 * Reset operation to normal mode, reposition arm and door
 */
void resetToNormalMode() {
  Serial.println("Getting tired, leaving impatient mode...");      
  impatientCount = 0;
  impatient = false;
  arm.reattach();
  door.reattach();    
  door.setHome(POS_DOOR_HOME);
  door.isHome(false);
  arm.setHome(POS_ARM_HOME);    
  arm.isHome(false);
}

/**
 * Set operation mode to impation mode, reposition arm and door
 */
void setImpatientMode() {
  Serial.println("Alright, thats it! I'm getting impatient!");
  impatient = true;    
  impatientThresholdCount = 0;     
  arm.isHome(false);
  arm.setHome(POS_ARM_PEEK);      
  door.isHome(false);
  door.setHome(POS_DOOR_MEDIUM_OPEN);            
  delay(100);
}

/**
 * Get the next normal behaviour
 */
int determineNormalBehaviour() {            
  int newBehaviour;
  do {
    newBehaviour = random(1, NUM_BEHAVIOURS);      
  } while (newBehaviour == selectedBehaviour );
  selectedBehaviour = newBehaviour;    
  return newBehaviour;  
}

/**
 * Get the next behaviour for impatient mode
 */
int determineImpatientBehaviour() {     
  //define "special" moves
  switch(impatientCount) {
    case NUM_MAX_IMPATIENT_ACTION - 3:
      return 9;
    case NUM_MAX_IMPATIENT_ACTION - 2:
      return 101;
    case NUM_MAX_IMPATIENT_ACTION - 1: 
      return 100;
    case NUM_MAX_IMPATIENT_ACTION:
      return 102;
    default:
      return 1;  
  }  
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

void openDoorNija(int msec = 30) {
  interrupted = door.move(POS_DOOR_CHECK_NINJA, msec);
}

void openDoorFlap(int msec = 0) {
  interrupted = door.move(POS_DOOR_FLAP, msec);  
}

void openDoorMedium(int msec = 0) {
  interrupted = door.move(POS_DOOR_MEDIUM_OPEN, msec);
  //delay(MECANIC_DELAY_SWITCH);
}

void openDoorFull(int msec = 0) {
  interrupted = door.move(POS_DOOR_FULL_OPEN, msec);
  //delay(MECANIC_DELAY_SWITCH);
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
  //delay(MECANIC_DELAY_SWITCH);
}


void raiseflag(int msec = 0) {
  interrupted = flag.move(POS_flag_RAISED, msec);
  //delay(MECANIC_DELAY_SWITCH);
}

void tiltflag(int msec = 0) {
  interrupted = flag.move(POS_flag_TILTED, msec);
  //delay(MECANIC_DELAY_SWITCH);
}

void hideflag(int msec = 0) {
  interrupted = flag.move(POS_flag_HIDDEN, msec);  
  //delay(MECANIC_DELAY_SWITCH);
}

void waveflag(int times)
{
  byte i;
  if (!interrupted) raiseflag();  
  if (!interrupted) softDelay(100);
  for (i = 0; i < times; i++) {    
    if (!interrupted) tiltflag();
    if (!interrupted) softDelay(15);
    if (!interrupted) raiseflag();
    if(interrupted) {
      break;
    }
  }
  if (!interrupted) softDelay(70);
  hideflag();
}

/* ------------------------------------ */
/* ---- INTERRUPTABLE BEHAVIOURS ------ */
/* use these for the concrete movements */
/* ------------------------------------ */

// Go out and flip that switch the user just
void goFlipThatSwitch(int msec = 0) { 
  if (!interrupted) openDoorMedium(msec);
  if (!interrupted) flipSwitch(msec);
}

void driveAway(){
  if (!interrupted) openDoorMedium();
  if (!interrupted) motor.forward(100);
  if (!interrupted) softDelay(2000);  
  /*if(!interrupted && detect()) {
    Serial.println("Detected movement!");
    if (!interrupted) motor.forward(100);
    if (!interrupted) softDelay(300);
    if (!interrupted) motor.halt();
  }*/  
  motor.halt();
  if (!interrupted) goFlipThatSwitch();  
}

void whiteflag() 
{  
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
  if (!interrupted) softDelay(600);
  if (!interrupted) goFlipThatSwitch();
}

// Multi tries. At the end, succeeds...
void multiTry() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);
  if (!interrupted) tryFail();
  if (!interrupted) softDelay(500);  
  if (!interrupted) backHome();
  if (!interrupted) softDelay(200);
  if (!interrupted) goFlipThatSwitch();
}

// Check once slowly, return, check again, return to stealth position, then flip
void checkCheckReturn() { //not ok
  if (!interrupted) openDoorNija();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(1000);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck();
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(700);
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(500);
  if (!interrupted) goFlipThatSwitch();
}

void afraid() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail(0);  
  if (!interrupted) goCheck(0);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(1000);
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
}

void crazyDoor() {
  if (!interrupted) openDoorMedium(7);
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(700);
  if (!interrupted) openDoorFull();    
  if (!interrupted) softDelay(500);  
  if (!interrupted) closeDoor();  
  if (!interrupted) openDoorFull();      
  if (!interrupted) goFlipThatSwitch(15);  
}

void crazySlow() { //ok  
  if (!interrupted) goFlipThatSwitch(30);  
}

void flappingAround() {
  int i;
  for(i = 0; i < 5; i++) {
    if (!interrupted) openDoorFull(10);
    if (!interrupted) closeDoor();    
    if (!interrupted) softDelay(200);         
  }
  if (!interrupted) goFlipThatSwitch();   
}

void lowFlappingAround() {
  int i;
  for(i = 0; i < 5; i++) {
    if (!interrupted) openDoorFlap(0);
    if (!interrupted) closeDoor();        
  }  
  if (!interrupted) softDelay(300);         
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
  if (!interrupted) softDelay(600); 
  if (!interrupted) motor.backward(100);
  if (!interrupted) softDelay(600); 
  if (!interrupted) motor.forward(100);  
  if (!interrupted) softDelay(600); 
  if (!interrupted) motor.backward(100);
  if (!interrupted) softDelay(600); 
  /*if(!interrupted && detect()) {
    Serial.println("Detected movement!");
    if (!interrupted) motor.forward(100);
    if (!interrupted) softDelay(300);
    if (!interrupted) motor.halt();
  }*/  
  motor.halt();
  if (!interrupted) openDoorNija();  
  if (!interrupted) goFlipThatSwitch();  
}

void turnOffThenWait() {  
  arm.interruptable(false);
  if (!interrupted) goFlipThatSwitch(0);
  if (!interrupted) goCheck(0);
  arm.interruptable(true);
  if (!interrupted) softDelay(2000);  
}

void turnOffwaitAndFlipDoor() {
  arm.interruptable(false);
  if (!interrupted) goFlipThatSwitch(0);  
  int i;
  for(i = 0; i < 3; i++) {
    if (!interrupted) openDoorFull(0);    
    if (!interrupted) openDoorMedium(7);      
  }  
  arm.interruptable(true);  
  if (!interrupted) softDelay(500);    
}

void angryTurnOff() {
  arm.interruptable(false);
  goFlipThatSwitch();  
  backHome();
  goFlipThatSwitch();  
  backHome();
  goFlipThatSwitch();
  arm.move(POS_ARM_HOME, 0);
  arm.interruptable(true);
  if (!interrupted) softDelay(300);
  flag.isHome(false);  
  if (!interrupted) openDoorFull();
  if (!interrupted) waveflag(7);  
}

int randomCheck() {
  hasAlreadyChecked = true;
  arm.isHome(false);         
  if (!interrupted) arm.reattach();
  if (!interrupted) door.reattach();            
  if (!interrupted) goCheck(5);
  if (!interrupted) softDelay(1000);      
}

void loop() {
  // Update the switch position
  bouncer.update();
  activated = bouncer.read();  
     
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
    Serial.println("Random check, baby. ");            
    randomCheck();
  }  else if (activated == HIGH) {    
    Serial.println("Who turned me on?");
    
    arm.isHome(false);
    door.isHome(false);
    
    arm.reattach();
    door.reattach();
    flag.reattach();
       
    if(impatient) {
      Serial.print("I'm impatient right now - for the ");       
      impatientCount++;
      Serial.print(impatientCount);
      Serial.println(" time!");
    } else {
      //increase impatient trigger count if box is operated repeatedly in interval
      //if (!impatient && (millis() - activatedTimestamp <= IMPATIENT_INTERVAL_TIME)) {      
      //TODO activate threshold as soon as fixed standalone problem    
      impatientThresholdCount++;
      Serial.print("Getting impatient... ");
      Serial.print(impatientThresholdCount);
      Serial.print(" of ");
      Serial.println(IMPATIENT_INTERVAL_THRESHOLD);
      Serial.print("Time difference is: ");
      Serial.println(millis() - activatedTimestamp);
      debugBlink(impatientThresholdCount);      
    }
    
    //update activated after impatient check
    activatedTimestamp = millis();

    // set to impatient mode if conditions are met
    if (!impatient && impatientThresholdCount >= IMPATIENT_INTERVAL_THRESHOLD ) {
      setImpatientMode();
      //TODO: Debug led, remove
      debugBlink(5, 70);  
      digitalWrite(debugPin, HIGH);
    }

    //reset to normal mode after NUM_MAX_IMPATIENT_ACTION actions have been performed;
    if(impatient && impatientCount >= NUM_MAX_IMPATIENT_ACTION) { 
      //TODO: Debug led, remove
      debugBlink(5, 70);  
      digitalWrite(debugPin, LOW);
      resetToNormalMode(); 
    }  

    //use specific moves in impatient mode 
    if (impatient) {
      selectedBehaviour = determineImpatientBehaviour();    
    } else {      
      selectedBehaviour  = determineNormalBehaviour();
    }    
    
    Serial.print("-------- Starting behaviour: [");
    Serial.print(selectedBehaviour);
    Serial.println("]");
    Serial.print("-------- ");
    switch(selectedBehaviour) { 
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
      //should i really include this in the normal operaion mode? it's a special gimick
      //case 12:
      //  Serial.println("White flag");
      //  whiteflag(); break;         
      case 12:
        Serial.println("Back and Forth");
        moveBackAndForth(); break;
      case 14:
        Serial.println("low flapping around");
        lowFlappingAround(); break;  
      case 15:
        Serial.println("turn off, wait");
        turnOffThenWait(); break;  
      case 16:
        Serial.println("long wait after turnoff");
        turnOffwaitAndFlipDoor(); break; 
      case 100:
        Serial.println("Vibrating");      
        vibrating(); break;  
      case 101:
        Serial.println("Flapping door around");
        flappingAround(); break;      
      case 102:
        Serial.println("Angry turn-off");
        angryTurnOff(); break;
      default: 
        Serial.println("Default");
        goFlipThatSwitch(); break;   
      } 
      interrupted = false; 
  } 
  arm.waitAndDetatch();
  door.waitAndDetatch();
  flag.waitAndDetatch();  
  Serial.println("end of loop, all detatched...");
  randCheck = random(1, 500000);  
}

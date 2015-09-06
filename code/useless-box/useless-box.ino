//#define ENABLE_PRINT

#ifdef ENABLE_PRINT
  #define Sprintln(a) (Serial.println(a))
  #define Sprint(a) (Serial.print(a))
  #define Sbegin(a) (Serial.begin(a))
#else   
  #define Sprintln(a)
  #define Sprint(a)
  #define Sbegin(a)
#endif

#include <Servo.h>
#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>

#include "MotorControl.h"
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
//-------------------------------------------------
#define MIN_DOOR_DEG 95
#define MAX_DOOR_DEG 40
#define POS_DOOR_HOME MIN_DOOR_DEG
#define POS_DOOR_MEDIUM_OPEN (POS_DOOR_HOME - 32) 
#define POS_DOOR_FULL_OPEN MAX_DOOR_DEG
#define POS_DOOR_CHECK_NINJA (POS_DOOR_HOME - 20)
#define POS_DOOR_FLAP POS_DOOR_HOME - 20
//-------------------------------------------------
#define MIN_FLAG_DEG 55
#define MAX_FLAG_DEG 160
#define POS_FLAG_HIDDEN MIN_FLAG_DEG
#define POS_FLAG_RAISED MAX_FLAG_DEG
#define POS_FLAG_TILTED (MAX_FLAG_DEG - 40)
//-------------------------------------------------
// 
#define MECANIC_SPEED_PER_DEGREE 2  // the number of msec (approx.) for the servo to go one degree further
#define MECANIC_DELAY_SWITCH 70     // the delay between the arm hitting the switcha and the switch reporting a change in value 
//-------------------------------------------------
#define IMPATIENT_INTERVAL_THRESHOLD 7  //times the interval must be triggered before impatient mode is activated
#define IMPATIENT_INTERVAL_TIME 20000
#define IMPATIENT_MODE_TIMEOUT 30000
#define NUM_MAX_IMPATIENT_ACTION 7      //number of impatient actions - while the box stays in impatient mode


#define NUM_BEHAVIOURS 19

/*
 * I/O
 */ 
#define ARM_SERVO_PIN   9
#define DOOR_SERVO_PIN  6
#define FLAG_SERVO_PIN  5
#define DIRECTION_PIN   13
#define THROTTLE_PIN    11
#define DISTANCE_PIN    A4
#define BOX_SWITCH_PIN  2
#define PIXEL_PIN       10

/*
 * Operation
 */ 
Bounce bouncer = Bounce();
int activated = LOW;

int selectedBehaviour = 1;        // This is used to choose the next behaviour 
long randCheck = 1;               // Random check when not activated
boolean hasAlreadyChecked = true; // Set when box has checked the switch (peeked at it)
boolean interrupted = false;      // Is set to true when the switch has been changed while doing something. Allows for a quick check in the loop() for what we should do next

//Distance detection
int lastDistance = 0;             // Last measured distance
int threshold = 200;              // Distance threshold
unsigned long timer;              // Distance timer

// Impatient mode variables
int impatientCount = 0;           // How many impatient actions were done
int impatientThresholdCount = 0;  // How may times was the switch operated in short intervals
boolean impatient = false;        // is the box impatient
unsigned long activatedTimestamp; // When fwas the box last activated

MotorControl motor = MotorControl();
ServoControl arm = ServoControl("arm");
ServoControl door = ServoControl("door");
ServoControl flag = ServoControl("flag");
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

/*
 * Colors
 */ 
uint32_t black = pixels.Color(0, 0, 0);
uint32_t red =  pixels.Color(255, 0, 0);
uint32_t yellow = pixels.Color(255, 194, 0);
uint32_t orange = pixels.Color(255,146,0);
uint32_t orangeRed = pixels.Color(255,65,0);
uint32_t blue = pixels.Color(11,97,164);
uint32_t impatientColors[] = {
  pixels.Color(255, 194, 0), //yellow
  pixels.Color(255,146,0),   //orange
  pixels.Color(255,65,0),    //orangeRed
  pixels.Color(234,0,55),    //purplered
  pixels.Color(255, 0, 0),   //red
  pixels.Color(255, 0, 0),   //red
  pixels.Color(255, 0, 0),   //red
  pixels.Color(11,97,164)    //blue  
};

/*
 * Setup
 */ 
void setup() {  
  pinMode(BOX_SWITCH_PIN, INPUT);
  pinMode(DIRECTION_PIN, OUTPUT);  
  pinMode(DISTANCE_PIN,INPUT);
  
  Sbegin(9600);
  Sprintln(F("Started.")); 
  Sprintln(F("Initializing positions."));

  bouncer.attach(BOX_SWITCH_PIN);
  motor.attach(DIRECTION_PIN, THROTTLE_PIN);
  arm.attach(&bouncer, ARM_SERVO_PIN, POS_ARM_HOME, POS_SWITCH_ARM);
  door.attach(&bouncer, DOOR_SERVO_PIN, POS_DOOR_HOME, POS_DOOR_FULL_OPEN);
  flag.attach(&bouncer, FLAG_SERVO_PIN, POS_FLAG_HIDDEN, POS_FLAG_RAISED);
  
  activatedTimestamp = IMPATIENT_INTERVAL_TIME + 1;

  //initializeneopixels
  pixels.begin();
  pixels.show();
  pixels.setBrightness(255); 
  randomSeed(analogRead(0));      
}

/**
 * A "soft" delay function - This function can be interrupted by manual switch change.
 * If interrupted the method returns early and sets the interruped state variable to true.
 * The soft delay function returns the value of the switch
 */
int softDelay(int msec) {
  Sprint(F("Delaying for "));
  Sprint(msec);
  Sprintln(F(" msec...")); 
  bouncer.update();
  int val = bouncer.read();
  int current_value;
  long time_counter = 0;  
  do {
    delay(1);
    time_counter++;
    bouncer.update();
    current_value = bouncer.read();
    if (current_value != val) {
      Sprintln(F("/!\\[Soft Delay] Interrupted - The switch was operated while I was waiting on purpose !"));
      interrupted = true;
      break;
    }
  } while(time_counter <= msec);
  return current_value;
}

/**
 * Detect movement inside a given threshold
 */
boolean detect(int detectionTime) {
  int currentDistance;
  delay(1200);    //wait to stabilize sensor readings after opening the door   
  timer = millis();
  //interrupt if bouncer value changes
  // wait for movement that exceeds threshold or if timer expires (5 sec),
  bouncer.update();
  int oldButtonValue = bouncer.read();
  
  while(millis() - timer <= detectionTime - 1200) {
    currentDistance = analogRead(DISTANCE_PIN);        
    Sprint(F("Current Distance"));
    Sprintln(currentDistance);
    if(currentDistance >= 380) { // The users hand is near the switch!
      Sprintln(F("Detected!"));
      return true;
    }    
    delay(500);

    //exit loop if switch is on
    /*bouncer.update();
    int newButtonValue = bouncer.read();
    if(oldButtonValue != newButtonValue && newButtonValue == HIGH) {
      break;
    }*/    
  }
  return false;
}

/**
 * Reset operation to normal mode, reposition arm and door
 */
void resetToNormalMode() {
  Sprintln(F("Getting tired, leaving impatient mode..."));      
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
  Sprintln(F("Alright, thats it! I'm getting impatient!"));
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
  int result = 1;
  //define "special" moves
  switch(impatientCount) {
    case NUM_MAX_IMPATIENT_ACTION - 3:
      result = 9; break;
    case NUM_MAX_IMPATIENT_ACTION - 2:
      result = 101; break;
    case NUM_MAX_IMPATIENT_ACTION - 1: 
      result = 100; break;
    case NUM_MAX_IMPATIENT_ACTION:
      result = 102; break;
    default:
      break;
  }  
  return result;
}

void setRgbColor(byte red, byte green, byte blue) {
  setColor(pixels.Color(red, green, blue));
}

void setColor(uint32_t color) {
  pixels.setPixelColor(0, color); 
  pixels.setPixelColor(1, color); 
  pixels.show();
}

void pulseColor(uint32_t color, byte times) {
  for(byte i = 0; i < times; i++) {
    setColor(color);
    delay(100);
    setColor(black);
    delay(100);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(byte wheelpos) {
  wheelpos = 255 - wheelpos;
  if(wheelpos < 85) {
    return pixels.Color(255 - wheelpos * 3, 0, wheelpos * 3);
  }
  if(wheelpos < 170) {
    wheelpos -= 85;
    return pixels.Color(0, wheelpos * 3, 255 - wheelpos * 3);
  }
  wheelpos -= 170;
  return pixels.Color(wheelpos * 3, 255 - wheelpos * 3, 0);
}

/* ------------------------------- */
/* ---- STANDARD BEHAVIOURS ------ */
/* ------------------------------- */

void flipSwitch(int msec = 0) { 
  interrupted = arm.move(POS_SWITCH_ARM, msec); 
  delay(MECANIC_DELAY_SWITCH); // Wait for the switch to acknowledge before returning in the loop
}

// Go near the arm, but not as close as to push the switch, and then retracts a bit
void tryFail(int msec = 0) {  
  interrupted = arm.move(POS_ARM_NEAR_SWITCH, msec);
  interrupted = arm.move(POS_ARM_NEAR_SWITCH + 7, msec); 
}

// Open the lid stealthly. User does not see me. I'm a NINJA !
void goStealthCheck() {    
  interrupted = arm.move(POS_ARM_CHECK_NINJA, 30);
  interrupted = arm.move(POS_ARM_NEAR_SWITCH + 7, 30);  
}

void openDoorNinja(int msec = 0) {
  interrupted = door.move(POS_DOOR_CHECK_NINJA, msec);
}

void openDoorFlap(int msec = 0) {
  interrupted = door.move(POS_DOOR_FLAP, msec);  
}

void openDoorMedium(int msec = 0) {
  interrupted = door.move(POS_DOOR_MEDIUM_OPEN, msec);  
}

void openDoorFull(int msec = 0) {
  interrupted = door.move(POS_DOOR_FULL_OPEN, msec);
}

// Ok, back home now
void backHome() { 
  int homePosArm = impatient ? POS_ARM_PEEK : POS_ARM_HOME;
  arm.move(homePosArm, 0); 
}

// Open the lid to see out
void goCheck(int msec = 0) {  
  interrupted = arm.move(POS_ARM_PEEK, msec); 
}

void closeDoor(int msec = 0) {
  int homePos = impatient ? POS_DOOR_MEDIUM_OPEN : POS_DOOR_HOME;  
  interrupted = door.move(homePos, msec);
}


void raiseflag(int msec = 0) {
  interrupted = flag.move(POS_FLAG_RAISED, msec);
}

void tiltflag(int msec = 0) {
  interrupted = flag.move(POS_FLAG_TILTED, msec);
}

void hideflag(int msec = 0) {
  interrupted = flag.move(POS_FLAG_HIDDEN, msec);  
}

void waveflag(int times) {
  byte i;
  raiseflag();  
  delay(100);
  for (i = 0; i < times; i++) {    
    tiltflag();
    delay(15);    
    //door.move(POS_DOOR_FULL_OPEN, 0); //damn this hack
    raiseflag();    
  }  
  delay(70);
  hideflag();  
}

/* ------------------------------------ */
/* ---- INTERRUPTABLE BEHAVIOURS ------ */
/* use these for the concrete movements */
/* ------------------------------------ */

// Go out and flip that switch the user just
void goOpenDoorAndFlipThatSwitch(int msec = 0) { 
  if (!interrupted) openDoorMedium(msec);
  if (!interrupted) flipSwitch(msec);
}

void driveAway(){
  if (!interrupted) openDoorMedium();
  if (!interrupted) motor.forward(100);
  if (!interrupted) softDelay(2000);  
  if(!interrupted && detect(3000)) {
    Sprintln(F("Detected movement!"));
    if (!interrupted) motor.backward(100);
    if (!interrupted) softDelay(800);
    if (!interrupted) motor.halt();
  }
  motor.halt();
  if (!interrupted) goOpenDoorAndFlipThatSwitch();  
}

void whiteflag() 
{  
  flag.isHome(false);  
  openDoorFull();
  waveflag(7);
  flag.isHome(true);  
  softDelay(500);  
  if (!interrupted) flipSwitch();   
}

// Check once, wait, then flip the switch
void check() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck(random(0, 10));
  if (!interrupted) softDelay(1000);
  if (!interrupted) goOpenDoorAndFlipThatSwitch();
}

// Check, return back home, then flip
void checkReturn() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck(random(0,10));
  if (!interrupted) softDelay(1500);
  if (!interrupted) backHome();
  if (!interrupted) softDelay(600);
  if (!interrupted) goOpenDoorAndFlipThatSwitch();
}

// Multi tries. At the end, succeeds...
void multiTry() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail(5);
  if (!interrupted) softDelay(500);
  if (!interrupted) tryFail(5);
  if (!interrupted) softDelay(500);  
  if (!interrupted) backHome();
  if (!interrupted) softDelay(200);
  if (!interrupted) goOpenDoorAndFlipThatSwitch();
}

// Check once slowly, return, check again, return to stealth position, then flip
void checkCheckReturn() { //not ok
  if (!interrupted) openDoorNinja(30);
  if (!interrupted) openDoorMedium();
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(1000);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck(10);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(700);
  if (!interrupted) openDoorNinja(30);
  if (!interrupted) goStealthCheck();
  if (!interrupted) softDelay(500);
  if (!interrupted) goOpenDoorAndFlipThatSwitch();
}

void afraid() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail();  
  if (!interrupted) goCheck();
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(1000);
  if (!interrupted) openDoorMedium(20);
  if (!interrupted) flipSwitch(15);
}

// #OhWait 
void ohWait() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail();
  if (!interrupted) backHome();
  if (!interrupted) openDoorMedium();
  if (!interrupted) softDelay(700);
  if (!interrupted) goCheck(2); // Woops. Forgot something ?
  if (!interrupted) softDelay(1000);
  if (!interrupted) goOpenDoorAndFlipThatSwitch(random(0,15));
}

void matrix() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck(10);  
  if (!interrupted) flipSwitch(30);
  if (!interrupted) softDelay(300);  
}

void crazyDoor() {
  if (!interrupted) openDoorMedium(random(7,15));
  if (!interrupted) closeDoor();
  if (!interrupted) softDelay(700);
  if (!interrupted) openDoorFull(random(0,10));    
  if (!interrupted) softDelay(500);  
  if (!interrupted) closeDoor();  
  if (!interrupted) openDoorFull(random(0,20));      
  if (!interrupted) flipSwitch(random(5,20));  
}

void crazySlow() { //ok  
  if (!interrupted) goOpenDoorAndFlipThatSwitch(30);  
}

void flappingAround() {
  int i;
  for(i = 0; i < 5; i++) {
    if (!interrupted) openDoorFull(10);
    if (!interrupted) closeDoor();    
    if (!interrupted) softDelay(200);         
  }
  if (!interrupted) goOpenDoorAndFlipThatSwitch();   
}

void lowFlappingAround() {
  int i;
  for(i = 0; i < 5; i++) {
    if (!interrupted) openDoorFlap(0);
    if (!interrupted) closeDoor();        
  }  
  if (!interrupted) softDelay(300);         
  if (!interrupted) goOpenDoorAndFlipThatSwitch();   
}

void vibrating() {
  if (!interrupted) openDoorMedium();
  if (!interrupted) tryFail();  
  if (!interrupted) tryFail();
  if (!interrupted) tryFail();
  if (!interrupted) tryFail();
  if (!interrupted) tryFail();
  if (!interrupted) tryFail();
  if (!interrupted) tryFail();
  if (!interrupted) backHome();
  if (!interrupted) goOpenDoorAndFlipThatSwitch();  
}

void moveBackAndForth() {  
  if (!interrupted) motor.forward(100);
  if (!interrupted) softDelay(600); 
  if (!interrupted) motor.backward(100);
  if (!interrupted) softDelay(600); 
  if (!interrupted) motor.forward(100);  
  if (!interrupted) softDelay(600); 
  if (!interrupted) motor.backward(100);
  if (!interrupted) softDelay(700); 
  if(!interrupted && detect(3000)) {
    Sprintln(F("Detected movement!"));
    if (!interrupted) motor.forward(100);
    if (!interrupted) softDelay(300);
    if (!interrupted) motor.halt();
  }  
  motor.halt();
  if (!interrupted) openDoorNinja(30);  
  if (!interrupted) goOpenDoorAndFlipThatSwitch();  
}

void turnOffThenWait() {  
  arm.interruptable(false);
  if (!interrupted) goOpenDoorAndFlipThatSwitch(0);
  if (!interrupted) goCheck();
  arm.interruptable(true);
  if (!interrupted) softDelay(2000);  
}

void turnOffwaitAndFlipDoor() {
  arm.interruptable(false);
  if (!interrupted) goOpenDoorAndFlipThatSwitch(0);  
  int i;
  for(i = 0; i < 3; i++) {
    if (!interrupted) openDoorFull(0);    
    if (!interrupted) openDoorMedium(5);      
  }  
  arm.interruptable(true);  
  if (!interrupted) softDelay(500);    
}

void angryTurnOff() {
  arm.interruptable(false);
  goOpenDoorAndFlipThatSwitch();  
  backHome();
  goOpenDoorAndFlipThatSwitch();  
  backHome();
  goOpenDoorAndFlipThatSwitch();
  arm.move(POS_ARM_HOME, 0);
  arm.interruptable(true);
  if (!interrupted) softDelay(300);  
  door.move(POS_DOOR_FULL_OPEN, 0);  
  flag.isHome(false);  
  //openDoorFull();
  waveflag(7);  
  hideflag();  
  flag.isHome(true);  
  //arm.move(POS_ARM_HOME, 0);   
  //door.move(POS_DOOR_HOME, 0);
  pulseColor(red, 3);
}

/**
 * TODO when user is detected
 * 1. drive away
 * 2. flap door
 * 3. ??
 * 
 */
//working debug test
void turnOffIfUserIsDetected() {
  int i;
    for (i = 0; i < random(3, 5); i++) {
    openDoorMedium(15);
    flipSwitch();
    backHome();   
    //TODO should turn off immediatly if turned on during detection 
    if (detect(10000)) {        
      bouncer.update();
      if(bouncer.read() == HIGH) { 
        flipSwitch();
        backHome();   
      } else {
        goCheck(random(0, 15));                         
      }      
    }
    if(softDelay(5000) == LOW) {
        break;    
    }
  }  
}

void randomCheck() {
  hasAlreadyChecked = true;
  arm.isHome(false);         
  if (!interrupted) arm.reattach();
  if (!interrupted) door.reattach();            
  if (!interrupted) goCheck(5);
  if (!interrupted) softDelay(1000);      
}

/*
 * Loop
 */ 
void loop() {  
  bouncer.update();
  activated = bouncer.read();

  //reset to normal mode after NUM_MAX_IMPATIENT_ACTION actions have been performed;
  if(impatient && impatientCount >= NUM_MAX_IMPATIENT_ACTION) {       
    resetToNormalMode(); 
  }  

  // set to impatient mode if conditions are met
  if (!impatient && impatientThresholdCount >= IMPATIENT_INTERVAL_THRESHOLD ) {
    setImpatientMode();
    //TODO: Debug led, remove      
  }  
  
  /*if (flag.isHome() == false) {    
    Sprintln(F("Hiding flag"));
    hideflag();        
  } else*/
  if (arm.isHome() == false) {    
    Sprintln(F("Going back home"));
    backHome();        
  } else if (door.isHome() == false) {
    Sprintln(F("Closing door"));
    closeDoor();     
  } else if (randCheck < 5 && hasAlreadyChecked == false) {                
    Sprintln(F("Random check, baby. "));            
    randomCheck();
  }  else if (activated == HIGH) {    
    Sprintln(F("Who turned me on?"));
    
    arm.isHome(false);
    door.isHome(false);
    
    arm.reattach();
    door.reattach();
    flag.reattach();
       
    if(impatient) {
      Sprint(F("I'm impatient right now - for the "));       
      impatientCount++;
      Sprint(impatientCount);
      Sprintln(F(" time!"));
    } else if (!impatient && (millis() - activatedTimestamp <= IMPATIENT_INTERVAL_TIME)) { 
      //increase impatient trigger count if box is operated repeatedly in interval   
      impatientThresholdCount++;
      Sprint(F("Getting impatient... "));
      Sprint(impatientThresholdCount);
      Sprint(F(" of "));
      Sprintln(IMPATIENT_INTERVAL_THRESHOLD);
      Sprint(F("Time difference is: "));
      Sprintln(millis() - activatedTimestamp);      
    }      

    //use specific moves in impatient mode and set the correct color
    if (impatient) {
      selectedBehaviour = determineImpatientBehaviour(); 
      setColor(impatientColors[impatientCount % NUM_MAX_IMPATIENT_ACTION]); 
    } else {      
      selectedBehaviour  = determineNormalBehaviour();
      setColor(wheel(random(0,255)));       
    } 

    //update activated after impatient check
    activatedTimestamp = millis();

    Sprint(F("-------- Starting behaviour: ["));
    Sprint(selectedBehaviour);
    Sprintln(F("]"));
    Sprint(F("-------- "));
    switch(selectedBehaviour) { 
      case 1: Sprintln(F("Normal flip")); goOpenDoorAndFlipThatSwitch(); break;
      case 2: Sprintln(F("Check")); check(); break;
      case 3: Sprintln(F("Multi try")); multiTry(); break;
      case 4: Sprintln(F("Check and return")); checkReturn(); break;
      case 5: Sprintln(F("Check, check and return")); checkCheckReturn(); break;
      case 6: Sprintln(F("Afraid")); afraid(); break;
      case 7: Sprintln(F("Oh, wait")); ohWait(); break;
      case 8: Sprintln(F("Crazy slow")); crazySlow(); break;        
      case 9: Sprintln(F("Matrix")); matrix(); break;
      case 10: Sprintln(F("Crazy door")); crazyDoor(); break;
      case 11: Sprintln(F("Drive away")); driveAway(); break;      
      case 12: Sprintln(F("Back and Forth")); moveBackAndForth(); break;
      case 14: Sprintln(F("low flapping around")); lowFlappingAround(); break;  
      case 15: Sprintln(F("turn off, wait")); turnOffThenWait(); break;  
      case 16: Sprintln(F("long wait after turnoff")); turnOffwaitAndFlipDoor(); break; 
      //case 17:  Sprintln(F("White flag")); whiteflag(); break;         
      case 18: Sprintln(F("Measure Distance")); turnOffIfUserIsDetected(); break;    
      case 100: Sprintln(F("Vibrating")); vibrating(); break;  
      case 101: Sprintln(F("Flapping door around")); flappingAround(); break;      
      case 102: Sprintln(F("Angry turn-off")); angryTurnOff(); break;
      default: Sprintln(F("Default")); goOpenDoorAndFlipThatSwitch(); break;   
      }       
      interrupted = false; //reset the interrupted state - i'm done now...
  } // end switch-if   

  //turn off leds after 20sec inactivity
  if (millis() - activatedTimestamp >= 20000 ) {
    setColor(black);
  }
  
  arm.waitAndDetatch();
  door.waitAndDetatch();
  flag.waitAndDetatch();  
  randCheck = random(1, 500000);  
}

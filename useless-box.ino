#include <Servo.h>
#include <Bounce2.h>

#define MIN_ARM_DEG 15
#define MAX_ARM_DEG 180

#define MIN_DOOR_DEG 2
#define MAX_DOOR_DEG 120

#define MIN_FLAG_DEG 60
#define MAX_FLAG_DEG 160

#define MECANIC_SPEED_PER_DEGREE 2 // 2 msec to move 1 degree
#define MECANIC_DELAY_SWITCH 50 // 50 msec to acknowledge a hit

#define NUM_BEHAVIOURS 10

#define POS_HOME_ARM MIN_ARM_DEG
#define POS_SWITCH_ARM MAX_ARM_DEG
#define POS_ARM_PEEK (MAX_ARM_DEG - 40)
#define POS_ARM_CHECK_NINJA (MIN_ARM_DEG + 20)
#define POS_ARM_NEAR_SWITCH (MIN_ARM_DEG + 40)

#define POS_HOME_DOOR MIN_DOOR_DEG
#define POS_DOOR_MEDIUM_OPEN (MAX_DOOR_DEG - 20)
#define POS_DOOR_FULL_OPEN MAX_DOOR_DEG
#define POS_DOOR_CHECK_NINJA (MIN_DOOR_DEG + 40)

#define POS_FLAG_HIDDEN MIN_FLAG_DEG
#define POS_FLAG_RAISED MAX_FLAG_DEG
#define POS_FLAG_TILTED (MAX_FLAG_DEG - 40)


Servo arm;
Servo door;
Servo flag;

const int arm_servo_pin = 8;
const int door_servo_pin = 6;
const int flag_servo_pin = 5;

const int direction_pin = 13;
const int throttle_pin = 11;

const int distance_pin = 3;

int current_arm_speed = 0;
int current_door_speed = 0;
int current_flag_speed = 0;

int last_arm_write = POS_HOME_ARM;
int last_door_write = POS_HOME_DOOR;
int last_flag_write = POS_FLAG_HIDDEN;
boolean is_arm_home = true;
boolean is_door_home = true;
boolean is_flag_home = true;

const int box_switch  = 2;
Bounce bouncer = Bounce();

int activated = LOW;

int randBehaviour = 1;
long randCheck = 1;
boolean hasAlreadyChecked = true;
boolean interrupted = false;

int lastDistance = 0;
int threshold = 200;
unsigned long timer;


void setup() {
  // put your setup code here, to run once:
  pinMode(box_switch, INPUT);

  pinMode(direction_pin, OUTPUT);  
  
  bouncer.attach(box_switch);

  // Debug
  Serial.begin(9600);
  Serial.println("Started.");
  

  arm.attach(arm_servo_pin);
  door.attach(door_servo_pin);
  flag.attach(flag_servo_pin);

  
  move_arm(POS_HOME_ARM);
  move_door(POS_HOME_DOOR);
  move_flag(POS_FLAG_HIDDEN);
  
 
  randomSeed(analogRead(0));
}

void move_arm(int degree) {

  Serial.print("Moving arm to position : ");
  Serial.print(degree);
  Serial.println("deg.");
  
  // Update the switch value.
  bouncer.update();
  int val = bouncer.read();

  // Check the last command we gave to the servo
  int current_degree = arm.read();
  last_arm_write = current_degree;

  // And then moves ! degree by degree, checking the switch position each time
  while (current_degree != degree) {
    if (current_degree < degree) {
      current_degree++;
    }
    else {
      current_degree--;
    }

    arm.write(current_degree);
    delay(MECANIC_SPEED_PER_DEGREE + current_arm_speed);

    bouncer.update();
    int current_value = bouncer.read();

    if (current_value != val) {
      Serial.println("/!\\ Interrupted - The switch was operated while I was moving !");
      interrupted = true;
      break;
    }
  }

}

void move_door(int degree) {
  
  Serial.print("Moving door to position : ");
  Serial.print(degree);
  Serial.println("deg.");
  
  // Update the switch value.
  bouncer.update();
  int val = bouncer.read();

  // Check the last command we gave to the servo
  int current_degree = arm.read();
  last_door_write = current_degree;

  // And then moves ! degree by degree, checking the switch position each time
  while (current_degree != degree) {
    if (current_degree < degree) {
      current_degree++;
    }
    else {
      current_degree--;
    }

    arm.write(current_degree);
    delay(MECANIC_SPEED_PER_DEGREE + current_door_speed);

    bouncer.update();
    int current_value = bouncer.read();

    if (current_value != val) {
      Serial.println("/!\\ Interrupted - The switch was operated while I was moving !");
      interrupted = true;
      break;
    }
  }

}

void move_flag(int degree) {
 
  Serial.print("Moving flag to position : ");
  Serial.print(degree);
  Serial.println("deg.");
  
  // Update the switch value.
  bouncer.update();
  int val = bouncer.read();

  // Check the last command we gave to the servo
  int current_degree = arm.read();
  last_flag_write = current_degree;

  // And then moves ! degree by degree, checking the switch position each time
  while (current_degree != degree) {
    if (current_degree < degree) {
      current_degree++;
    }
    else {
      current_degree--;
    }

    arm.write(current_degree);
    delay(MECANIC_SPEED_PER_DEGREE + current_flag_speed);

    bouncer.update();
    int current_value = bouncer.read();

    if (current_value != val) {
      Serial.println("/!\\ Interrupted - The switch was operated while I was moving !");
      interrupted = true;
      break;
    }
  }
 
}

// A "soft" delay function
// (This function can be interrupted by manual switch change)
void soft_delay(int msec) {

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


void forward(int speed = 100)
{
  digitalWrite(direction_pin, HIGH); //Establishes LEFT direction of Channel B  
  analogWrite(throttle_pin, speed);   //Spins the motor on Channel B at half speed
}

void backward(int speed = 100)
{
  digitalWrite(direction_pin, LOW); //Establishes LEFT direction of Channel B  
  analogWrite(throttle_pin, speed);   //Spins the motor on Channel B at half speed
}

void halt()
{
  analogWrite(throttle_pin, 0);   //Spins the motor on Channel B at half speed
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
  current_arm_speed = msec; // MAXIMUM POWWAAAAAA 
  move_arm(POS_SWITCH_ARM); // Go to the switch
  delay(MECANIC_DELAY_SWITCH); // Wait for the switch to acknowledge before returning in the loop
}

// Go near the arm, but not as close as to push the switch, and then retracts a bit
void tryFail(int msec = 5) {  
  current_arm_speed = msec; 
  move_arm(POS_ARM_NEAR_SWITCH);
  move_arm(POS_ARM_NEAR_SWITCH + 7); 
}

// Open the lid stealthly. User does not see me. I'm a NINJA !
void goStealthCheck() {  
  current_arm_speed = 30;   
  move_arm(POS_ARM_CHECK_NINJA);
  move_arm(POS_ARM_NEAR_SWITCH + 7);  
}

void openDoorNija() {
  current_door_speed = 30; 
  move_door(POS_DOOR_CHECK_NINJA);
}

void openDoorMedium(int msec = 0) {
  current_door_speed = msec;
  move_door(POS_DOOR_MEDIUM_OPEN);
  delay(MECANIC_DELAY_SWITCH);
}

void openDoorFull(int msec = 0) {
  current_door_speed = msec;
  move_door(POS_DOOR_FULL_OPEN);
  delay(MECANIC_DELAY_SWITCH);
}

// Ok, back home now
void backHome() { 
  current_arm_speed = 0; 
  move_arm(POS_HOME_ARM); 
}

// Open the lid to see out
void goCheck(int msec = 10) {  
  current_arm_speed = msec;   
  move_arm(POS_ARM_PEEK); 
}

void closeDoor() {
  current_door_speed = 0;
  move_door(POS_HOME_DOOR);
}
void raiseFlag(int msec = 0) {
  current_flag_speed = msec;
  move_flag(POS_FLAG_RAISED);
  delay(MECANIC_DELAY_SWITCH);
}

void tiltFlag(int msec = 0) {
  current_flag_speed = msec;
  move_flag(POS_FLAG_TILTED);
  delay(MECANIC_DELAY_SWITCH);
}

void hideFlag(int msec = 0) {
  current_flag_speed = msec;
  move_flag(POS_FLAG_HIDDEN);
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
    forward();
    delay(300);
    halt();
  }
  if (!interrupted) delay(100);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();  
}


void whiteFlag() 
{
  if (!interrupted) openDoorFull();
  if (!interrupted) waveFlag(5);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) delay(2000);
  if(!interrupted && detect()) {
    waveFlag(7);
  }  
  if (!interrupted) closeDoor();
}




// Check once, wait, then flip the switch
void check() {
  if (!interrupted) openDoorFull();
  if (!interrupted) goCheck();
  if (!interrupted) soft_delay(1000);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}


// Check, return back home, then flip
void checkReturn() {
  if (!interrupted) openDoorFull();
  if (!interrupted) goCheck();
  if (!interrupted) soft_delay(1500);
  if (!interrupted) backHome();
  if (!interrupted) soft_delay(800);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}


// Multi tries. At the end, succeeds...
void multiTry() {
  if (!interrupted) openDoorFull();
  if (!interrupted) tryFail();
  if (!interrupted) soft_delay(500);
  if (!interrupted) tryFail();
  if (!interrupted) soft_delay(500);
  if (!interrupted) tryFail();
  if (!interrupted) soft_delay(500);
  if (!interrupted) backHome();
  if (!interrupted) soft_delay(500);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}

// Check once slowly, return, check again, return to stealth position, then flip
void checkCheckReturn() {
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) soft_delay(1500);
  if (!interrupted) backHome();
  if (!interrupted) closeDoor();
  if (!interrupted) openDoorMedium();
  if (!interrupted) goCheck();
  if (!interrupted) closeDoor();
  if (!interrupted) soft_delay(1000);
  if (!interrupted) openDoorNija();
  if (!interrupted) goStealthCheck();
  if (!interrupted) soft_delay(800);
  if (!interrupted) goFlipThatSwitch();
  if (!interrupted) closeDoor();
}

void afraid() {
  if (!interrupted) openDoorFull();
  if (!interrupted) tryFail(0);  
  if (!interrupted) goCheck(0);
  if (!interrupted) closeDoor();  
  if (!interrupted) soft_delay(1500);
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
  if (!interrupted) soft_delay(700);
  if (!interrupted) goCheck(2); // Woops. Forgot something ?
  if (!interrupted) soft_delay(1000);
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

/**

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

 */

void loop() {
  // Update the switch position
  bouncer.update();
  activated = bouncer.read();

  // If the user wants me to go out
  if (activated == HIGH) {

    Serial.println("Going out ! Yippee ! ");
    
    is_arm_home = false; // We're just leaving !
    arm.attach(arm_servo_pin);
    door.attach(door_servo_pin);
    flag.attach(flag_servo_pin);

    // Find a new behaviour for next time
    randBehaviour = random(1, NUM_BEHAVIOURS);
    
    switch(randBehaviour) { // What animation shall I do today ?
      case 1: // Standard
        goFlipThatSwitch();
        break;
      case 2:
        check();
        break;
      case 3:
        multiTry();
        break;
      case 4:
        checkReturn();
        break;
      case 5:
        checkCheckReturn();
        break;
      case 6:
        afraid();
        break;
      case 7:
        ohWait();
        break;
      case 8:
        driveAway();
      case 9:
        whiteFlag();
      case 10:
        matrix();
      case 11:
        
      default:
        goFlipThatSwitch();
        break;
    }

    interrupted = false; // Now that I have finished, I can rest.
    hasAlreadyChecked = false; // We can check once after that

  } else if (is_arm_home == false) {

    // If we're not home yet, we shall go there !
    Serial.println("Going back home");
    backHome();
    
    
  } else if ( randCheck < 5 && hasAlreadyChecked == false) {
      
      // We only check once after an activation
      hasAlreadyChecked = true;
      is_arm_home = false;      
      
      Serial.println("Random check, baby. ");
      arm.attach(arm_servo_pin);
      door.attach(door_servo_pin);
      flag.attach(flag_servo_pin);
      
      if (!interrupted) goCheck(5);
      if (!interrupted) soft_delay(1000);
      
  }

  
  if (arm.read() == POS_HOME_ARM && is_arm_home == false) {
    Serial.println("Powering off the arm servo ...");
    int time_to_wait = abs(last_arm_write-POS_HOME_ARM)*(current_arm_speed + MECANIC_SPEED_PER_DEGREE);
    delay(time_to_wait);
    is_arm_home = true;
    arm.detach();
  }

  if (door.read() == POS_HOME_DOOR && is_door_home == false) {
    Serial.println("Powering off the door servo ...");
    int time_to_wait = abs(last_door_write-POS_HOME_ARM)*(current_door_speed + MECANIC_SPEED_PER_DEGREE);
    delay(time_to_wait);
    is_door_home = true;
    door.detach();
  }

  if (flag.read() == POS_FLAG_HIDDEN && is_flag_home == false) {
    Serial.println("Powering off the flag servo ...");
    int time_to_wait = abs(last_flag_write-POS_FLAG_HIDDEN)*(current_flag_speed + MECANIC_SPEED_PER_DEGREE);
    delay(time_to_wait);
    is_flag_home = true;
    flag.detach();
  }

  // Random number to check sometimes
  randCheck = random(1, 1000000);

}

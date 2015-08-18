#ifndef ServoControl_h
#define ServoControl_h


#include <Servo.h>
#include <Bounce2.h>
#include <inttypes.h>

class ServoControl
{
  public: 
    ServoControl(char name[]);
    bool move(int degree);
    bool move(int degree, int speed);
    void attach(Bounce* bouncer, int pin, int pos_home);    
    void interruptable(bool interruptable);
    void waitAndDetatch();
    void reattach();
    uint8_t getLastWrite();    
    bool isHome();
    void setHome(int pos_home);
    void isHome(bool home);
  protected:
    Bounce* bouncer;
    Servo servo;        
    uint8_t last_write;
    uint8_t pos_home;
    uint8_t current_speed;
    bool is_home;
    bool is_interruptable;
    uint8_t pin;
    char* name;
};

#endif

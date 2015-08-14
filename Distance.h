#ifndef Distance_h
#define Distance_h

#include <inttypes.h>

class Distance
{
  public: 
    Distance();
    bool detect();    
    void attach(int pin, int threshold);
    void attach(int pin);
  protected:
    uint8_t pin;
    uint16_t lastDistance;
    uint16_t threshold;
    unsigned long timer;
};

#endif

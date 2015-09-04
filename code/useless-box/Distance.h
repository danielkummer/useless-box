#ifndef Distance_h
#define Distance_h

#include <inttypes.h>

class Distance
{
  public: 
    Distance();
    bool detect(int threshold);    
    void attach(int pin, int read_time);
    void attach(int pin);
  protected:
    uint8_t pin;
    uint16_t lastDistance;
    uint16_t threshold;
    uint16_t read_time;
    unsigned long timer;
};

#endif

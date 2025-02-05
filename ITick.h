#ifndef ITICK_H
#define ITICK_H

#include <stdint.h>

class ITick {
public:
    virtual ~ITick() {}
    
    virtual bool delay(uint32_t time) = 0;
    virtual uint32_t elapsed(uint32_t time) = 0;
    virtual uint32_t getElapsed(uint32_t time1, uint32_t time2) = 0;
    virtual uint32_t getTickCount(void) = 0;
    virtual void tickUpdate() = 0;
    virtual bool tickCheck(uint32_t time) = 0;
};

#endif // ITICK_H 
#ifndef STM32TICKIMPL_H
#define STM32TICKIMPL_H

#if defined(USE_HAL_DRIVER)
#include "ITick.h"
#include "main.h"

class STM32TickImpl : public ITick {
public:
    STM32TickImpl();
    virtual ~STM32TickImpl();

    virtual bool delay(uint32_t time) override;
    virtual uint32_t elapsed(uint32_t time) override;
    virtual uint32_t getElapsed(uint32_t time1, uint32_t time2) override;
    virtual uint32_t getTickCount(void) override;
    virtual void tickUpdate() override;
    virtual bool tickCheck(uint32_t time) override;

private:
    uint32_t tickTime;
};

#endif
#endif // STM32TICKIMPL_H 
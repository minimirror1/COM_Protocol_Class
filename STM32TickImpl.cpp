#if defined(USE_HAL_DRIVER)
#include "STM32TickImpl.h"

STM32TickImpl::STM32TickImpl() : tickTime(0) {
}

STM32TickImpl::~STM32TickImpl() {
}

bool STM32TickImpl::delay(uint32_t time) {
    uint32_t currentTick = HAL_GetTick();
    return (currentTick - tickTime) >= time;
}

uint32_t STM32TickImpl::elapsed(uint32_t time) {
    return HAL_GetTick() - time;
}

uint32_t STM32TickImpl::getElapsed(uint32_t time1, uint32_t time2) {
    return time2 - time1;
}

uint32_t STM32TickImpl::getTickCount(void) {
    return HAL_GetTick();
}

void STM32TickImpl::tickUpdate() {
    tickTime = HAL_GetTick();
}

bool STM32TickImpl::tickCheck(uint32_t time) {
    return delay(time);
}

#endif 
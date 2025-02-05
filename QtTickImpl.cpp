#ifdef QT_PLATFORM
#include "QtTickImpl.h"

QtTickImpl::QtTickImpl() : tickTime(0) {
    timer.start();
}

QtTickImpl::~QtTickImpl() {
}

bool QtTickImpl::delay(uint32_t time) {
    return (timer.elapsed() - tickTime) >= time;
}

uint32_t QtTickImpl::elapsed(uint32_t time) {
    return timer.elapsed() - time;
}

uint32_t QtTickImpl::getElapsed(uint32_t time1, uint32_t time2) {
    return time2 - time1;
}

uint32_t QtTickImpl::getTickCount(void) {
    return timer.elapsed();
}

void QtTickImpl::tickUpdate() {
    tickTime = timer.elapsed();
}

bool QtTickImpl::tickCheck(uint32_t time) {
    return delay(time);
} 
#endif

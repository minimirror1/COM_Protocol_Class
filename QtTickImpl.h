#ifndef QTTICKIMPL_H
#define QTTICKIMPL_H

#ifdef QT_PLATFORM
#include "ITick.h"
#include <QElapsedTimer>

class QtTickImpl : public ITick {
public:
    QtTickImpl();
    virtual ~QtTickImpl();

    virtual bool delay(uint32_t time) override;
    virtual uint32_t elapsed(uint32_t time) override;
    virtual uint32_t getElapsed(uint32_t time1, uint32_t time2) override;
    virtual uint32_t getTickCount(void) override;
    virtual void tickUpdate() override;
    virtual bool tickCheck(uint32_t time) override;

private:
    QElapsedTimer timer;
    qint64 tickTime;
};

#endif

#endif // QTTICKIMPL_H 

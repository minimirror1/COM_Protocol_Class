#ifndef I_SERIAL_INTERFACE_H_
#define I_SERIAL_INTERFACE_H_

// stm32
#if defined(USE_HAL_DRIVER)
#include "main.h"
#endif

//qt

class ISerialInterface {
public:
    virtual ~ISerialInterface() = default;
    
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    virtual size_t read(uint8_t* buffer, size_t length) = 0;
    virtual bool isOpen() = 0;
    virtual void flush() = 0;
};

#endif /* I_SERIAL_INTERFACE_H_ */

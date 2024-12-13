#if defined(USE_HAL_DRIVER)
#include "UART_Class.h"

class STM32SerialImpl : public ISerialInterface {
public:
    STM32SerialImpl(UART_HandleTypeDef* huart);
    virtual bool open() override;
    virtual void close() override;
    virtual size_t write(const uint8_t* data, size_t length) override;
    virtual size_t read(uint8_t* buffer, size_t length) override;
    virtual bool isOpen() override;
    virtual void flush() override;

private:
    UART_HandleTypeDef* huart_;
};
#endif

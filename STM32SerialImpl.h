#if defined(USE_HAL_DRIVER)
#include "ISerialInterface.h"
#include "UART_Class.h"

class STM32SerialImpl : public ISerialInterface {
public:
    STM32SerialImpl(UART_HandleTypeDef* huart, IRQn_Type UART_IRQn_);
    virtual bool open() override;
    virtual void close() override;
    virtual size_t write(const uint8_t* data, size_t length) override;
    virtual size_t read(uint8_t* buffer, size_t length) override;
    virtual bool isOpen() override;
    virtual void flush() override;

    // UART 콜백 함수 추가
    void TxCpltCallback(UART_HandleTypeDef *huart);
    void RxCpltCallback(UART_HandleTypeDef *huart);
    void loop();

    // LED 초기화 함수 추가
    void init_txLed(GPIO_TypeDef *Port, uint16_t Pin, GPIO_PinState OnState);
    void init_rxLed(GPIO_TypeDef *Port, uint16_t Pin, GPIO_PinState OnState);
    void init_rs485(GPIO_TypeDef *Port, uint16_t Pin);

private:
    UART_HandleTypeDef* huart_;
    IRQn_Type UART_IRQn_;
    Serial serial_;
};

#endif

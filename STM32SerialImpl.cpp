#include "STM32SerialImpl.h"

STM32SerialImpl::STM32SerialImpl(UART_HandleTypeDef* huart, IRQn_Type UART_IRQn) : huart_(huart), UART_IRQn_(UART_IRQn) {
    serial_.init(huart, UART_IRQn_);
}

void STM32SerialImpl::init(){
	serial_.init(huart_, UART_IRQn_);
}

bool STM32SerialImpl::open() {
    return true; // UART_Class는 init 시점에 초기화되므로 항상 true 반환
}

void STM32SerialImpl::close() {
    // UART_Class는 별도의 close 기능이 없음
}

size_t STM32SerialImpl::write(const uint8_t* data, size_t length) {
    return serial_.write((uint8_t*)data, length);
}

size_t STM32SerialImpl::read(uint8_t* buffer, size_t length) {
    size_t readCount = 0;
    while (readCount < length && serial_.available() > 0) {
        buffer[readCount++] = serial_.read();
    }
    return readCount;
}

bool STM32SerialImpl::isOpen() {
    return true; // UART_Class는 항상 열려있는 상태로 간주
}

void STM32SerialImpl::flush() {
    // 송신 버퍼가 비워질 때까지 대기
    while (serial_.available() > 0) {
        serial_.read();
    }
}

void STM32SerialImpl::TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == huart_) {
        serial_.TxCpltCallback(huart);
    }
}

void STM32SerialImpl::RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == huart_) {
        serial_.RxCpltCallback(huart);
    }
}

void STM32SerialImpl::loop() {
    serial_.loop();
}

void STM32SerialImpl::init_txLed(GPIO_TypeDef *Port, uint16_t Pin, GPIO_PinState OnState) {
    serial_.init_txLed(Port, Pin, OnState);
}

void STM32SerialImpl::init_rxLed(GPIO_TypeDef *Port, uint16_t Pin, GPIO_PinState OnState) {
    serial_.init_rxLed(Port, Pin, OnState);
}

void STM32SerialImpl::init_rs485(GPIO_TypeDef *Port, uint16_t Pin) {
    serial_.init_rs485(Port, Pin);
}


/*
 * com_protocol_class.h
 *
 *  Created on: Nov 21, 2024
 *      Author: minim
 */

#ifndef COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_
#define COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_

#include "UART_Class.h"
#include "cpp_tick.h"

class Com_Protocol {
public:
    Com_Protocol();
    virtual ~Com_Protocol();

    void initialize(Serial* uart, Tick* tick);
    void sendData(uint16_t receiverId, uint16_t senderId, uint16_t cmd,
                 const uint8_t* data, size_t length);
    void receiveData(uint8_t* buffer, size_t length);
    bool isDataAvailable() const;
    void processReceivedData();

protected:
    // 사용자가 선택적으로 재정의할 수 있는 가상 함수들
    virtual void handlePing(uint16_t senderId, uint8_t* payload, size_t length);
    virtual void handleData(uint16_t senderId, uint8_t* payload, size_t length) {}
    virtual void handleConfig(uint16_t senderId, uint8_t* payload, size_t length) {}
    virtual void handleUnknownCommand(uint16_t cmd) {}

    // 명령어 정의
    static const uint16_t CMD_PING = 0x0001;
    static const uint16_t CMD_PONG = 0x0011;  // PONG 응답용 명령어 추가
    static const uint16_t CMD_DATA = 0x0002;
    static const uint16_t CMD_CONFIG = 0x0003;

private:
    static const uint8_t START_MARKER = 0x16;
    static const uint8_t START_SEQUENCE_LENGTH = 4;
    static const uint32_t PACKET_TIMEOUT_MS = 100;
    
    enum class ReceiveState {
        WAIT_START,
        READ_LENGTH,
        READ_RECEIVER_ID,
        READ_SENDER_ID,
        READ_CMD,
        READ_PAYLOAD,
        //VERIFY_CRC,
        
    };
    
    ReceiveState currentState_;
    uint32_t lastReceiveTime_;
    uint16_t expectedLength_;
    uint16_t receiverId_;
    uint16_t senderId_;
    uint16_t payloadIndex_;
    uint8_t startSequenceCount_;
    
    Serial* uart_;
    Tick* tick_;
    uint8_t* receiveBuffer_;
    size_t bufferLength_;
    
    static const uint16_t CRC16_INIT = 0xFFFF;
    static const uint16_t CRC16_POLY = 0x1021;  // CCITT 다항식
    
    uint16_t calculateCRC16(const uint8_t* data, size_t length);
    
    uint16_t receivedCRC_;
    uint16_t calculatedCRC_;
    uint16_t cmd_;  // CMD 필드
    
    void processCommand(uint16_t senderId, uint16_t receiverId, 
                       uint16_t cmd, uint8_t* payload, size_t payloadLength);
};

#endif /* COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_ */

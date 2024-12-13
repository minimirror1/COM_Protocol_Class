/*
 * com_protocol_class.h
 *
 *  Created on: Nov 21, 2024
 *      Author: minim
 */

#ifndef COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_
#define COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_

#include "ISerialInterface.h"
#include "cpp_tick.h"

// 파일 전송 단계 정의
enum class FileTransferStage : uint8_t {
	REQUEST_RECEIVE = 1,     // 파일 수신 요청
	READY_TO_RECEIVE = 2,    // 수신 준비 완료
	RECEIVING_DATA = 3,      // 데이터 수신 중
	VERIFY_CHECKSUM = 4      // 체크섬 검증
};

class Com_Protocol {
public:
    Com_Protocol();
    virtual ~Com_Protocol();

    void initialize(ISerialInterface* serial, Tick* tick);
    void sendData(uint16_t receiverId, uint16_t senderId, uint16_t cmd,
                 const uint8_t* data, size_t length);
    void receiveData(uint8_t* buffer, size_t length);
    bool isDataAvailable() const;
    void processReceivedData();

    // ping 요청 함수 추가
    void sendPing(uint16_t targetId);

protected:
    // 사용자가 선택적으로 재정의할 수 있는 가상 함수들
    virtual void handlePing(uint16_t senderId, uint8_t* payload, size_t length);
    virtual void handleData(uint16_t senderId, uint8_t* payload, size_t length) {}
    virtual void handleConfig(uint16_t senderId, uint8_t* payload, size_t length) {}
    virtual void handleUnknownCommand(uint16_t cmd) {}

    // 명령어 정의
    static const uint16_t CMD_ACK_BIT = 0x8000;
    static const uint16_t CMD_PING = 0x0001;
    static const uint16_t CMD_PONG = CMD_PING | CMD_ACK_BIT;  // PONG 응답용 명령어 추가
    static const uint16_t CMD_FILE_RECEIVE = 0x0002;  // CMD_FILE을 CMD_FILE_RECEIVE로 변경
    static const uint16_t CMD_FILE_RECEIVE_ACK = CMD_FILE_RECEIVE | CMD_ACK_BIT;
    static const uint16_t CMD_CONFIG = 0x0003;



    // 파일 전송 관련 상수
    static const uint8_t MAX_RETRY_COUNT = 5;
    static const uint16_t MAX_FILENAME_LENGTH = 256;
    static const uint32_t MAX_FILE_SIZE = 1024 * 1024; // 1MB

    // 파일 전송 관련 가상 함수 추가
    virtual void handleFileReceive(uint16_t senderId, uint8_t* payload, size_t length);
    //virtual void handleFileReceiveAck(uint16_t senderId, uint8_t* payload, size_t length);

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
    
    ISerialInterface* serial_;
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

    // 파일 전송 관련 멤버 변수
    struct FileTransferContext {
        char filename[MAX_FILENAME_LENGTH];
        uint32_t fileSize;
        uint32_t bufferSize;
        uint32_t currentIndex;
        uint8_t retryCount;
        bool isTransferring;
        bool isSender;
        uint32_t receivedSize;
        uint16_t checksum;
    } fileContext_;

    void resetFileTransferContext();
    void processFileTransfer(FileTransferStage stage, uint8_t* payload, size_t length);
    void sendFileAck(uint16_t receiverId, FileTransferStage stage, bool success, 
                    uint32_t data = 0);
    uint32_t calculateFileChecksum(uint8_t* data, size_t length);

    // 파일 수신 응답 전송 함수 추가
    void sendFileReceiveAck(uint16_t receiverId, FileTransferStage stage, 
                           bool success, uint32_t data = 0);
};

#endif /* COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_ */

/*
 * com_protocol_class.h
 *
 *  Created on: Nov 21, 2024
 *      Author: minim
 */

#ifndef COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_
#define COM_PROTOCOL_CLASS_COM_PROTOCOL_CLASS_H_

#include "ISerialInterface.h"
#include "ITick.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// 파일 전송 단계 정의
enum class FileTransferStage : uint8_t {
	REQUEST_RECEIVE = 1,     // 파일 수신 요청
	READY_TO_RECEIVE = 2,    // 수신 준비 완료
	RECEIVING_DATA = 3,      // 데이터 수신 중
	VERIFY_CHECKSUM = 4      // 체크섬 검증
};

// PlayControl 상태 정의
enum class PlayControlState : uint8_t {
    PLAY_ONE = 0x01,
    PLAY_REPEAT = 0x02,    
    PAUSE = 0x03,
    STOP = 0x04    
};

// 모터 타입 정의
enum class MotorType : uint8_t {
    MOTOR_NULL = 0,
    MOTOR_RC = 1,
    MOTOR_AC = 2,
    MOTOR_BL = 3,
    MOTOR_ZER = 4,
    MOTOR_DXL = 5
};

class Com_Protocol {
public:
    // 생성자 매개변수 추가
    Com_Protocol(ISerialInterface* serial, ITick* tick, uint16_t my_id);
    virtual ~Com_Protocol();

    void sendData(uint16_t receiverId, uint16_t senderId, uint16_t cmd,
                 const uint8_t* data, size_t length);
    void receiveData(uint8_t* buffer, size_t length);
    bool isDataAvailable() const;
    void processReceivedData();

    
    void sendPing(uint16_t targetId);// ping 요청 함수
    void sendIdScan(uint16_t targetId);// id 스캔 요청 함수
    
    void sendSync();// 동기화 요청 함수        
    void sendSyncAck(uint16_t targetId, uint32_t timestamp);// 동기화 응답 함수

    // my_id getter 추가
    uint16_t getMyId() const { return my_id_; }

protected:
    // 파싱 전 : 사용자가 선택적으로 재정의할 수 있는 가상 함수들, 파싱 전에 호출되는 함수들
    /* 네트워크 0x0000 ~ 0x00FF */
    virtual void handlePing(uint16_t senderId, uint8_t* payload, size_t length);//CMD_PONG
    virtual void handleData(uint16_t senderId, uint8_t* payload, size_t length) {}
    virtual void handleConfig(uint16_t senderId, uint8_t* payload, size_t length) {}
    virtual void handleIdScan(uint16_t senderId, uint8_t* payload, size_t length);//CMD_ID_SCAN

    // 상태 동기화
    virtual void handleStatusSync(uint16_t senderId, uint8_t* payload, size_t length);//CMD_STATUS_SYNC
    // 제어
    virtual void handleMainPowerControl(uint16_t senderId, uint8_t* payload, size_t length);//CMD_MAIN_POWER_CONTROL
    virtual void handlePlayControl(uint16_t senderId, uint8_t* payload, size_t length);//CMD_PLAY_CONTROL
    void handleJogMoveCwCcw(uint16_t senderId, uint8_t* payload, size_t length);//CMD_JOG_MOVE_CW_CCW
    virtual void handleUnknownCommand(uint16_t cmd) {}

    
    // 파싱 후 : 호출되는 함수
    virtual void setMainPower(uint8_t powerFlag);//CMD_MAIN_POWER_CONTROL
    virtual void setJogMoveCwCcw(uint8_t id, uint8_t subId, uint32_t speed, uint8_t direction);//CMD_JOG_MOVE_CW_CCW
   

    // 명령어 정의
    /* 네트워크 0x0000 ~ 0x00FF */
    static const uint16_t CMD_ACK_BIT = 0x8000;
    static const uint16_t CMD_PING = 0x0001;
    static const uint16_t CMD_PONG = CMD_PING | CMD_ACK_BIT;  // PONG 응답용 명령어 추가
    static const uint16_t CMD_FILE_RECEIVE = 0x0002;  // CMD_FILE을 CMD_FILE_RECEIVE로 변경
    static const uint16_t CMD_FILE_RECEIVE_ACK = CMD_FILE_RECEIVE | CMD_ACK_BIT;
    static const uint16_t CMD_CONFIG = 0x0003;
    static const uint16_t CMD_ID_SCAN = 0x0004;
    static const uint16_t CMD_ID_SCAN_ACK = CMD_ID_SCAN | CMD_ACK_BIT;

    // 상태 동기화
    static const uint16_t CMD_STATUS_SYNC = 0x0010;
    static const uint16_t CMD_STATUS_SYNC_ACK = CMD_STATUS_SYNC | CMD_ACK_BIT;
    // 새 세션 연결 (인증 및 타임스탬프 포함)
    static const uint16_t CMD_SYNC = 0x0020;
    static const uint16_t CMD_SYNC_ACK = CMD_SYNC | CMD_ACK_BIT;


    /* 제어 0x0100 ~ 0x01FF */
    static const uint16_t CMD_MAIN_POWER_CONTROL = 0x0100;
    static const uint16_t CMD_MAIN_POWER_CONTROL_ACK = CMD_MAIN_POWER_CONTROL | CMD_ACK_BIT;

    static const uint16_t CMD_PLAY_CONTROL = 0x0110;
    static const uint16_t CMD_PLAY_CONTROL_ACK = CMD_PLAY_CONTROL | CMD_ACK_BIT;
    
    static const uint16_t CMD_JOG_MOVE_CW_CCW = 0x0120;
    static const uint16_t CMD_JOG_MOVE_CW_CCW_ACK = CMD_JOG_MOVE_CW_CCW | CMD_ACK_BIT;


    // 파일 전송 관련 상수
    static const uint8_t MAX_RETRY_COUNT = 5;
    static const uint16_t MAX_FILENAME_LENGTH = 256;
    static const uint32_t MAX_FILE_SIZE = 1024 * 1024; // 1MB

    // 파일 전송 관련 가상 함수 추가
    virtual void handleFileReceive(uint16_t senderId, uint8_t* payload, size_t length);
    //virtual void handleFileReceiveAck(uint16_t senderId, uint8_t* payload, size_t length);

    ITick* tick_;
    ISerialInterface* serial_;

    uint16_t my_id_;

private:
    static const uint8_t START_MARKER = 0x16;
    static const uint8_t START_SEQUENCE_LENGTH = 4;
    static const uint32_t PACKET_TIMEOUT_MS = 100;
    
    // 추가: 시퀀스 번호 점프 임계치
    static const uint16_t SEQUENCE_JUMP_THRESHOLD = 3;

    // 송신 및 수신 시퀀스 번호 관련 변수
    uint16_t currentSequenceNumber_;    // 송신 시퀀스 번호
    uint16_t expectedSequenceNumber_;   // 수신측에서 기대하는 다음 시퀀스 번호
    uint32_t missingPacketCount_;       // 누락된 패킷 수

    enum class ReceiveState {
        WAIT_START,
        READ_LENGTH,
        READ_RECEIVER_ID,
        READ_SENDER_ID,
        READ_CMD,
        READ_SEQ,         // 추가: 시퀀스 번호 읽기
        READ_PAYLOAD
    };
    
    ReceiveState currentState_;
    uint32_t lastReceiveTime_;
    uint16_t expectedLength_;
    
    uint16_t senderId_;
    uint16_t receivedId_;
    uint16_t payloadIndex_;
    uint8_t startSequenceCount_;
    
    uint8_t* receiveBuffer_;
    size_t bufferLength_;
    
    static const uint16_t CRC16_INIT = 0xFFFF;
    static const uint16_t CRC16_POLY = 0x1021;  // CCITT 다항식
    
    uint16_t calculateCRC16(const uint8_t* data, size_t length);
    
    uint16_t receivedCRC_;
    uint16_t calculatedCRC_;
    uint16_t cmd_;  // CMD 필드
    
    uint16_t seq_; // 수신된 시퀀스 번호
    
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

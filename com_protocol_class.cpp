/*
 * com_protocol_class.cpp
 *
 *  Created on: Nov 21, 2024
 *      Author: minim
 */
#include "main.h"
#include "string.h"
#include "com_protocol_class.h"
#include "ISerialInterface.h"

// 생성자: 멤버 변수 초기화 (새로운 시퀀스 관련 변수 포함)
Com_Protocol::Com_Protocol(ISerialInterface* serial, ITick* tick, uint16_t my_id) :
    serial_(serial), 
    tick_(tick),
    my_id_(my_id),  // my_id로 my_id_ 초기화
    receiveBuffer_(nullptr),
    bufferLength_(256),
    currentSequenceNumber_(0),
    expectedSequenceNumber_(0),
    missingPacketCount_(0),
    currentState_(ReceiveState::WAIT_START),
    lastReceiveTime_(0),
    expectedLength_(0),
    payloadIndex_(0),
    startSequenceCount_(0)
{
    resetFileTransferContext();
    receiveBuffer_ = new uint8_t[bufferLength_];
}

// 소멸자: 동적 할당된 메모리 해제
Com_Protocol::~Com_Protocol() {
    if (receiveBuffer_) {
        delete[] receiveBuffer_;
        receiveBuffer_ = nullptr;
    }
}

// 데이터 전송 함수 수정 (헤더에 시퀀스 번호 추가)
void Com_Protocol::sendData(uint16_t receiverId, uint16_t senderId, uint16_t cmd,
                          const uint8_t* data, size_t length) {
    if (!serial_) return;
    
    // 시작 시퀀스 전송 (필수)
    const uint8_t startMarker = START_MARKER;
    for (int i = 0; i < START_SEQUENCE_LENGTH; i++) {
        serial_->write(&startMarker, 1);
    }
    
    // 전체 길이 계산 및 전송 (필수)
    uint16_t totalLength = 8 + length + 2; // header(8) + payload + CRC(2)
    uint8_t lengthBytes[2] = {
        static_cast<uint8_t>(totalLength >> 8),
        static_cast<uint8_t>(totalLength & 0xFF)
    };
    serial_->write(lengthBytes, 2);
    
    // 헤더 생성 (시퀀스 번호 포함)
    uint8_t headerBytes[8] = {0,};
    headerBytes[0] = static_cast<uint8_t>(receiverId >> 8);
    headerBytes[1] = static_cast<uint8_t>(receiverId & 0xFF);
    headerBytes[2] = static_cast<uint8_t>(senderId >> 8);
    headerBytes[3] = static_cast<uint8_t>(senderId & 0xFF);
    headerBytes[4] = static_cast<uint8_t>(cmd >> 8);
    headerBytes[5] = static_cast<uint8_t>(cmd & 0xFF);
    headerBytes[6] = static_cast<uint8_t>(currentSequenceNumber_ >> 8);
    headerBytes[7] = static_cast<uint8_t>(currentSequenceNumber_ & 0xFF);
    
    serial_->write(headerBytes, 8);
    
    // 페이로드 전송
    if (length > 0) {
        serial_->write(data, length);
    }
    
    // CRC 계산 및 전송
    uint8_t* crcBuffer = new uint8_t[8 + length];
    memcpy(crcBuffer, headerBytes, 8);
    if (length > 0) {
        memcpy(crcBuffer + 8, data, length);
    }
    
    uint16_t crc = calculateCRC16(crcBuffer, 8 + length);
    delete[] crcBuffer;
    
    uint8_t crcBytes[2] = {
        static_cast<uint8_t>(crc >> 8),
        static_cast<uint8_t>(crc & 0xFF)
    };
    serial_->write(crcBytes, 2);
    
    // 송신 시퀀스 번호 증가
    currentSequenceNumber_++;
}

// 데이터 수신
void Com_Protocol::receiveData(uint8_t* buffer, size_t length) {
    if (!serial_ || !buffer) return;
    
    size_t bytesRead = serial_->read(buffer, length);
}

// 수신 가능한 데이터가 있는지 확인
bool Com_Protocol::isDataAvailable() const {
    return (serial_ && serial_->isOpen());
}

// processReceivedData() 함수에 READ_SEQ 상태 추가
void Com_Protocol::processReceivedData() {
    if (!serial_ || !receiveBuffer_) return;
    
    uint32_t currentTime = tick_->getTickCount();
    
    // 패킷 타임아웃 체크
    if (currentState_ != ReceiveState::WAIT_START && 
        (currentTime - lastReceiveTime_) > PACKET_TIMEOUT_MS) {
        currentState_ = ReceiveState::WAIT_START;
        startSequenceCount_ = 0;
    }

    uint8_t tempBuffer[1];
    while (serial_->read(tempBuffer, 1) > 0) {
        uint8_t data = tempBuffer[0];
        lastReceiveTime_ = currentTime;

        switch (currentState_) {
            case ReceiveState::WAIT_START:
                if (data == START_MARKER) {
                    startSequenceCount_++;
                    if (startSequenceCount_ == START_SEQUENCE_LENGTH) {
                        currentState_ = ReceiveState::READ_LENGTH;
                        payloadIndex_ = 0;
                        memset(receiveBuffer_, 0, bufferLength_);
                    }
                } else {
                    startSequenceCount_ = 0;
                }
                break;

            case ReceiveState::READ_LENGTH:
                receiveBuffer_[payloadIndex_++] = data;
                if (payloadIndex_ == 2) {
                    expectedLength_ = (receiveBuffer_[0] << 8) | receiveBuffer_[1];
                    if (expectedLength_ > bufferLength_ || expectedLength_ < 10) {
                        currentState_ = ReceiveState::WAIT_START;
                        startSequenceCount_ = 0;
                    } else {
                        currentState_ = ReceiveState::READ_RECEIVER_ID;
                        payloadIndex_ = 0;
                    }
                }
                break;

            case ReceiveState::READ_RECEIVER_ID:
                receiveBuffer_[payloadIndex_++] = data;
                if (payloadIndex_ == 2) {
                    uint16_t receivedId = (receiveBuffer_[0] << 8) | receiveBuffer_[1];
                    // 수신자 ID가 my_id와 일치하는지 확인
                    if (receivedId != my_id_ && receivedId != 0xFFFF) {  // 0xFFFF는 브로드캐스트 주소
                    	currentState_ = ReceiveState::WAIT_START;
                        startSequenceCount_ = 0;
                        payloadIndex_ = 0;
                        continue;
                    }
                    receivedId_ = receivedId;
                    currentState_ = ReceiveState::READ_SENDER_ID;
                    payloadIndex_ = 0;
                }
                break;

            case ReceiveState::READ_SENDER_ID:
                receiveBuffer_[payloadIndex_++] = data;
                if (payloadIndex_ == 2) {
                    senderId_ = (receiveBuffer_[0] << 8) | receiveBuffer_[1];
                    currentState_ = ReceiveState::READ_CMD;
                    payloadIndex_ = 0;
                }
                break;

            case ReceiveState::READ_CMD:
                receiveBuffer_[payloadIndex_++] = data;
                if (payloadIndex_ == 2) {
                    cmd_ = (receiveBuffer_[0] << 8) | receiveBuffer_[1];
                    currentState_ = ReceiveState::READ_SEQ;
                    payloadIndex_ = 0;
                }
                break;

            case ReceiveState::READ_SEQ:
                receiveBuffer_[payloadIndex_++] = data;
                if (payloadIndex_ == 2) {
                    seq_ = (receiveBuffer_[0] << 8) | receiveBuffer_[1];
                    if (cmd_ == CMD_SYNC) {
                        expectedSequenceNumber_ = 0;
                    } else {
                        uint16_t diff = seq_ - expectedSequenceNumber_;
                        if (diff == 0) {
                            expectedSequenceNumber_++;
                        } else if (diff > 0 && diff <= SEQUENCE_JUMP_THRESHOLD) {
                            missingPacketCount_ += diff;
                            expectedSequenceNumber_ = seq_ + 1;
                        } else if (diff > 0) {
                            missingPacketCount_ += diff;
                            expectedSequenceNumber_ = seq_ + 1;
                        } else {
                            currentState_ = ReceiveState::WAIT_START;
                            startSequenceCount_ = 0;
                            payloadIndex_ = 0;
                            continue;
                        }
                    }
                    currentState_ = ReceiveState::READ_PAYLOAD;
                    payloadIndex_ = 0;
                }
                break;

            case ReceiveState::READ_PAYLOAD:
                receiveBuffer_[payloadIndex_++] = data;
                if (payloadIndex_ == expectedLength_-8) {
                    // 마지막 2바이트는 CRC
                    receivedCRC_ = (receiveBuffer_[payloadIndex_ - 2] << 8) | 
                                  receiveBuffer_[payloadIndex_ - 1];
                    
                    // CRC 계산을 위한 임시 버퍼 생성
                    uint8_t* crcBuffer = new uint8_t[expectedLength_ - 2]; // CRC 제외한 크기                    
                    size_t crcIndex = 0;
                    
                    // 수신자 ID 복사
                    crcBuffer[crcIndex++] = (uint8_t)(receivedId_ >> 8);
                    crcBuffer[crcIndex++] = (uint8_t)(receivedId_ & 0xFF);
                    
                    // 송신자 ID 복사
                    crcBuffer[crcIndex++] = (uint8_t)(senderId_ >> 8);
                    crcBuffer[crcIndex++] = (uint8_t)(senderId_ & 0xFF);
                    
                    // CMD 복사
                    crcBuffer[crcIndex++] = (uint8_t)(cmd_ >> 8);
                    crcBuffer[crcIndex++] = (uint8_t)(cmd_ & 0xFF);
                    
                    // 시퀀스 번호 복사
                    crcBuffer[crcIndex++] = (uint8_t)(seq_ >> 8);
                    crcBuffer[crcIndex++] = (uint8_t)(seq_ & 0xFF);
                    
                    // 페이로드 복사 (있는 경우)
                    if (expectedLength_ > 10) { // 10 = header(8) + CRC(2)
                        memcpy(crcBuffer + crcIndex, 
                               receiveBuffer_,
                               expectedLength_ - 10);
                    }
                    
                    // CRC 계산
                    calculatedCRC_ = calculateCRC16(crcBuffer, expectedLength_ - 2);
                    delete[] crcBuffer;
                    
                    if (calculatedCRC_ == receivedCRC_) {
                        // CRC 검증 성공
                        processCommand(senderId_, my_id_, cmd_, 
                                    receiveBuffer_, expectedLength_ - 8);
                    } else {
                        // CRC 검증 실패
                    }
                    // 상태 초기화
                    currentState_ = ReceiveState::WAIT_START;
                    startSequenceCount_ = 0;
                }
                break;
        }
    }
}

// CRC16 XMODEM 테이블
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

uint16_t Com_Protocol::calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0x0000;  // XMODEM 초기값
    
    while (length--) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ *data++) & 0xFF];
    	//crc = (crc >> 8) ^ crc16_table[(crc & 0xFF) ^ *data++];

    }
    
    return crc;
}

// processCommand() 함수에 CMD_SYNC 처리 추가
void Com_Protocol::processCommand(uint16_t senderId, uint16_t receiverId, 
                                uint16_t cmd, uint8_t* payload, size_t payloadLength) {
    switch (cmd) {
        case CMD_PING:
            handlePing(senderId, payload, payloadLength);
            break;
            
        case CMD_FILE_RECEIVE:
            handleFileReceive(senderId, payload, payloadLength);
            break;
            
        case CMD_CONFIG:
            handleConfig(senderId, payload, payloadLength);
            break;
        case CMD_STATUS_SYNC:
        	handleStatusSync(senderId, payload, payloadLength);
        	break;
        case CMD_ID_SCAN:
            handleIdScan(senderId, payload, payloadLength);
            break;
        case CMD_SYNC:
            if (payloadLength >= 6) {
                uint32_t timestamp = (payload[0] << 24) | (payload[1] << 16) |
                                   (payload[2] << 8) | payload[3];
                uint16_t authToken = (payload[4] << 8) | payload[5];
                if (authToken == 0xABCD) {
                    expectedSequenceNumber_ = 0;
                    // 동기화 성공시 ACK 전송
                    sendSyncAck(senderId, timestamp);
                }
            }
            break;                        
        case CMD_MAIN_POWER_CONTROL:
        	handleMainPowerControl(senderId, payload, payloadLength);
        	break;
        case CMD_PLAY_CONTROL:
        	handlePlayControl(senderId, payload, payloadLength);
        	break;
        case CMD_JOG_MOVE_CW_CCW:
        	handleJogMoveCwCcw(senderId, payload, payloadLength);
        	break;
        default:
            handleUnknownCommand(cmd);
            break;
    }
}

void Com_Protocol::handlePing(uint16_t senderId, uint8_t* payload, size_t length) {
    // PING에 대한 응답으로 PONG 메시지 전송
    uint8_t pongPayload[] = "PONG";
    
    // PONG 메시지 전송 (송신자와 수신자 ID를 교체하여 응답)
    sendData(senderId, my_id_, CMD_PONG, pongPayload, 4);
}

void Com_Protocol::handleStatusSync(uint16_t senderId, uint8_t* payload, size_t length) {
    // 페이로드 길이 체크

    /* 상태 동기화 로직 구현  재정의 하여 측정된 값을 추가*/
    // 메인파워 상태
    uint8_t mainPowerStatus = 1; // 1: ON, 0: OFF
    // 모션 재생 상태
    uint8_t motionPlayStatus = 1; // 1:1회 재생, 2:반복 재생, 3:일시 정지 , 4:정지
    // 연속 구동시간
    uint32_t totalRunTime = 60000; // ms 단위의 전체 구동시간 획득
    // 동작 회차
    uint16_t currentCount = 1000; // 현재 동작 회차
    uint16_t totalCount = 2000; // 총 동작 회차
    // 전압
    uint16_t voltage = 3000; // 30.00V
    // 전류
    uint16_t current = 4000; // 40.00A
    // 모션 시간
    uint32_t motionCurrentTime = 10000; // 10.00s
    uint32_t motionEndTime = 20000; // 20.00s

    // 마지막 에러
    uint8_t f_error = 0;            // 0: 정상, 1: 에러
    uint8_t can_id = 0;             // CAN ID
    uint8_t can_sub_id = 0;         // CAN SUB ID
    MotorType motor_type = MotorType::MOTOR_NULL;
    char error_code_str[8] = {0};   // 8바이트 제한의 에러 코드 문자열
    /************************************************* */



    /*패킷 생성*/
    // 응답 패킷 준비 (29byte로 확장)
    uint8_t responsePayload[29] = {0,};
    
    
    // 시간 변환 (ms -> 시/분/초)
    uint8_t hours = totalRunTime / (1000 * 60 * 60);
    uint8_t minutes = (totalRunTime % (1000 * 60 * 60)) / (1000 * 60);
    uint8_t seconds = ((totalRunTime % (1000 * 60 * 60)) % (1000 * 60)) / 1000;

    // 상태 플레그
    responsePayload[0] = mainPowerStatus;
    responsePayload[1] = motionPlayStatus;
    
    // 시간 정보 입력
    responsePayload[2] = hours;
    responsePayload[3] = minutes;
    responsePayload[4] = seconds;
    
    // 동작 회차 정보 입력 (현재/총)

    responsePayload[5] = (currentCount >> 8) & 0xFF;
    responsePayload[6] = currentCount & 0xFF;
    responsePayload[7] = (totalCount >> 8) & 0xFF;
    responsePayload[8] = totalCount & 0xFF;
    
    // 에너지 정보 입력 (전압/전류)

    responsePayload[9] = (voltage >> 8) & 0xFF;
    responsePayload[10] = voltage & 0xFF;
    responsePayload[11] = (current >> 8) & 0xFF;
    responsePayload[12] = current & 0xFF;

    // 모션 시간 입력
    responsePayload[13] = (motionCurrentTime >> 8) & 0xFF;
    responsePayload[14] = motionCurrentTime & 0xFF;
    responsePayload[15] = (motionEndTime >> 8) & 0xFF;
    responsePayload[16] = motionEndTime & 0xFF;
    
    // 마지막 에러 입력
    responsePayload[17] = f_error;
    responsePayload[18] = can_id;
    responsePayload[19] = can_sub_id;
    responsePayload[20] = static_cast<uint8_t>(motor_type);

    // error_code_str을 responsePayload에 복사 (최대 8바이트)
    for (int i = 0; i < 8 && i < strlen(error_code_str); i++) {
        responsePayload[21 + i] = static_cast<uint8_t>(error_code_str[i]);
    }
    
    // 응답 전송 (크기를 28바이트로 변경)
    sendData(senderId, my_id_, CMD_STATUS_SYNC_ACK, responsePayload, 29);
}

void Com_Protocol::handleIdScan(uint16_t senderId, uint8_t* payload, size_t length){
    // 페이로드 유효성 검사 (1바이트 ID만 필요)
    if (length < 1 || !payload) {
        return;
    }

    const uint16_t SCAN_ID = (payload[0] << 8) | payload[1];     // 스캔 요청된 ID (2바이트)
    
    // ID 매칭 여부 확인 및 응답 전송
    if (SCAN_ID == my_id_) {
        const uint16_t response = my_id_;      // 2바이트 응답 데이터
        sendData(senderId, my_id_, CMD_ID_SCAN_ACK, (uint8_t *)&response, 2);
    }    
}


void Com_Protocol::handleMainPowerControl(uint16_t senderId, uint8_t* payload, size_t length){
    // 페이로드 길이 체크
    if (length < 1) return;

    uint8_t powerFlag = payload[0];

    // 플래그 값 검증
    if (powerFlag > 1) return;  // 0 또는 1만 유효

    // 메인파워 제어 로직 구현
    // - powerFlag가 0이면 메인파워 OFF
    // - powerFlag가 1이면 메인파워 ON
    // - 하드웨어 제어 인터페이스를 통해 실제 전원 제어 수행
    
    // 제어 결과에 대한 응답 전송
    // - 제어 성공/실패 여부를 포함한 응답 패킷 구성
    // - sendData() 함수를 사용하여 응답 전송

    setMainPower(powerFlag);

    /* 응답 구현 */
    uint8_t ackPayload[1];
    ackPayload[0] = powerFlag;

    sendData(senderId, my_id_, CMD_MAIN_POWER_CONTROL_ACK, ackPayload, 1);

}

void Com_Protocol::setMainPower(uint8_t powerFlag){
    if (powerFlag == 1){
        // 메인파워 ON
    } else {
        // 메인파워 OFF
    }
}

void Com_Protocol::handlePlayControl(uint16_t senderId, uint8_t* payload, size_t length) {
    // 페이로드 길이 체크
    if (length < 1) return;
    // todo : 각 switch 내부 처리 추가, currentPlayState_ 에 상태 반환환
    // 페이로드에서 PlayControl 상태 추출
    PlayControlState playState = static_cast<PlayControlState>(payload[0]);    
    // 상태에 따른 처리
    switch (playState) {
        case PlayControlState::PLAY_ONE:
            // 1회 재생
            break;
            
        case PlayControlState::PLAY_REPEAT:
            // 반복 재생
            break;
            
        case PlayControlState::PAUSE:
            // 일시정지
            break;            

        case PlayControlState::STOP:
            // 정지
            break;

        default:
            // 잘못된 상태값 수신
            return;
    }
    
    // 응답 전송
    uint8_t response[1];
    uint8_t currentPlayState_ = 0;

    response[0] = static_cast<uint8_t>(currentPlayState_);    
    
    sendData(senderId, my_id_, CMD_PLAY_CONTROL_ACK, response, 1);
}

void Com_Protocol::handleJogMoveCwCcw(uint16_t senderId, uint8_t* payload, size_t length){
    // 페이로드 길이 체크
    if (length < 7) return;  // 최소 7바이트 필요 (id, sub-id, speed[4], direction)

    uint8_t id = payload[0];
    uint8_t subId = payload[1];
    
    // speed 값 추출 (4바이트)
    uint32_t speed = 0;
    speed |= static_cast<uint32_t>(payload[2]) << 24;
    speed |= static_cast<uint32_t>(payload[3]) << 16;
    speed |= static_cast<uint32_t>(payload[4]) << 8;
    speed |= static_cast<uint32_t>(payload[5]);
    
    uint8_t direction = payload[6];
    // 방향 값 검증
    if (direction > 1) return;  // 0(CCW) 또는 1(CW)만 유효

    /* 조그 이동 로직 사용자 재정의 */
    setJogMoveCwCcw(id, subId, speed, direction);

    // 응답 없음
    /*
    uint8_t response[7];
    response[0] = id;
    response[1] = subId;
    response[2] = (speed >> 24) & 0xFF;
    response[3] = (speed >> 16) & 0xFF;
    response[4] = (speed >> 8) & 0xFF;
    response[5] = speed & 0xFF;
    response[6] = direction;
    
    sendData(senderId, my_id_, CMD_JOG_MOVE_CW_CCW_ACK, response, 7);
    */
}

void Com_Protocol::setJogMoveCwCcw(uint8_t id, uint8_t subId, uint32_t speed, uint8_t direction){
    // 조그 이동 로직 구현
    // - direction이 0이면 반시계 방향(CCW) 이동
    // - direction이 1이면 시계 방향(CW) 이동
    
    // TODO: 실제 모터 제어 로직 구현
    // 여기에 id, subId, speed, direction을 사용하여 모터 제어 로직 추가
}





void Com_Protocol::resetFileTransferContext() {
    memset(&fileContext_, 0, sizeof(FileTransferContext));
    fileContext_.isTransferring = false;
    fileContext_.retryCount = 0;
}

void Com_Protocol::handleFileReceive(uint16_t senderId, uint8_t* payload, size_t length) {
    if (length < 1) return;
    
    FileTransferStage stage = static_cast<FileTransferStage>(payload[0]);
    
    switch (stage) {
        case FileTransferStage::REQUEST_RECEIVE: {
            // 파일 수신 요청 처리
            if (length < sizeof(uint32_t) + 1) return;  // 최소 크기 체크
            
            uint32_t fileSize = *reinterpret_cast<uint32_t*>(payload + 1);
            if (fileSize > MAX_FILE_SIZE) {
                sendFileReceiveAck(senderId, stage, false);
                return;
            }
            
            fileContext_.fileSize = fileSize;
            fileContext_.isTransferring = true;
            fileContext_.receivedSize = 0;
            fileContext_.currentIndex = 0;
            
            sendFileReceiveAck(senderId, stage, true);
            break;
        }
        
        case FileTransferStage::RECEIVING_DATA: {
            if (!fileContext_.isTransferring) {
                sendFileReceiveAck(senderId, stage, false);
                return;
            }
            
            uint32_t blockIndex = *reinterpret_cast<uint32_t*>(payload + 1);
            if (blockIndex != fileContext_.currentIndex) {
                sendFileReceiveAck(senderId, stage, false);
                return;
            }
            
            // 데이터 처리
            size_t dataSize = length - 5;  // stage(1) + blockIndex(4)
            fileContext_.receivedSize += dataSize;
            fileContext_.currentIndex++;
            
            // 체섬 업데이트
            fileContext_.checksum = calculateCRC16(payload + 5, dataSize);
            
            sendFileReceiveAck(senderId, stage, true, blockIndex);
            break;
        }
        
        case FileTransferStage::VERIFY_CHECKSUM: {
            if (!fileContext_.isTransferring) {
                sendFileReceiveAck(senderId, stage, false);
                return;
            }
            
            uint16_t receivedChecksum = *reinterpret_cast<uint16_t*>(payload + 1);
            bool checksumMatch = (receivedChecksum == fileContext_.checksum);
            
            sendFileReceiveAck(senderId, stage, checksumMatch);
            
            if (checksumMatch) {
                // 파일 수신 완료
                resetFileTransferContext();
            }
            break;
        }
    }
}

void Com_Protocol::sendFileReceiveAck(uint16_t receiverId, FileTransferStage stage, 
                                    bool success, uint32_t data) {
    uint8_t response[6];
    response[0] = static_cast<uint8_t>(stage);
    response[1] = success ? 1 : 0;
    
    if (data != 0) {
        *reinterpret_cast<uint32_t*>(response + 2) = data;
        sendData(receiverId, my_id_, CMD_FILE_RECEIVE_ACK, response, 6);
    } else {
        sendData(receiverId, my_id_, CMD_FILE_RECEIVE_ACK, response, 2);
    }
}

// ping 요청 함수 구현
void Com_Protocol::sendPing(uint16_t targetId) {
    uint8_t pingPayload[] = "PING";
    sendData(targetId, my_id_, CMD_PING, pingPayload, 4);
    //sendData(1, 2, CMD_PING, pingPayload, 4);
}

void Com_Protocol::sendIdScan(uint16_t targetId){
    uint8_t idScanPayload[1];
    idScanPayload[0] = my_id_;  // 현재 장치의 ID (1바이트로 간주)
    sendData(targetId, my_id_, CMD_ID_SCAN, idScanPayload, 1);
}

// 새로운 동기화 함수 추가
void Com_Protocol::sendSync() {
    uint8_t syncPayload[6];
    uint32_t timestamp = tick_->getTickCount();
    syncPayload[0] = static_cast<uint8_t>((timestamp >> 24) & 0xFF);
    syncPayload[1] = static_cast<uint8_t>((timestamp >> 16) & 0xFF);
    syncPayload[2] = static_cast<uint8_t>((timestamp >> 8) & 0xFF);
    syncPayload[3] = static_cast<uint8_t>(timestamp & 0xFF);
    uint16_t authToken = 0xABCD;
    syncPayload[4] = static_cast<uint8_t>(authToken >> 8);
    syncPayload[5] = static_cast<uint8_t>(authToken & 0xFF);
    
    sendData(0xFFFF, my_id_, CMD_SYNC, syncPayload, 6);
}

// sendSyncAck 함수 추가
void Com_Protocol::sendSyncAck(uint16_t targetId, uint32_t timestamp) {
    uint8_t syncAckPayload[6];
    syncAckPayload[0] = static_cast<uint8_t>((timestamp >> 24) & 0xFF);
    syncAckPayload[1] = static_cast<uint8_t>((timestamp >> 16) & 0xFF);
    syncAckPayload[2] = static_cast<uint8_t>((timestamp >> 8) & 0xFF);
    syncAckPayload[3] = static_cast<uint8_t>(timestamp & 0xFF);
    uint16_t authToken = 0xABCD;
    syncAckPayload[4] = static_cast<uint8_t>(authToken >> 8);
    syncAckPayload[5] = static_cast<uint8_t>(authToken & 0xFF);
    
    sendData(targetId, my_id_, CMD_SYNC_ACK, syncAckPayload, 6);
}


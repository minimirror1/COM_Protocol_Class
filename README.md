# COM Protocol Class

통신 프로토콜을 구현한 C++ 클래스입니다.

## 의존성

이 클래스는 다음 라이브러리들에 의존성이 있습니다:

- [cpp_Uart_Class_v2](https://github.com/minimirror1/cpp_Uart_Class_v2): UART 통신을 위한 클래스 라이브러리
- [cpp_tick](https://github.com/minimirror1/cpp_tick): 타이머 기능을 위한 클래스 라이브러리

## 초기화 방법

플랫폼에 따라 적절한 시리얼 인터페이스와 타이머 구현체를 선택하여 초기화합니다.

### Qt 환경에서의 초기화

```cpp
#ifdef QT_PLATFORM
int main() {
    QtSerialImpl serial("COM1", 115200);
    QtTickImpl tick;
  
    Com_Protocol protocol(&serial, &tick, 0x0001);
    // ... 이하 동일한 코드
}
#endif
```

### STM32 환경에서의 사용

```cpp
STM32SerialImpl serial(&huart1, USART1_IRQn); /*****필수*****/ // 추상 인터페이스 초기화, 구조체와 인터럽트 IRQ 전달
//HAL Driver Callback 호출을 위해 전역 객체로 선언

Tick delay1;//테스트 코드

int main(void)
{
    serial.init();/*****필수*****/

    // Option. 송수신 표시용 LED
    serial.init_txLed(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    serial.init_rxLed(LD7_GPIO_Port, LD7_Pin, GPIO_PIN_SET);

    STM32TickImpl tick; /*****필수*****/ //타이머 인스턴스 초기화
    Com_Protocol protocol(&serial, &tick, 0x0001); /*****필수*****///시리얼 인터페이스, tick 인스턴스, 장치 ID 전달

    while(1){
        // 테스트 코드
        serial.loop();
        if(delay1.delay(1000)){
            protocol.sendPing(10);
        }

        protocol.processReceivedData();/*****필수*****/
    }
}

/*****필수*****/
/* HAL Driver Callback */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    serial.TxCpltCallback(huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    serial.RxCpltCallback(huart);
}
```

## 주요 기능

- UART 기반의 패킷 통신 프로토콜 구현
- 시작 시퀀스, 길이, ID, 명령어, 페이로드, CRC 체크섬을 포함한 패킷 구조
- 데이터 송수신 및 패킷 처리 기능
- CRC16 XMODEM 체크섬 검증

## 패킷 구조

| 필드        | 크기 (바이트) | 설명                                                          |
| ----------- | ------------- | ------------------------------------------------------------- |
| 시작 시퀀스 | 4             | 패킷의 시작을 나타내는 마커 (0x16)                            |
| 길이        | 2             | 전체 패킷의 길이 (수신자ID + 송신자ID + CMD + 페이로드 + CRC) |
| 수신자 ID   | 2             | 수신 장치의 고유 식별자                                       |
| 송신자 ID   | 2             | 송신 장치의 고유 식별자                                       |
| CMD         | 2             | 명령어 코드                                                   |
| 시퀀스 번호 | 2             | 패킷 순서를 식별하는 번호                                     |
| 페이로드    | 가변          | 실제 전송할 데이터                                            |
| CRC         | 2             | CRC16 XMODEM 체크섬                                           |

## 명령어 코드

| 명령어                    | 코드   | 설명                                    |
|--------------------------|--------|-----------------------------------------|
| CMD_PING                 | 0x0001 | 연결 상태 확인용 PING 요청              |
| CMD_PONG                 | 0x8001 | PING에 대한 응답                        |
| CMD_FILE_RECEIVE         | 0x0002 | 파일 수신 요청                          |
| CMD_FILE_RECEIVE_ACK     | 0x8002 | 파일 수신 요청에 대한 응답              |
| CMD_CONFIG               | 0x0003 | 설정 데이터 전송                        |
| CMD_ID_SCAN              | 0x0004 | 장치 ID 스캔 요청                       |
| CMD_ID_SCAN_ACK          | 0x8004 | ID 스캔 요청에 대한 응답                |
| CMD_STATUS_SYNC          | 0x0010 | 상태 동기화 요청                        |
| CMD_STATUS_SYNC_ACK      | 0x8010 | 상태 동기화 요청에 대한 응답            |
| CMD_SYNC                 | 0x0020 | 시퀀스 동기화 요청                      |
| CMD_SYNC_ACK             | 0x8020 | 시퀀스 동기화 요청에 대한 응답          |
| CMD_MAIN_POWER_CONTROL   | 0x0100 | 메인 전원 제어 요청                     |
| CMD_MAIN_POWER_CONTROL_ACK| 0x8100 | 메인 전원 제어 요청에 대한 응답         |
| CMD_PLAY_CONTROL         | 0x0110 | 재생 제어 요청                          |
| CMD_PLAY_CONTROL_ACK     | 0x8110 | 재생 제어 요청에 대한 응답              |
| CMD_JOG_MOVE_CW_CCW      | 0x0120 | 조그 이동 제어 요청                     |
| CMD_JOG_MOVE_CW_CCW_ACK  | 0x8120 | 조그 이동 제어 요청에 대한 응답         |

* 참고: ACK 명령어는 원본 명령어 코드에 0x8000을 더한 값입니다.

## 주요 클래스 메서드

### 초기화 및 기본 동작

- `Com_Protocol(ISerialInterface* serial, ITick* tick, uint16_t my_id)`: 생성자를 통한 초기화
- `sendData()`: 데이터 패킷 전송
- `receiveData()`: 데이터 수신
- `processReceivedData()`: 수신된 데이터 처리

### 패킷 처리

- `handlePing()`: PING 명령어 처리
- `handleData()`: 데이터 명령어 처리
- `handleConfig()`: 설정 명령어 처리
- `handleUnknownCommand()`: 알 수 없는 명령어 처리

### 오류 검증

- `calculateCRC16()`: CRC16 XMODEM 체크섬 계산
- [CRC Calculator](https://crccalc.com/?crc=&method=CRC-16&datatype=0&outtype=0)
- [CRC16 XMODEM Table](https://crccalc.com/?crc=&method=CRC-16/XMODEM&datatype=0&outtype=0)
- 패킷 타임아웃 처리 (100ms)
- 패킷 길이 검증

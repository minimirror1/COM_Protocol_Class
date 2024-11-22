# COM Protocol Class

통신 프로토콜을 구현한 C++ 클래스입니다.

## 주요 기능

- UART 기반의 패킷 통신 프로토콜 구현
- 시작 시퀀스, 길이, ID, 명령어, 페이로드, CRC 체크섬을 포함한 패킷 구조
- 데이터 송수신 및 패킷 처리 기능
- CRC16 XMODEM 체크섬 검증

## 패킷 구조
| 필드 | 크기 (바이트) | 설명 |
|------|--------------|------|
| 시작 시퀀스 | 4 | 패킷의 시작을 나타내는 마커 (0x16) |
| 길이 | 2 | 전체 패킷의 길이 (수신자ID + 송신자ID + CMD + 페이로드 + CRC) |
| 수신자 ID | 2 | 수신 장치의 고유 식별자 |
| 송신자 ID | 2 | 송신 장치의 고유 식별자 |
| CMD | 2 | 명령어 코드 |
| 페이로드 | 가변 | 실제 전송할 데이터 |
| CRC | 2 | CRC16 XMODEM 체크섬 |

## 명령어 코드
| 명령어 | 코드 | 설명 |
|--------|------|------|
| CMD_PING | 0x0001 | 연결 상태 확인용 PING 요청 |
| CMD_PONG | 0x0011 | PING에 대한 응답 |
| CMD_DATA | 0x0002 | 일반 데이터 전송 |
| CMD_CONFIG | 0x0003 | 설정 데이터 전송 |

## 주요 클래스 메서드

### 초기화 및 기본 동작
- `initialize()`: UART 및 타이머 초기화
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
- 패킷 타임아웃 처리 (100ms)
- 패킷 길이 검증

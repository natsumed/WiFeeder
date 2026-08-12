# WiFeeder v2 — Software Architecture

> **MCU (today)**: Bare-metal superloop (`wifeeder_init` / `wifeeder_poll`) — FreeRTOS vendored but **not linked** yet  
> **Motor PWM**: PCA9685 over bit-bang I2C (PB6/PB7)  
> **Host**: C++17 + CMake · diet/DB/MQTT/NRF  

---

## 1. STM32 Firmware Architecture

### 1.0 Current runtime (MVP)

```mermaid
flowchart LR
  main["main.c"] --> init["wifeeder_init"]
  main --> loop["wifeeder_poll"]
  init --> pca["pca9685 + motor"]
  init --> enc["encoder TIM2"]
  init --> nrf["nrf24l01"]
  loop --> status["STATUS"]
  loop --> feed["FEED_CMD / FEEDING"]
```

FreeRTOS task split below is the **target** model, not what `CMakeLists.txt` builds today.

### 1.1 Task Hierarchy (planned)

```mermaid
graph TB
    subgraph Kernel["FreeRTOS Kernel"]
        direction TB
        S["Scheduler<br/>(Preemptive)"]
    end

    subgraph Tasks["Application Tasks"]
        T1["CmdTask<br/>Prio: BelowNormal<br/>Stack: 512 words<br/>UART CLI + Debug"]
        T2["WifeederTask<br/>Prio: Normal<br/>Stack: 1024 words<br/>Main Control Loop"]
        T3["RadioTask<br/>Prio: AboveNormal<br/>Stack: 512 words<br/>NRF24L01+ TX/RX"]
        T4["IwdgTask<br/>Prio: Low<br/>Stack: 256 words<br/>Watchdog Refresh"]
    end

    S --> T1
    S --> T2
    S --> T3
    S --> T4

    T2 --> |"Queues"| T3
    T3 --> |"Queues"| T2
    T1 --> |"Commands"| T2
```

### 1.2 Module Dependency Graph

```mermaid
graph TB
    subgraph App["Application Layer"]
        MAIN["main.c<br/>Init + Task Creation"]
        WIFEEDER["wifeeder.c<br/>Main Loop + State Machine"]
        CMD["cmd.c<br/>CLI Command Handler"]
    end

    subgraph Drivers["Driver Layer"]
        NRF["nrf24l01.c<br/>SPI + NRF Protocol"]
        RFID["rfid.c<br/>UART2 + Tag Parsing"]
        MOTOR["motor.c<br/>PWM + IBT-2 Control"]
        ENCODER["encoder.c<br/>TIM2/3 Quadrature"]
        HX711_DRV["hx711.c<br/>Load Cell Reading"]
        FLASH["flash.c<br/>Config Storage"]
    end

    subgraph HAL["HAL Layer"]
        SPI["STM32 HAL SPI"]
        UART["STM32 HAL UART"]
        TIM["STM32 HAL TIM"]
        GPIO["STM32 HAL GPIO"]
    end

    subgraph Common["Common Libraries"]
        CRC["crc8.c<br/>CRC-8 Checksum"]
        ERROR["error.h<br/>Error Codes"]
        UTIL["utilities.c<br/>String Conversion"]
    end

    WIFEEDER --> NRF
    WIFEEDER --> RFID
    WIFEEDER --> MOTOR
    WIFEEDER --> ENCODER
    WIFEEDER --> HX711_DRV
    WIFEEDER --> FLASH

    NRF --> SPI
    NRF --> GPIO
    RFID --> UART
    MOTOR --> TIM
    MOTOR --> GPIO
    ENCODER --> TIM
    HX711_DRV --> GPIO

    WIFEEDER --> CRC
    WIFEEDER --> ERROR
    WIFEEDER --> UTIL
    NRF --> CRC
```

### 1.3 NRF24L01+ Driver API

```mermaid
classDiagram
    class NRF24L01 {
        +nrf24_init() T_eError_eErrorType
        +nrf24_send(uint8_t* data, size_t len) T_eError_eErrorType
        +nrf24_receive(uint8_t* data, size_t* len) T_eError_eErrorType
        +nrf24_is_data_available() bool
        +nrf24_set_channel(uint8_t ch) void
        +nrf24_set_power(uint8_t power) void
        -spi_transfer(uint8_t cmd) uint8_t
        -spi_write_register(uint8_t reg, uint8_t val) void
        -spi_read_register(uint8_t reg) uint8_t
        -flush_tx() void
        -flush_rx() void
    }

    class Protocol {
        +protocol_send_status(uint32_t heartbeat) T_eError_eErrorType
        +protocol_send_rfid(uint32_t tag_id) T_eError_eErrorType
        +protocol_send_feed_done(uint32_t animal_id, uint16_t m1, uint16_t m2) T_eError_eErrorType
        +protocol_send_weight(uint32_t animal_id, uint32_t grams) T_eError_eErrorType
        +protocol_receive(msg_t* msg) T_eError_eErrorType
        -pack(msg_t* msg, uint8_t* buf) void
        -unpack(uint8_t* buf, msg_t* msg) T_eError_eErrorType
        -crc8(uint8_t* data, size_t len) uint8_t
    }

    Protocol --> NRF24L01 : uses
```

### 1.4 Timer Allocations

> **L432KC note:** This MCU has **no TIM3 / TIM4 / TIM8**. Use TIM1 + TIM16 for PWM and TIM2 for Enc1. Enc2 is soft EXTI (PA11/PA12). See [`docs/wifeeder-v2-final-plan.md`](docs/wifeeder-v2-final-plan.md) §2.

| Timer | Channel | Function | Frequency |
|-------|---------|----------|-----------|
| TIM1 | CH1 | PWM Motor 1 (PA8) | 10 kHz |
| TIM16 | CH1 | PWM Motor 2 (PB6) | 10 kHz |
| TIM2 | CH1, CH2 | Encoder 1 (PA0/PA1) | 400 PPR × 4 = 1600 cnt/rev |
| SysTick | - | FreeRTOS tick / HAL tick | 1 kHz |

### 1.5 Memory Map

```
FLASH (256 KB):
  0x0800_0000 - 0x0800_7FFF : Bootloader (32 KB)
  0x0800_8000 - 0x0800_FFFF : Application (32 KB)
  0x0801_0000 - 0x0801_7FFF : Config Pages (32 KB)
  0x0801_8000 - 0x0803_FFFF : Reserved (160 KB)

RAM (64 KB):
  0x2000_0000 - 0x2000_0FFF : Stack (4 KB)
  0x2000_1000 - 0x2000_7FFF : Heap + FreeRTOS (28 KB)
  0x2000_8000 - 0x2000_FFFF : Static Data + BSS (32 KB)
```

---

## 2. Raspberry Pi Software Architecture

### 2.1 Thread Model

```mermaid
graph TB
    subgraph Main["Main Process (wcontroller)"]
        direction TB

        subgraph Threads["Threads"]
            T1["NRF Thread<br/>Radio RX/TX<br/>Prio: High"]
            T2["Diet Thread<br/>Feed Calculation<br/>Prio: Normal"]
            T3["MQTT Thread<br/>Cloud Sync<br/>Prio: Normal"]
            T4["Web Thread<br/>Dashboard<br/>Prio: Low"]
        end

        subgraph Shared["Shared Resources"]
            DB["SQLite DB<br/>(Mutex Protected)"]
            NRF_QUEUE["NRF TX Queue<br/>(Thread-Safe)"]
            EVENT_QUEUE["Event Queue<br/>(Thread-Safe)"]
        end
    end

    T1 -->|"RX events"| EVENT_QUEUE
    EVENT_QUEUE -->|"Process"| T2
    T2 -->|"FEED_CMD"| NRF_QUEUE
    NRF_QUEUE -->|"TX"| T1
    T2 --> DB
    T3 --> DB
    T3 -->|"Config"| NRF_QUEUE
    T4 --> DB

    style T1 fill:#fff3e0
    style T2 fill:#e8f5e9
    style T3 fill:#e8eaf6
    style T4 fill:#f3e5f5
```

### 2.2 Class Diagram

```mermaid
classDiagram
    class Nrf24l01 {
        -int spi_fd
        -int ce_pin
        -int irq_pin
        +init() bool
        +send(uint8_t* data, size_t len) bool
        +receive(uint8_t* data, size_t* len) bool
        +dataAvailable() bool
        -spiTransfer(uint8_t* tx, uint8_t* rx, size_t len) void
    }

    class Protocol {
        +sendStatus(uint32_t heartbeat) bool
        +sendRfid(uint32_t tagId) bool
        +sendFeedCmd(FeedCmd cmd) bool
        +receive(Message* msg) bool
        -pack(Message* msg, uint8_t* buf) void
        -unpack(uint8_t* buf, Message* msg) bool
        -crc8(uint8_t* data, size_t len) uint8_t
    }

    class DietEngine {
        +calcPortion(uint32_t rfid) FeedCmd
        +updateConsumption(uint32_t rfid, uint16_t actual)
        +getDailyStatus(uint32_t rfid) DietStatus
        -getTargetDailyQty(uint32_t rfid) double
        -getCurrentConsumption(uint32_t rfid) double
        -getRemainingPortions(uint32_t rfid) uint32_t
    }

    class Database {
        -sqlite3* db
        -std::mutex mtx
        +open() bool
        +getAnimal(uint32_t rfid) Animal
        +updateConsumption(uint32_t rfid, uint16_t qty) bool
        +getWeight(uint32_t rfid) double
        +storeWeight(uint32_t rfid, double weight) bool
        +getAllActiveRfids() vector~uint32_t~
    }

    class MqttClient {
        -mosquitto* mqtt
        +connect() bool
        +publishFeed(uint32_t rfid, uint16_t actual) bool
        +publishWeight(uint32_t rfid, double grams) bool
        +subscribe(string topic) bool
        -onMessage(string topic, string payload) void
    }

    Protocol --> Nrf24l01 : uses
    DietEngine --> Database : queries
    DietEngine --> Protocol : sends FEED_CMD
    MqttClient --> Database : reads/writes
```

### 2.3 Event Processing Pipeline

```mermaid
sequenceDiagram
    participant NRF_Thread as NRF Thread
    participant EventQ as Event Queue
    participant Diet_Thread as Diet Thread
    participant DB as Database
    participant MQTT_Thread as MQTT Thread
    participant Cloud as Agpro Cloud

    NRF_Thread->>NRF_Thread: Packet received
    NRF_Thread->>NRF_Thread: Parse + validate CRC
    NRF_Thread->>EventQ: Push(event)
    EventQ->>Diet_Thread: Pop(event)

    alt RFID_TAG event
        Diet_Thread->>DB: Lookup animal (RFID)
        DB->>Diet_Thread: Diet data
        Diet_Thread->>Diet_Thread: Calc portion
        Diet_Thread->>NRF_Thread: Send FEED_CMD
    else FEED_DONE event
        Diet_Thread->>DB: Update consumption
        DB->>Diet_Thread: OK
        Diet_Thread->>MQTT_Thread: Publish feed event
        MQTT_Thread->>Cloud: MQTT message
    else WEIGHT event
        Diet_Thread->>DB: Store weight
        Diet_Thread->>MQTT_Thread: Publish weight
        MQTT_Thread->>Cloud: MQTT message
    end
```

### 2.4 Database Schema

```mermaid
erDiagram
    ANIMALS {
        uint32 rfid PK
        string name
        uint32 diet_id FK
        double weight_kg
        uint64 weight_ts
    }

    DIETS {
        uint32 diet_id PK
        uint8 feed_id
        string start_time
        string end_time
        uint32 inter_seconds
        uint32 d_max_grams
        uint32 d_qty_grams
        uint32 p_max_grams
        uint32 density
    }

    CONSUMPTION {
        uint32 id PK
        uint32 rfid FK
        uint32 diet_id FK
        uint32 feed_id FK
        double day_weight
        double day_left
        uint32 day_ports
        string date
    }

    FEED_HISTORY {
        uint32 id PK
        uint32 rfid FK
        uint16 motor1_actual
        uint16 motor2_actual
        uint64 timestamp
    }

    WEIGHTS {
        uint32 id PK
        uint32 rfid FK
        double weight_g
        uint64 timestamp
    }

    ACTUATORS {
        uint8 station_id PK
        uint8 motor1_status
        uint8 motor2_status
        uint8 rfid_status
        uint8 hx711_status
        uint64 last_seen
    }

    ANIMALS ||--o{ CONSUMPTION : has
    ANIMALS ||--o{ FEED_HISTORY : has
    ANIMALS ||--o{ WEIGHTS : has
    DIETS ||--o{ ANIMALS : assigned_to
    DIETS ||--o{ CONSUMPTION : tracked_by
    ACTUATORS ||--o{ FEED_HISTORY : produced_by
```

### 2.5 Config File Format (`config.json`)

```json
{
  "station": {
    "sn": "WCN-A100-0001X",
    "device_id": 0,
    "name": "Station 1"
  },
  "nrf24": {
    "spi_device": "/dev/spidev0.0",
    "ce_pin": 25,
    "irq_pin": 24,
    "channel": 76,
    "data_rate": "1Mbps",
    "tx_power_dbm": 0
  },
  "mqtt": {
    "broker": "localhost",
    "port": 1883,
    "username": "client2",
    "password": "wifeeder@agTEK.2020_2_mqtt",
    "keep_alive": 60,
    "qos": 2,
    "topic_prefix": "WCN-A100-0001X"
  },
  "database": {
    "path": "/etc/wifeeder-v2/wifeeder.db"
  },
  "feeding": {
    "algorithm": "linear_dynamic",
    "calibration_tag": 0,
    "intervals": {
      "status_ok": 5,
      "status_disconnected": 1
    }
  }
}
```

---

## 3. Build System

### 3.1 STM32 (CubeIDE / CMake)

```cmake
# stm32/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(wifeeder_v2_mcu C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(wifeeder_mcu
    Core/Src/main.c
    Core/Src/nrf24l01.c
    Core/Src/protocol.c
    Core/Src/rfid.c
    Core/Src/motor.c
    Core/Src/encoder.c
    Core/Src/hx711.c
    Core/Src/flash.c
    Core/Src/cmd.c
    Core/Src/stm32l4xx_it.c
    startup/startup_stm32l432xx.s
)

target_link_libraries(wifeeder_mcu
    STM32_HAL
    FreeRTOS
    CMSIS
    Common
)
```

### 3.2 Raspberry Pi (CMake)

```cmake
# raspberry/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(wifeeder_v2_host CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(wifeeder_host
    src/main.cpp
    src/nrf24l01.cpp
    src/protocol.cpp
    src/diet_engine.cpp
    src/database.cpp
    src/mqtt_client.cpp
)

target_link_libraries(wifeeder_host
    PRIVATE PkgConfig::MOSQUITTO
    PRIVATE PkgConfig::SQLITE3
    PRIVATE pthread
    PRIVATE wiringPi
)
```

---

## 4. Key Data Structures

### 4.1 Message (Both Platforms)

```c
typedef struct __attribute__((packed)) {
    uint8_t  dest_id;
    uint8_t  src_id;
    uint8_t  msg_type;
    uint8_t  seq_num;
    union {
        struct { uint32_t heartbeat; uint8_t sensors; uint8_t battery; } status;
        struct { uint32_t tag_id;   uint32_t timestamp;               } rfid_tag;
        struct { uint32_t animal_id; uint16_t motor1_revs; uint16_t motor2_revs; } feed_cmd;
        struct { uint32_t animal_id; uint16_t motor1_actual; uint16_t motor2_actual; } feed_done;
        struct { uint32_t animal_id; uint32_t weight_g;               } weight;
        struct { uint8_t  param_id;  uint32_t value;                   } config;
        struct { uint8_t  echoed_type; uint8_t status;                 } ack;
        struct { uint8_t  error_code; uint32_t details;                } error;
    } payload;
    uint8_t crc;
} T_stWifeederV2_iMessage;
```

### 4.2 NRF Stats (Debugging)

```c
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t acks_received;
    uint32_t retries;
    uint32_t crc_errors;
    uint32_t max_rt_errors;
    uint32_t sequence_gaps;
    int8_t   last_rssi;
    uint32_t uptime_ms;
} T_stNrf_iStats;
```

---

## 5. Error Handling Strategy

```mermaid
stateDiagram-v2
    [*] --> NORMAL: System booted

    NORMAL --> WARNING: Connectivity issues
    NORMAL --> ERROR: Hardware fault

    WARNING --> NORMAL: Issue resolved
    WARNING --> ERROR: Issue persists > threshold

    ERROR --> RECOVERY: Attempt auto-recovery
    RECOVERY --> NORMAL: Recovery successful
    RECOVERY --> FATAL: Recovery failed 3x

    FATAL --> [*]: System reset / maintenance
```

### Error Response Matrix

| Error | STM32 Action | RPi Action |
|-------|-------------|-----------|
| NRF TX fail (1x) | Retry app-level | Wait for next STATUS |
| NRF TX fail (3x) | Mark disconnected, 1s heartbeat | Log, alert MQTT |
| NRF TX fail (10x) | Increment fail counter | Alert MQTT, mark offline |
| RFID read fail | Retry 5x, then ignore | N/A |
| Motor stall | Stop motor, send ERROR | Log, alert MQTT |
| HX711 timeout | Send ERROR, retry next cycle | Log |
| Watchdog timeout | System reset | Log reset event |

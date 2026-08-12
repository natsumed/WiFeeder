# WiFeeder v2 — Architecture Overview

> **Version**: 2.0.0  
> **Communication**: NRF24L01+ 2.4 GHz Wireless  
> **MCU**: STM32 NUCLEO-L432KC  
> **Host**: Raspberry Pi 3B+/4  

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Component Architecture](#component-architecture)
3. [Data Flow](#data-flow)
4. [Network Topology](#network-topology)
5. [Power Architecture](#power-architecture)
6. [State Machines](#state-machines)
7. [Directory Structure](#directory-structure)

---

## System Overview

```mermaid
graph TB
    subgraph Cloud[" Agpro Cloud"]
        MQTT_BROKER["MQTT Broker<br/>Cloud Backend"]
        WEB_DASH["Web Dashboard<br/>Farmer Interface"]
    end

    subgraph RPi[" Raspberry Pi Controller"]
        MQTT_CLIENT["MQTT Client<br/>(Mosquitto)"]
        DIET_ENGINE["Diet Engine<br/>(C++20)"]
        DB["SQLite<br/>Database"]
        WEB_SRV["Web Server<br/>(Optional)"]
        NRF_RPI["NRF24L01+<br/>SPI0 Module"]

        MQTT_CLIENT <--> |"JSON/TLS"| MQTT_BROKER
        MQTT_CLIENT --> DIET_ENGINE
        DIET_ENGINE --> DB
        DIET_ENGINE --> NRF_RPI
        WEB_SRV --> DB
    end

    subgraph STM32[" NUCLEO-L432KC Actuator"]
        NRF_STM32["NRF24L01+<br/>SPI1 Module"]
        MAIN_LOOP["Main Control<br/>Loop"]
        RFID["RFID Reader<br/>125kHz/EM"]
        MOTOR["IBT-2 Motor<br/>Driver"]
        ENCODER["Quadrature<br/>Encoders"]
        HX711["HX711 Load<br/>Cell (Opt.)"]
        CLI["Debug CLI<br/>UART2"]

        NRF_STM32 --> MAIN_LOOP
        MAIN_LOOP --> RFID
        MAIN_LOOP --> MOTOR
        MAIN_LOOP --> ENCODER
        MAIN_LOOP --> HX711
        MAIN_LOOP --> CLI
    end

    NRF_RPI <-.-> |"2.4GHz"| NRF_STM32
    POWER[" 12V Battery<br/>+ Mini360 Buck"] --> STM32

    style Cloud fill:#e1f5fe,stroke:#01579b
    style RPi fill:#e8f5e9,stroke:#1b5e20
    style STM32 fill:#fff3e0,stroke:#e65100
    style POWER fill:#fce4ec,stroke:#880e4f
```

---

## Component Architecture

```mermaid
graph LR
    subgraph Pi["Raspberry Pi"]
        direction TB
        A1["NRF24L01+ Driver<br/>(C++)"] --> A2["Protocol Layer<br/>(msg pack/unpack)"]
        A2 --> A3["Message Router"]
        A3 --> A4["Diet Engine"]
        A3 --> A5["MQTT Client"]
        A4 --> A6["SQLite DB"]
        A5 --> A7["Mosquitto"]
    end

    subgraph STM32["NUCLEO-L432KC"]
        direction TB
        B1["NRF24L01+ Driver<br/>(C)"]
        B1 --> B2["Protocol Layer<br/>(msg pack/unpack)"]
        B2 --> B3["Main Task<br/>(FreeRTOS)"]
        B3 --> B4["RFID Task"]
        B3 --> B5["Motor Task"]
        B3 --> B6["Sensor Task"]
        B3 --> B7["Watchdog Task"]
        B4 --> B8["UART2"]
        B5 --> B9["PWM + GPIO"]
        B6 --> B10["HX711 SPI"]
    end

    A1 <-.-> |"2.4 GHz"| B1
```

---

## Data Flow

### End-to-End Feeding Cycle

```mermaid
sequenceDiagram
    participant Cow as  Cow
    participant RFID as RFID Reader
    participant STM32 as STM32 MCU
    participant NRF_S as NRF24L01 (STM)
    participant NRF_P as NRF24L01 (RPi)
    participant RPi as Raspberry Pi
    participant DB as SQLite DB
    participant MQTT as Agpro Cloud

    Cow->>RFID: Approaches feeder
    RFID->>STM32: Tag detected (125kHz)
    STM32->>NRF_S: Send RFID_TAG (msg ID=0x02)
    NRF_S->>NRF_P: 2.4GHz packet
    NRF_P->>RPi: RFID_TAG received
    RPi->>DB: Lookup animal (RFID)
    DB->>RPi: Diet data found
    RPi->>RPi: Calculate portion
    RPi->>NRF_P: Send FEED_CMD (msg ID=0x03)
    NRF_P->>NRF_S: 2.4GHz packet
    NRF_S->>STM32: FEED_CMD received
    STM32->>STM32: Start motors + encoders
    Note over STM32: Dispensing feed...
    STM32->>NRF_S: Send FEED_DONE (msg ID=0x04)
    NRF_S->>NRF_P: 2.4GHz packet
    NRF_P->>RPi: FEED_DONE received
    RPi->>DB: Update consumption
    RPi->>MQTT: Publish feed event
    MQTT->>RPi: Acknowledge
```

### Heartbeat & Status Flow

```mermaid
sequenceDiagram
    participant STM32 as STM32 MCU
    participant NRF_S as NRF24L01 (STM)
    participant NRF_P as NRF24L01 (RPi)
    participant RPi as Raspberry Pi

    loop Every 5 seconds
        STM32->>NRF_S: Send STATUS (msg ID=0x01)
        NRF_S->>NRF_P: 2.4GHz packet + auto-ACK
        NRF_P->>RPi: STATUS received
        RPi->>NRF_P: Send ACK (msg ID=0x07)
        NRF_P->>NRF_S: 2.4GHz packet + auto-ACK
        NRF_S->>STM32: ACK received
        Note over STM32: Update connection status
    end
```

---

## Network Topology

```mermaid
graph TB
    subgraph Farm[" Farm Layout"]
        subgraph Station1["Station 1"]
            RPi1["Raspberry Pi<br/>Controller #1"]
            STM1_1["Actuator #1<br/>NUCLEO-L432KC"]
            STM1_2["Actuator #2<br/>NUCLEO-L432KC"]
            STM1_3["Actuator #3<br/>NUCLEO-L432KC"]
            RPi1 <-.-> |"NRF24L01+"| STM1_1
            RPi1 <-.-> |"NRF24L01+"| STM1_2
            RPi1 <-.-> |"NRF24L01+"| STM1_3
        end

        subgraph Station2["Station 2"]
            RPi2["Raspberry Pi<br/>Controller #2"]
            STM2_1["Actuator #1<br/>NUCLEO-L432KC"]
            STM2_2["Actuator #2<br/>NUCLEO-L432KC"]
            RPi2 <-.-> |"NRF24L01+"| STM2_1
            RPi2 <-.-> |"NRF24L01+"| STM2_2
        end
    end

    RPi1 --> |"MQTT"| Cloud[" Agpro Cloud"]
    RPi2 --> |"MQTT"| Cloud
```

---

## Power Architecture

```mermaid
graph TB
    BAT[" 12V Battery<br/>or Adapter"] -->|"12V DC"| IBT["IBT-2 Motor Driver<br/>(5-27V tolerant)"]
    BAT -->|"12V DC"| MINI["Mini360 DC-DC<br/>Buck Converter"]
    MINI -->|"3.3V / 3A"| STM32["NUCLEO-L432KC<br/>(3.3V)"]
    MINI -->|"3.3V / 3A"| NRF["NRF24L01+ Module<br/>(3.3V max)"]
    MINI -->|"3.3V / 3A"| RFID_READER["RFID Reader<br/>(3.3V/5V)"]
    IBT -->|"12V PWM"| MOTOR1["Motor 1<br/>DC Brushed"]
    IBT -->|"12V PWM"| MOTOR2["Motor 2<br/>DC Brushed"]
    IBT -->|"5V out"| ENCODER["Encoder<br/>Power"]
    STM32 -->|"5V USB"| USB["USB Debug<br/>Serial Console"]

    style BAT fill:#fce4ec,stroke:#880e4f
    style MINI fill:#e8eaf6,stroke:#283593
    style IBT fill:#fff3e0,stroke:#e65100
    style STM32 fill:#e8f5e9,stroke:#1b5e20
```

---

## STM32 State Machine

```mermaid
stateDiagram-v2
    [*] --> INIT: Power On
    INIT --> CONFIG_READ: Boot
    CONFIG_READ --> RADIO_INIT: Read config
    RADIO_INIT --> IDLE: NRF24L01+ ready

    IDLE --> HEARTBEAT: Timer (5s)
    HEARTBEAT --> WAIT_ACK: Send STATUS
    WAIT_ACK --> IDLE: ACK received
    WAIT_ACK --> RETRY: No ACK (3 retries)
    RETRY --> IDLE: After retry
    RETRY --> ERROR: After max retries
    ERROR --> IDLE: Reconnect

    IDLE --> TAG_READ: RFID detected
    TAG_READ --> SEND_TAG: Tag valid
    SEND_TAG --> WAIT_CMD: RFID_TAG sent

    WAIT_CMD --> START_FEED: FEED_CMD received
    WAIT_CMD --> IDLE: Timeout (no command)
    START_FEED --> FEEDING: Motors running
    FEEDING --> FEED_DONE: Encoders done
    FEED_DONE --> IDLE: FEED_DONE sent

    IDLE --> WEIGHING: Weight requested
    WEIGHING --> IDLE: WEIGHT sent
```

---

## Message Routing (Raspberry Pi)

```mermaid
stateDiagram-v2
    [*] --> LISTENING: NRF24L01+ initialized

    LISTENING --> PARSE: Packet received
    PARSE --> CRC_CHECK: Parse header
    CRC_CHECK --> ROUTE: CRC valid
    CRC_CHECK --> LISTENING: CRC invalid (discard)

    ROUTE --> STATUS_HANDLER: MsgType=0x01
    ROUTE --> RFID_HANDLER: MsgType=0x02
    ROUTE --> FEEDBACK_HANDLER: MsgType=0x04
    ROUTE --> WEIGHT_HANDLER: MsgType=0x05
    ROUTE --> ERROR_HANDLER: MsgType=0x08

    STATUS_HANDLER --> UPDATE_HEARTBEAT: Update timestamp
    UPDATE_HEARTBEAT --> SEND_ACK: Send ACK (0x07)
    SEND_ACK --> LISTENING

    RFID_HANDLER --> LOOKUP_DB: Lookup animal
    LOOKUP_DB --> CALC_PORTION: Animal found
    LOOKUP_DB --> LISTENING: Unknown animal
    CALC_PORTION --> SEND_CMD: Send FEED_CMD (0x03)
    SEND_CMD --> LISTENING

    FEEDBACK_HANDLER --> UPDATE_DB: Update consumption
    UPDATE_DB --> PUBLISH_MQTT: Publish to cloud
    PUBLISH_MQTT --> LISTENING

    WEIGHT_HANDLER --> STORE_WEIGHT: Store in DB
    STORE_WEIGHT --> LISTENING

    ERROR_HANDLER --> LOG_ERROR: Log to syslog
    LOG_ERROR --> LISTENING
```

---

## Directory Structure

```
wifeeder-v2/
├── README.md                          # Project overview
├── ARCHITECTURE.md                    # This file
├── HARDWARE.md                        # Hardware specs & wiring
├── PROTOCOL.md                        # NRF24L01+ protocol
├── SOFTWARE.md                        # Software architecture
├── TESTING.md                         # Test plan
├── DEPLOYMENT.md                      # Deployment guide
├── TROUBLESHOOTING.md                 # Common issues
├── BILL_OF_MATERIALS.md              # Materials list
│
├── stm32/                             # STM32 firmware
│   ├── Core/
│   │   ├── Inc/                       # Headers
│   │   └── Src/                       # Source
│   ├── Drivers/                       # HAL & CMSIS
│   ├── Middlewares/                   # FreeRTOS
│   └── Config/                        # FreeRTOS config
│
├── raspberry/                         # Raspberry Pi controller
│   ├── src/                           # C++ source
│   ├── include/                       # C++ headers
│   ├── config/                        # JSON configs
│   └── tests/                         # GTest tests
│
├── docs/                              # Additional docs
├── scripts/                           # Build & deploy scripts
├── tests/                             # Integration tests
│   ├── unit/                          # Unit tests
│   ├── integration/                   # Integration tests
│   └── system/                        # System tests
└── assets/                            # Images & diagrams
```

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| NRF24L01+ over CAN | Eliminates wiring issues; cheaper modules |
| NUCLEO-L432KC over L486 | Smaller, cheaper, sufficient peripherals |
| IBT-2 over GTK08 | BTS7960B more robust; better current handling |
| Mini360 buck converter | Efficient 12V→3.3V; adjustable; 3A capacity |
| FreeRTOS (kept) | Real-time motor/encoder control needs |
| SQLite (new on RPi) | Better structured queries vs JSON file |
| 1 Mbps NRF data rate | Best balance of range/speed |

---

## Comparison: v1 vs v2

```mermaid
graph LR
    subgraph V1["v1 (CAN Bus)"]
        direction TB
        C1[" CAN bus wiring<br/>problems for years"]
        C2[" Termination<br/>issues"]
        C3[" Wired connections<br/>limit placement"]
        C4[" Complex<br/>debugging"]
    end

    subgraph V2["v2 (NRF24L01+)"]
        direction TB
        D1[" No wiring<br/>wireless comms"]
        D2[" Auto-retry<br/>reliable delivery"]
        D3[" Flexible<br/>actuator placement"]
        D4[" Simple SPI<br/>easy to debug"]
    end

    V1 --> V2
    style V1 fill:#ffcdd2,stroke:#c62828
    style V2 fill:#c8e6c9,stroke:#2e7d32
```

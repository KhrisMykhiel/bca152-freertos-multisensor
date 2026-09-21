# Real Time Room Monitoring System

A real time room monitoring system built with ESP-IDF and FreeRTOS for the ESP32 chip, simulated on Wokwi.

---

## 1. System Overview
The Room Monitoring System checks indoor temperature, humidity, and light levels. It detects if someone is in the room and turns on a buzzer alarm if the temperature gets too high or too low. It uses FreeRTOS tasks to run smoothly and save power when the room is empty.

---

## 2. Hardware Wiring and Connections

| Part | Component | ESP32 Pin | Connection Type | Setup |
| :--- | :--- | :--- | :--- | :--- |
| Temperature & Humidity | DHT22 | GPIO 4 | Digital Pin | Internal pull-up |
| Light Sensor | Photoresistor (LDR) | GPIO 34 | Analog Pin | Reads values from 0 to 4095 |
| Motion Sensor | PIR Sensor | GPIO 27 | Digital Pin | Pull-down enabled |
| Knob (Turn Signal) | KY-040 (CLK) | GPIO 18 | Interrupt Pin | Reacts to turning |
| Knob (Direction Signal)| KY-040 (DT) | GPIO 19 | Digital Pin | Internal pull-up |
| Knob (Button) | KY-040 (SW) | GPIO 5 | Digital Pin | Internal pull-up |
| Screen | SSD1306 OLED | GPIO 21 (SDA), GPIO 22 (SCL) | I2C | Fast mode |
| Alarm Buzzer | Active Buzzer | GPIO 25 | Digital Pin | Turns on/off |

---

## 3. System Diagram

```mermaid
flowchart TD
    subgraph Hardware["Sensors and Knobs"]
        DHT["DHT22 (Temperature/Humidity)"]
        LDR["LDR (Light Sensor)"]
        PIR["PIR (Motion Sensor)"]
        ENC["Knob"]
    end

    subgraph ISRs["Quick Hardware Events"]
        E_ISR["Knob Turn Event"]
    end

    subgraph Tasks["Running Tasks"]
        MTask["MotionTask"]
        ITask["InputTask"]
        STask["SensorTask"]
        ATask["AlarmTask"]
        StateT["StateTask"]
        DTask["DisplayTask"]
    end

    subgraph IPC["Shared Tools"]
        S_QUEUE[("Sensor Mailbox")]
        D_QUEUE[("Screen Page Mailbox")]
        E_QUEUE[("Knob Turn Mailbox")]
        EVT_GRP{{"System Flags"}}
        MUTEX(("Print Lock"))
    end

    subgraph Outputs["Screen and Alarm"]
        OLED["OLED Screen"]
        BUZZ["Buzzer"]
        UART["Text Output"]
    end

    ENC -->|Knob turned| E_ISR
    E_ISR -->|Send turn| E_QUEUE
    E_QUEUE -->|Read turn| ITask
    ITask -->|Change page| D_QUEUE

    PIR -->|Check motion| MTask
    MTask -->|Set motion flag| EVT_GRP

    DHT -->|Read temp/humid| STask
    LDR -->|Read light| STask
    STask -->|Send data| S_QUEUE

    S_QUEUE -->|Read temp| ATask
    EVT_GRP -->|Check awake| ATask
    ATask -->|Turn on/off| BUZZ
    ATask -->|Set alarm flag| EVT_GRP

    EVT_GRP -->|Wait for motion| StateT
    StateT -->|Manage sleep timer| EVT_GRP

    D_QUEUE -->|Read page| DTask
    S_QUEUE -->|Read data| DTask
    EVT_GRP -->|Check flags| DTask
    DTask -->|Draw picture| OLED

    MTask -.->|Use lock| MUTEX
    ITask -.->|Use lock| MUTEX
    STask -.->|Use lock| MUTEX
    StateT -.->|Use lock| MUTEX
    MUTEX -.-> UART
```

---

## 4. How the Code Works

1. **Sensor Monitoring (`src/sensors.c`)**:
   - The `SensorTask` reads the temperature, humidity, and light level every 2 seconds.
   - It sends the new readings to the sensor mailbox so other tasks can use them.
2. **Rotary Encoder Controls (`src/input.c`)**:
   - Turning the knob triggers a quick hardware event.
   - It checks which way the rotary encoder is turned and sends a message to the knob mailbox.
   - The `InputTask` changes the screen page when it gets the message.
3. **Screen Display (`src/display.c`)**:
   - Updates the OLED screen 5 times a second.
   - Reads the latest sensor data and current screen page to draw the right information.
4. **Alarm (`src/alarm.c`)**:
   - Checks the temperature: if it is below 18.0 C or above 30.0 C, it turns on the buzzer and sets the alarm flag.
5. **Sleep and Wake State (`src/system_state.c`)**:
   - Manages if the system is awake or asleep.
   - If no one moves in the room for 15 seconds, it turns off the screen and goes to sleep.
   - As soon as the motion sensor sees movement, it wakes up immediately.

---

## 5. Build and Test Instructions

### Run Automated Tests
```bash
~/.platformio/penv/bin/pio test -e native
```

### Build the Code for ESP32
```bash
~/.platformio/penv/bin/pio run -e esp32dev
```

### Check for Code Errors
```bash
~/.platformio/penv/bin/pio check -e esp32dev
```

---

## 6. Learning Objectives Demonstrated
- Creating and managing multiple FreeRTOS tasks with explicit priorities
- Using queues for inter-task communication (sensor data, display page changes, encoder events)
- Using a mutex to protect shared UART output
- Using an event group for cross-task signaling (motion, alarm, system-active state)
- Implementing periodic execution with `vTaskDelayUntil()`
- Separating hardware-independent decision logic (`evaluateTemperature`, `evaluateSystemState`, `nextDisplayMode`) from hardware drivers
- Writing and running automated unit tests with PlatformIO/Unity

## 7. FreeRTOS Task Table

| Task | Responsibility | Period/Trigger | Priority | IPC Used | Typical Blocked Condition |
|---|---|---|---|---|---|
| SensorTask | Read DHT22 + LDR | 2 s (vTaskDelayUntil) | 2 | Sends to sensor queue | Delay |
| DisplayTask | Own/update OLED | Periodic / on queue data | 1 | Reads sensor + page queues | Waiting for data |
| InputTask | Rotary encoder navigation | Event-driven (ISR) | 3 | Task notification/queue | Waiting for notification |
| MotionTask | PIR monitoring | Short poll | 3 | Sets event group bits | Delay |
| AlarmTask | Evaluate + drive buzzer | On sensor data | 2 | Reads sensor queue, sets event bits | Waiting for data |
| StateTask | ACTIVE/INACTIVE state machine | Periodic | 2 | Reads/sets event group | Delay |

*Priorities reflect scheduling urgency: MotionTask and InputTask (3) must respond promptly to real-world events; SensorTask/AlarmTask/StateTask (2) tolerate brief latency; DisplayTask (1) is lowest since a slightly stale screen has no functional consequence.*

## 8. Requirements Traceability Matrix

| Requirement | Implementation | Verification |
|---|---|---|
| FR-01 Temperature measurement | SensorTask | FT-01 |
| FR-02 Humidity measurement | SensorTask | FT-02 |
| FR-03 Ambient light | SensorTask | FT-03 |
| FR-04 Motion detection | MotionTask | FT-08, FT-10 |
| FR-05 OLED display | DisplayTask | FT-01–FT-03 |
| FR-06 Encoder navigation | InputTask | FT-04, FT-05 |
| FR-07 Temperature alarm | AlarmTask | FT-06, FT-07 |
| FR-08 ACTIVE/INACTIVE states | StateTask | FT-08–FT-10 |
| FR-09 Auto inactivity | StateTask | FT-09 |
| FR-10 Auto reactivation | StateTask, MotionTask | FT-10 |

## 9. Repository Structure

include/ - Header files (interfaces, shared declarations)
src/ - Implementation (.c) files
test/ - Unity unit tests for hardware-independent logic
diagram.json - Wokwi circuit definition
wokwi.toml - Wokwi simulator config
platformio.ini - Build configuration


## 10. Limitations
- Light level is a raw ADC-to-percentage mapping, not calibrated lux
- DHT22 timing is bit-banged; simulated in Wokwi rather than verified on physical hardware
- No persistent storage of alarm history or configuration across resets

## 11. Future Improvements
- Add NVS-based persistence for the temperature thresholds
- Add a physical hardware build to compare timing against simulation
- Expand static analysis coverage and address any remaining `pio check` findings
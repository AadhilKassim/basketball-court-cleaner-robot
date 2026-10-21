# System Connection Diagram & Pinout Specification

**Project:** Autonomous Basketball Court Cleaner

**Microcontroller:** STM32F411CEU6 Black Pill

**Primary Power Source:** 12V 3S LiPo Battery Rail via 20A Protection Fuse

---

## 1. System Block Diagram Overview

```
                        +----------------------------+
                        |  12V 3S LiPo Battery (BT1) |
                        +--------------+-------------+
                                       |
                                  [20A Fuse]
                                       |
          +----------------------------+----------------------------+
          |                            |                            |
  +-------v-------+            +-------v-------+            +-------v-------+
  | LM2596 (U2)   |            | LM2596 (U3)   |            | High Power    |
  | Step-Down 5V  |            | Step-Down 3.3V|            | DC Rails      |
  +-------+-------+            +-------+-------+            +-------+-------+
          |                            |                            |
          | +5V Rail                   | +3.3V Rail                 | +12V Rail
          |                            |                            |
          +--> STM32 Black Pill (U1)   +--> MPU6050 IMU (U8)        +--> Left BTS7960 Driver (U4)
          +--> BTS7960 VCC Pins (U4/5)                              +--> Right BTS7960 Driver (U5)
          +--> ACS712 Current Sensor                                +--> ACS712 Sensor IP+ Pin
          +--> HC-05 Bluetooth Module                               +--> 775 Crusher Motor Motor+
          +--> HC-SR04 Sensors (U9/10)

```

---

## 2. STM32F411CEU6 Master Pinout Mapping Table

| STM32 Pin | Signal Label | Peripheral Function | Connection Destination Pin | Signal Description / Range |
| --- | --- | --- | --- | --- |
| **PA0** | `PA0_TRIG1` | GPIO Output | HC-SR04 Front Left (`U9` Pin 2 - TRIG) | Ultrasonic Trigger Pulse (3.3V Logic) |
| **PA1** | `PA1_ECHO1_3V3` | EXTI Line 1 | Voltage Divider Junction (`R1` / `R2`) | Scaled Echo Input (0-3.3V) |
| **PA2** | `PA2_TRIG2` | GPIO Output | HC-SR04 Front Right (`U10` Pin 2 - TRIG) | Ultrasonic Trigger Pulse (3.3V Logic) |
| **PA3** | `PA3_ECHO2_3V3` | EXTI Line 3 | Voltage Divider Junction (`R3` / `R4`) | Scaled Echo Input (0-3.3V) |
| **PA4** | `PA4_ADC_CURRENT` | ADC1_IN4 | ACS712 Module Output (`U6` Pin 3 - OUT) | Analog Current Sensor Voltage |
| **PA5** | `PA5_ADC_BATTERY` | ADC1_IN5 | Battery Divider Junction (`R5` / `R6`) | Scaled Battery Monitor Voltage |
| **PA8** | `PA8_CRUSHER_GATE` | GPIO Output | Resistor `R7` Pin 1 | Crusher MOSFET Gate Drive Command |
| **PA9** | `PA9_BT_TX` | USART1_TX | HC-05 Module (`U7` Pin 4 - RXD) | Bluetooth Serial Transmit |
| **PA10** | `PA10_BT_RX` | USART1_RX | HC-05 Module (`U7` Pin 3 - TXD) | Bluetooth Serial Receive |
| **PA15** | `PA15_BTS_ENABLE` | GPIO Output | Left/Right BTS7960 (`U4`/`U5` Pins 3 & 4) | Motor Driver Logic Bridge Enable |
| **PB0** | `PB0_LEFT_RPWM` | TIM3_CH3 (PWM) | Left Driver (`U4` Pin 5 - RPWM) | Forward Direction PWM Signal |
| **PB1** | `PB1_LEFT_LPWM` | TIM3_CH4 (PWM) | Left Driver (`U4` Pin 6 - LPWM) | Reverse Direction PWM Signal |
| **PB6** | `PB6_I2C_SCL` | I2C1_SCL | MPU6050 Module (`U8` Pin 3 - SCL) | I2C Clock Line (3.3V) |
| **PB7** | `PB7_I2C_SDA` | I2C1_SDA | MPU6050 Module (`U8` Pin 4 - SDA) | I2C Data Line (3.3V) |
| **PB8** | `PB8_RIGHT_RPWM` | TIM4_CH3 (PWM) | Right Driver (`U5` Pin 5 - RPWM) | Forward Direction PWM Signal |
| **PB9** | `PB9_RIGHT_LPWM` | TIM4_CH4 (PWM) | Right Driver (`U5` Pin 6 - LPWM) | Reverse Direction PWM Signal |
| **5V** | `+5V` | Power Input | LM2596 Output (`U2` Pin 3) | System Board Power Input |
| **GND** | `GND` | Ground Reference | System Common Ground Rail | Power / Logic Reference Ground |

---

## 3. Subsystem Interconnection Details

### 3.1 Power Distribution & Step-Down Regulators

* **`BT1` (12V 3S LiPo Battery):**
* Pin 1 (`+12V_RAW`) $\rightarrow$ Fuse `F1` Pin 1


* Pin 2 (`GND`) $\rightarrow$ Common Ground Rail




* **`F1` (20A Fuse Block):**
* Pin 2 (`+12V`) $\rightarrow$ `U2` IN+, `U3` IN+, `U4` B+, `U5` B+, `U6` IP+, `R5` Pin 1




* **`U2` (LM2596 5V Buck Converter):**
* Pin 1 (`IN+`) $\rightarrow$ `+12V` Rail


* Pin 2 (`IN-`) $\rightarrow$ `GND`

* Pin 3 (`OUT+`) $\rightarrow$ `+5V` Rail


* Pin 4 (`OUT-`) $\rightarrow$ `GND`



* **`U3` (LM2596 3.3V Buck Converter):**
* Pin 1 (`IN+`) $\rightarrow$ `+12V` Rail


* Pin 2 (`IN-`) $\rightarrow$ `GND`

* Pin 3 (`OUT+`) $\rightarrow$ `+3.3V` Rail


* Pin 4 (`OUT-`) $\rightarrow$ `GND`




---

### 3.2 Drive Motors (BTS7960 Drivers)

#### Left Motor Driver (`U4` - BTS7960)

* Pin 1 (`VCC`) $\rightarrow$ `+5V` Rail


* Pin 2 (`GND`) $\rightarrow$ `GND`

* Pin 3 (`R_EN`) $\rightarrow$ STM32 `PA15`

* Pin 4 (`L_EN`) $\rightarrow$ STM32 `PA15`

* Pin 5 (`RPWM`) $\rightarrow$ STM32 `PB0`

* Pin 6 (`LPWM`) $\rightarrow$ STM32 `PB1`

* Pin 7 (`B+`) $\rightarrow$ `+12V` Rail


* Pin 8 (`B-`) $\rightarrow$ `GND`

* Pin 9 (`M+`) $\rightarrow$ Left Johnson Motor `M2` Pin 1


* Pin 10 (`M-`) $\rightarrow$ Left Johnson Motor `M2` Pin 2



#### Right Motor Driver (`U5` - BTS7960)

* Pin 1 (`VCC`) $\rightarrow$ `+5V` Rail


* Pin 2 (`GND`) $\rightarrow$ `GND`

* Pin 3 (`R_EN`) $\rightarrow$ STM32 `PA15`

* Pin 4 (`L_EN`) $\rightarrow$ STM32 `PA15`

* Pin 5 (`RPWM`) $\rightarrow$ STM32 `PB8`

* Pin 6 (`LPWM`) $\rightarrow$ STM32 `PB9`

* Pin 7 (`B+`) $\rightarrow$ `+12V` Rail


* Pin 8 (`B-`) $\rightarrow$ `GND`

* Pin 9 (`M+`) $\rightarrow$ Right Johnson Motor `M3` Pin 1


* Pin 10 (`M-`) $\rightarrow$ Right Johnson Motor `M3` Pin 2



---

### 3.3 Crusher Motor Subsystem (775 Motor & MOSFET Drive)

* **Current Sensing (`U6` - ACS712 20A):**
* Pin 1 (`VCC`) $\rightarrow$ `+5V` Rail


* Pin 2 (`GND`) $\rightarrow$ `GND`

* Pin 3 (`OUT`) $\rightarrow$ STM32 `PA4`

* Pin 4 (`IP+`) $\rightarrow$ `+12V` Rail


* Pin 5 (`IP-`) $\rightarrow$ 775 Motor `M1` Pin 1 & Flyback Diode `D1` Cathode (Pin 2)




* **MOSFET Switching (`Q1` - IRLZ44N):**
* Pin 1 (`GATE`) $\rightarrow$ Gate Resistor `R7` (220 $\Omega$) Pin 2


* Pin 2 (`DRAIN`) $\rightarrow$ 775 Motor `M1` Pin 2 & Flyback Diode `D1` Anode (Pin 1)


* Pin 3 (`SOURCE`) $\rightarrow$ `GND`




---

### 3.4 Sensors & Peripherals

#### Ultrasonic Sensor #1 - Front Left (`U9` - HC-SR04)

* Pin 1 (`VCC`) $\rightarrow$ `+5V` Rail


* Pin 2 (`TRIG`) $\rightarrow$ STM32 `PA0`

* Pin 3 (`ECHO`) $\rightarrow$ Resistor `R1` (20k $\Omega$) Pin 1


* Pin 4 (`GND`) $\rightarrow$ `GND`

* *Voltage Divider:* `R1` Pin 2 connects to `R2` (10k $\Omega$) Pin 1 and STM32 `PA1`. `R2` Pin 2 connects to `GND`.



#### Ultrasonic Sensor #2 - Front Right (`U10` - HC-SR04)

* Pin 1 (`VCC`) $\rightarrow$ `+5V` Rail


* Pin 2 (`TRIG`) $\rightarrow$ STM32 `PA2`

* Pin 3 (`ECHO`) $\rightarrow$ Resistor `R3` (20k $\Omega$) Pin 1


* Pin 4 (`GND`) $\rightarrow$ `GND`

* *Voltage Divider:* `R3` Pin 2 connects to `R4` (10k $\Omega$) Pin 1 and STM32 `PA3`. `R4` Pin 2 connects to `GND`.



#### Battery Monitoring Voltage Divider

* Resistor `R5` (10k $\Omega$) Pin 1 $\rightarrow$ `+12V` Rail


* Resistor `R5` Pin 2 $\rightarrow$ Resistor `R6` (3.3k $\Omega$) Pin 1 & STM32 `PA5`

* Resistor `R6` Pin 2 $\rightarrow$ `GND`


#### Bluetooth Module (`U7` - HC-05)

* Pin 1 (`VCC`) $\rightarrow$ `+5V` Rail


* Pin 2 (`GND`) $\rightarrow$ `GND`

* Pin 3 (`TXD`) $\rightarrow$ STM32 `PA10` (`USART1_RX`)


* Pin 4 (`RXD`) $\rightarrow$ STM32 `PA9` (`USART1_TX`)



#### IMU Sensor (`U8` - MPU6050)

* Pin 1 (`VCC`) $\rightarrow$ `+3.3V` Rail


* Pin 2 (`GND`) $\rightarrow$ `GND`

* Pin 3 (`SCL`) $\rightarrow$ STM32 `PB6`

* Pin 4 (`SDA`) $\rightarrow$ STM32 `PB7`


---

## 4. Complete Netlist Reference Table

| Net Name | Connected Components & Pins |
| --- | --- |
| **`GND`** | `BT1` (2), `U1` (2), `U2` (2, 4), `U3` (2, 4), `U4` (2, 8), `U5` (2, 8), `U6` (2), `U7` (2), `U8` (2), `U9` (4), `U10` (4), `Q1` (3), `R2` (2), `R4` (2), `R6` (2) |
| **`+12V_RAW`** | `BT1` (1), `F1` (1) |
| **`+12V`** | `F1` (2), `U2` (1), `U3` (1), `U4` (7), `U5` (7), `U6` (4), `R5` (1) |
| **`+12V_CRUSHER`** | `U6` (5), `M1` (1), `D1` (2) |
| **`CRUSHER_DRAIN`** | `M1` (2), `D1` (1), `Q1` (2) |
| **`+5V`** | `U2` (3), `U1` (1), `U4` (1), `U5` (1), `U6` (1), `U7` (1), `U9` (1), `U10` (1) |
| **`+3.3V`** | `U3` (3), `U8` (1) |
| **`PA0_TRIG1`** | `U1` (4), `U9` (2) |
| **`US1_ECHO_5V`** | `U9` (3), `R1` (1) |
| **`PA1_ECHO1_3V3`** | `U1` (5), `R1` (2), `R2` (1) |
| **`PA2_TRIG2`** | `U1` (6), `U10` (2) |
| **`US2_ECHO_5V`** | `U10` (3), `R3` (1) |
| **`PA3_ECHO2_3V3`** | `U1` (7), `R3` (2), `R4` (1) |
| **`PA4_ADC_CURRENT`** | `U1` (8), `U6` (3) |
| **`PA5_ADC_BATTERY`** | `U1` (9), `R5` (2), `R6` (1) |
| **`PA8_CRUSHER_GATE`** | `U1` (10), `R7` (1) |
| **`MOSFET_GATE`** | `R7` (2), `Q1` (1) |
| **`PA9_BT_TX`** | `U1` (11), `U7` (4) |
| **`PA10_BT_RX`** | `U1` (12), `U7` (3) |
| **`PA15_BTS_ENABLE`** | `U1` (13), `U4` (3, 4), `U5` (3, 4) |
| **`PB0_LEFT_RPWM`** | `U1` (14), `U4` (5) |
| **`PB1_LEFT_LPWM`** | `U1` (15), `U4` (6) |
| **`PB6_I2C_SCL`** | `U1` (16), `U8` (3) |
| **`PB7_I2C_SDA`** | `U1` (17), `U8` (4) |
| **`PB8_RIGHT_RPWM`** | `U1` (18), `U5` (5) |
| **`PB9_RIGHT_LPWM`** | `U1` (19), `U5` (6) |
| **`LEFT_M_PLUS`** | `U4` (9), `M2` (1) |
| **`LEFT_M_MINUS`** | `U4` (10), `M2` (2) |
| **`RIGHT_M_PLUS`** | `U5` (9), `M3` (1) |
| **`RIGHT_M_MINUS`** | `U5` (10), `M3` (2) |
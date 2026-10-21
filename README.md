# Basketball Court Cleaner Robot

An STM32F411-based mobile robot controller for cleaning a basketball court. The
firmware combines differential-drive motor control, a crusher or roller motor,
ultrasonic distance sensing, battery and motor-current monitoring, Bluetooth
manual control, and USB CDC diagnostics.

## Functionality

After reset, the firmware initializes the clock, GPIO, ADC, I2C, timers, UART,
and USB device stack. It then:

1. Starts TIM3 as a free-running counter for microsecond timing.
2. Enables the motor-driver logic on `PA15`.
3. Starts PWM outputs for the left and right drive motors.
4. Enables interrupt-driven Bluetooth reception on USART1 at `9600` baud.
5. Sends a boot and peripheral-status report over USB CDC.
6. Repeats the sensor, safety, telemetry, and status cycle every 500 ms.

The current firmware provides the control and sensing foundation for autonomous
operation. The main loop reports sensor values and performs stall protection;
it does not yet contain a path-planning or court-coverage algorithm.

## Hardware Interfaces

| Function | MCU interface | Details |
| --- | --- | --- |
| Left drive motor | TIM3 CH3 / CH4 | `PB0` / `PB1`, forward and reverse PWM |
| Right drive motor | TIM4 CH3 / CH4 | `PB8` / `PB9`, forward and reverse PWM |
| Motor-driver enable | GPIO `PA15` | Set high during normal operation |
| Crusher or roller motor | GPIO `PA8` | On/off output, toggled by Bluetooth command `C` |
| Ultrasonic sensor 1 | Trigger `PA0`, echo `PA1` | Rising/falling-edge echo timing |
| Ultrasonic sensor 2 | Trigger `PA2`, echo `PA3` | Rising/falling-edge echo timing |
| Battery voltage | ADC channel 5, `PA5` | Scaled using the `4.251` divider factor |
| Roller current | ADC channel 4 | Compared with the `2800` jam threshold |
| Bluetooth control | USART1 | `9600 8N1`, interrupt-driven, one-byte commands |
| Diagnostics | USB CDC | Boot log and periodic telemetry |
| Onboard status LED | GPIO `PC13` | Toggled once per main-loop cycle |

See [CONNECTIONS.md](CONNECTIONS.md) for the complete pinout, power distribution,
subsystem wiring, and netlist reference.

I2C1 is initialized at `100 kHz` for future or connected peripherals, but the
current application does not actively exchange data over I2C.

## Main Control Loop

Each cycle performs the following operations:

- Reads the battery voltage through ADC channel 5.
- Samples the roller current through ADC channel 4.
- If the current exceeds `2800`, turns off the roller, reverses both drive
	motors at speed `600` for 800 ms, and then stops them.
- Measures both ultrasonic sensors with a 20 ms gap to reduce acoustic
	cross-talk.
- Sends battery and distance telemetry over USB CDC.
- Toggles the onboard LED and waits 500 ms.

Battery low-voltage shutdown is implemented in the source but currently
disabled for USB testing without the battery attached. Re-enable that check
only after confirming the voltage-divider calibration and power hardware.

## Bluetooth Commands

Commands are received as single ASCII bytes over USART1:

| Command | Action |
| --- | --- |
| `F` | Drive forward at speed 700 |
| `B` | Drive backward at speed 700 |
| `L` | Spin left at speed 500 |
| `R` | Spin right at speed 500 |
| `S` | Stop both drive motors |
| `C` | Toggle the crusher or roller motor |
| `Z` | Stop the motors and enter MCU STOP mode |

Unknown commands are ignored. An interrupt callback automatically arms the
next one-byte receive after processing each command.

## USB Telemetry

The USB CDC output includes a boot report with clock speeds and peripheral
status, followed by lines in this format:

```text
Loop Running | Batt: 12.34V | D1: 45.6cm | D2: 78.9cm
```

On Linux, the device is normally available as `/dev/ttyACM0`. See
[USAGE.md](USAGE.md) for build, flash, and serial-monitoring instructions.

## Safety and Power Behavior

- Drive motors are set to zero before entering STOP mode.
- The motor-driver enable and crusher outputs are disabled before STOP mode.
- A detected roller jam causes an automatic reverse maneuver and stop.
- The low-battery shutdown path is present but intentionally commented out in
	the current USB-testing firmware.

## Project Structure

- `Core/Src/main.c` contains application initialization, the control loop,
	peripheral helpers, motor control, sensing, and Bluetooth handling.
- `Core/Inc/main.h` includes the STM32 HAL interface used by the application.
- `USB_DEVICE/` contains the USB CDC device implementation.
- `Drivers/` and `Middlewares/` contain the STM32 HAL, CMSIS, and USB library
	code.
- `CMakeLists.txt`, `CMakePresets.json`, and `cmake/` define the cross-build.

## Build and Flash

The project uses CMake, Ninja, and the GNU Arm Embedded toolchain. The normal
Debug workflow is:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

The resulting image is `build/Debug/basketballcourtrobot.elf`. Complete setup,
fresh-build, Release-build, ST-LINK flashing, and troubleshooting instructions
are in [USAGE.md](USAGE.md).
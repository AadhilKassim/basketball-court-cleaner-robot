## Prerequisites

Install or make available on `PATH`:

- `arm-none-eabi-gcc`
- CMake 3.22 or newer
- Ninja
- STM32CubeProgrammer CLI (`STM32_Programmer_CLI`)

The project is configured for an STM32F411 and uses the GNU Arm toolchain. Run
all commands from the repository root.

## Build

### Standard Debug build

The CMake preset selects the Ninja generator, the GNU Arm toolchain file, and
the `build/Debug` output directory:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

The firmware image is written to:

```text
build/Debug/basketballcourtrobot.elf
```

### Release build

```sh
cmake --preset Release
cmake --build --preset Release
```

The Release image is at `build/Release/basketballcourtrobot.elf`.

### Fresh build

Remove the selected build directory, then configure and build it again:

```sh
rm -rf build/Debug
cmake --preset Debug
cmake --build --preset Debug
```

Use `build/Release` and the `Release` preset instead when rebuilding Release.

### Manual configuration

If CMake presets are unavailable, configure the project explicitly:

```sh
cmake -B build/Debug -G Ninja \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/Debug
```

## Flash

Connect the ST-LINK probe to the target, then build the image before flashing:

```sh
cmake --preset Debug
cmake --build --preset Debug
Programmer -c port=SWD -w build/Debug/basketballcourtrobot.elf -v -rst
```

`Programmer` is a shell alias for STM32CubeProgrammer CLI. If it is not
available, define it for the current Bash session using the installed CLI path:

```sh
alias Programmer="$HOME/.local/share/stm32cube/bundles/programmer/2.23.0/bin/STM32_Programmer_CLI"
```

For a probe that requires connection recovery mode, a fixed frequency, or a
specific ST-LINK serial number, use the connection options recorded for this
setup. Replace the serial number with the one printed by your programmer:

```sh
Programmer -c port=SWD mode=UR freq=1000 \
	sn=YOUR_STLINK_SERIAL -w build/Debug/basketballcourtrobot.elf -v -rst
```

To list connected programmers and their serial numbers:

```sh
Programmer -l
```

## USB CDC serial output

When the board exposes its USB CDC device as `/dev/ttyACM0`, configure the
terminal and read the output with:

```sh
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0
```

Press `Ctrl+C` to stop reading. If `/dev/ttyACM0` does not exist, reconnect or
reset the board and check available serial devices with `ls /dev/ttyACM*`.
On systems where the device cannot be opened, add your user to the OS serial
device group or run the terminal command with the appropriate permissions.

## Troubleshooting

- Re-run `cmake --preset Debug` after changing the toolchain or CMake files.
- Delete `build/Debug` when CMake has stale configuration or generated files.
- Confirm the target is powered and the ST-LINK is connected before flashing.
- Use `mode=UR` when normal SWD connection cannot attach to a running target.
- Use `build/Release/basketballcourtrobot.elf` when flashing a Release build.
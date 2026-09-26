## Build

### Fresh Build
rm -rf build/Debug

### Relink Tool Chain files
cmake -B build/Debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake

### Build only
cmake --build build/Debug


## Flash
cmake -B build/Debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/Debug
STM32_Programmer_CLI -c port=SWD -w build/Debug/basketballcourtrobot.elf -v -rst
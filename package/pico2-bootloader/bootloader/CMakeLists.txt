# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion RISCV_COREV_MAY_24)
set(picotoolVersion 2.0.0)
set(pico_vscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${pico_vscode})
include(${pico_vscode})
endif()
# ====================================================================================

set(PICO_PLATFORM rp2350-riscv CACHE STRING "Pico Platform")

set(PICO_BOARD sparkfun_promicro_rp2350 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(psram-bootloader C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_compile_definitions(PICO_SDK)

include_directories(psram-bootloader
	src
)

add_executable(psram-bootloader
	src/main.c
	src/image.c
	src/qmi.c
)

# pull in common dependencies
target_link_libraries(psram-bootloader pico_stdlib)

pico_enable_stdio_usb(psram-bootloader 1)
pico_enable_stdio_uart(psram-bootloader 1)

pico_set_binary_type(psram-bootloader copy_to_ram)

# create map/bin/hex file etc.
pico_add_extra_outputs(psram-bootloader)

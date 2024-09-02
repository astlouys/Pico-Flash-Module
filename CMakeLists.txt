# CMakeLists.txt
# For Pico-Flash-Exammple
#
#
cmake_minimum_required(VERSION 3.13)
#
#
# Set Pico SDK path (should be already declared in the environment variables).
# set(PICO_SDK_PATH  "~/pico/pico-sdk")
#
# Pico SDK may be initialized from Git:
# set(PICO_SDK_FETCH_FROM_GIT on)
#
# Pico SDK may also be cloned locally without copying <pico_sdk_import.cmake>.
# include(~/pico/pico-sdk/Pico_sdk_import.cmake)
#
# Here, we selected to create a symbolic link in the project directory.
# This way, the symbolic link always point to the latest version, even when the SDK has been updated to a new version.
include(pico_sdk_import.cmake)
#
#
# Specify the project name.
project(Pico-Flash-Example C CXX ASM)
#
#
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
#
#
set(PICO_BOARD pico)
# set(PICO_BOARD pico_w)
#
#
# Initialize the SDK.
pico_sdk_init()
#
#
# List all source files that are part of the project.
add_executable(Pico-Flash-Example
	Pico-Flash-Example.c
  Pico-Flash-Driver.c
	)
#
#
# Specify all include directories:
target_include_directories(Pico-Flash-Example PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/
  )
#
#
# Determine if we should send Pico's standard output to UART and / or USB.
pico_enable_stdio_uart(Pico-Flash-Example 0)
pico_enable_stdio_usb(Pico-Flash-Example  1)
#
#
# Pull in our pico_stdlib which pulls in commonly used features and other libraries required for this project.
target_link_libraries(Pico-Flash-Example pico_stdlib hardware_flash pico_multicore)
#
#
# Make the binary run from RAM.
# pico_set_binary_type(Pico-Flash-Example no_flash)  # For some reason, this instruction make it put garbage all over flash memory space. Do not use.
#
#
# Create extra files (.bin / .hex / .map / .uf2) in addition to .elf
pico_add_extra_outputs(Pico-Flash-Example)
#
#
# For public repositories, add URL via pico_set_program_url
# example_auto_set_url(Pico-Public-Program)
#
#

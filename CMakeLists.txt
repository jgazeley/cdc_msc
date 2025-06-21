cmake_minimum_required(VERSION 3.17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 1) Tell family_support who we are
set(FAMILY        rp2040)
set(BOARD         pico_sdk)
set(CMAKE_BUILD_TYPE Debug)

# 2) Pull in the Pico-SDK’s TinyUSB BSP
#    (make sure PICO_SDK_PATH is set to your pico-sdk root)
include($ENV{PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

# 3) Derive a friendly project name from this folder
family_get_project_name(PROJECT ${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT} C CXX ASM)

# 4) Initialize the family/board hooks (sets up include paths, link libs, etc)
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

# 5) Espressif boards bail out here
if(FAMILY STREQUAL "espressif")
  return()
endif()

# 6) Create our executable and list only the .c files
add_executable(${PROJECT}
  src/main.c
  src/msc_disk.c
  src/usb_descriptors.c
)

# 7) Make sure “src/” is on the include path so tusb_config.h & friends resolve
target_include_directories(${PROJECT} PUBLIC
  src
)

# 8) Configure this as a “no-OS” device example: links pico_stdlib, hardware_pio, tinyusb_device, etc
family_configure_device_example(${PROJECT} noos)

# 9) Emit UF2, .bin, .hex, .map outputs alongside the ELF
pico_add_extra_outputs(${PROJECT})

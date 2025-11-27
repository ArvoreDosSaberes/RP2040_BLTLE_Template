# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(server C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Standalone example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(server server.cpp bt_server_setup.cpp)

# Enable USB serial
pico_enable_stdio_uart(server 0)
pico_enable_stdio_usb(server 1)

# Add subdirectories of the library
add_subdirectory(lib)

pico_btstack_make_gatt_header(server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")

target_link_libraries(server
    pico_stdlib
    hardware_adc
  
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
  
    log_vt100
    )

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )


pico_add_extra_outputs(server)

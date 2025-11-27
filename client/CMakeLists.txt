# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(test_01_sensor C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Standalone example that connects to picow_ble_temp_sensor and reads the temperature 
# Flahes once quickly each second when it's running but not connected to another device
# Flashes twice quickly each second when connected to another device and reading it's temperature
add_executable(client client.cpp bt_client_setup.cpp)

add_subdirectory(lib)

# Enable USB serial
pico_enable_stdio_uart(client 0)
pico_enable_stdio_usb(client 1)

target_link_libraries(client
    pico_stdlib
    hardware_adc
    hardware_pwm
  
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none    

    log_vt100
    )
target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )
target_compile_definitions(client PRIVATE
    RUNNING_AS_CLIENT=1
)

pico_add_extra_outputs(client)

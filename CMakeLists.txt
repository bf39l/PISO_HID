# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PISO_HID C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(PISO_HID 
    src/main.c
    src/task_oled.c
    src/task_scan.c
    src/task_usb.c
)

pico_set_program_name(PISO_HID "PISO_HID")
pico_set_program_version(PISO_HID "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(PISO_HID 0)
pico_enable_stdio_usb(PISO_HID 0)

#### Library PICO_USB_HID
add_library(CUSTOM_HID
    lib/CustomHID/usb_descriptors.c
    lib/CustomHID/keymap.c
    lib/CustomHID/functional_key_handler.c
    lib/CustomHID/custom_hid.c
)
target_include_directories(CUSTOM_HID PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/lib/CustomHID/ # tusb_config.h and etc...
)
target_link_libraries(CUSTOM_HID
    pico_stdlib
    tinyusb_device
    hardware_flash
)
#####

#### Library OLED_SSD1306
add_library(OLED_SSD1306 
    lib/OLED_SSD1306/oled.c
    lib/OLED_SSD1306/font.c
)

target_link_libraries(OLED_SSD1306
    # link dependency of lib for OLED_SSD1306
    pico_stdlib
    hardware_i2c
)
####

#### Library ShiftRegister-74HC165
add_library(ShiftRegister_74HC165 
    lib/ShiftRegister-74HC165/ShiftRegister_PISO.c
)
target_link_libraries(ShiftRegister_74HC165
    # link dependency of lib for ShiftRegister-74HC165
    pico_stdlib
)
####

#### Library FreeRTOS

##### Set FreeRTOS Kernel path
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
#####
##### Include FreeRTOS config directory
# target_include_directories(<target> <scope> <dirs...>)
# PRIVATE  : Only used when compiling this target. Other targets linking to it do not see these headers.
# PUBLIC   : Used for this target and any targets that link to it.
# INTERFACE: Not used for this target, only for targets that link to it.

# Define SMP macro
target_compile_definitions(PISO_HID PRIVATE FREE_RTOS_KERNEL_SMP)

# Add heap_4.c as source (not library!)
target_sources(PISO_HID PRIVATE
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)
#####
####


# Add the standard library to the build
target_link_libraries(PISO_HID
    pico_stdlib
)

# Add the standard include files to the build
target_include_directories(PISO_HID PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include/
        ${CMAKE_CURRENT_LIST_DIR}/include/FreeRTOS/ # FreeRTOSConfig.h and etc...
        ${FREERTOS_KERNEL_PATH}/include/
        ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/  # port headers

        ${CMAKE_CURRENT_LIST_DIR}/lib/ShiftRegister-74HC165/ # ShiftRegister-74HC165
        ${CMAKE_CURRENT_LIST_DIR}/lib/OLED_SSD1306/ # ShiftRegister-74HC165

        # ${CMAKE_CURRENT_LIST_DIR}/include/TinyUSB/ # tusb_config.h and etc...

        ${CMAKE_CURRENT_LIST_DIR}/lib/CustomHID/ # Custom HID
)

# Add any user requested libraries
target_link_libraries(PISO_HID 
    hardware_i2c
    tinyusb_device
    tinyusb_board
    FreeRTOS-Kernel
    ShiftRegister_74HC165
    OLED_SSD1306
    CUSTOM_HID
)

pico_add_extra_outputs(PISO_HID)


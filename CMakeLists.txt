cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(keyboard)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()



add_executable(keyboard
	keyboard.c
	usb_descriptors.c
	)

target_include_directories(keyboard PRIVATE .)

target_link_libraries(keyboard
	pico_stdlib
	tinyusb_device
)

pico_add_extra_outputs(keyboard)


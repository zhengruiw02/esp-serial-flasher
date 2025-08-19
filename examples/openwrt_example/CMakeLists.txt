cmake_minimum_required(VERSION 3.5)

# add_definitions(-DSERIAL_FLASHER_RESET_INVERT=1)

set(OPENWRT_DIR "/home/henry/workspace/openwrt-linkit-6.12")
set(OPENWRT_TOOLCHAIN_VER "toolchain-mipsel_24kc_gcc-14.3.0_musl")
set(OPENWRT_TOOLCHAIN_BIN "mipsel-openwrt-linux-musl-")

set(CMAKE_C_COMPILER ${OPENWRT_DIR}/staging_dir/${OPENWRT_TOOLCHAIN_VER}/bin/${OPENWRT_TOOLCHAIN_BIN}gcc)
set(CMAKE_CXX_COMPILER ${OPENWRT_DIR}/staging_dir/${OPENWRT_TOOLCHAIN_VER}/bin/${OPENWRT_TOOLCHAIN_BIN}g++)

set(FLASHER_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(PORT LINUX)

project(esp_flasher)

add_compile_definitions(SERIAL_FLASHER_INTERFACE_UART)

add_definitions(-DSINGLE_TARGET_SUPPORT)
# include(${CMAKE_CURRENT_LIST_DIR}/../common/bin2array.cmake)
# create_resources(${CMAKE_CURRENT_LIST_DIR}/../binaries/Hello-world Src/binaries_flash.c)
# create_resources(${CMAKE_CURRENT_LIST_DIR}/../binaries/RAM_APP Src/binaries_ram.c)

# add_executable(${CMAKE_PROJECT_NAME} ../common/example_common.c Src/main.c Src/binaries_flash.c Src/binaries_ram.c)
add_executable(${CMAKE_PROJECT_NAME} ../common/example_common.c Src/main.c Src/read_bin.c)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ../common)

add_subdirectory(${FLASHER_DIR} ${CMAKE_BINARY_DIR}/flasher)

target_compile_options(flasher
PRIVATE
    -Wunused-parameter
    -Wshadow
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE flasher)

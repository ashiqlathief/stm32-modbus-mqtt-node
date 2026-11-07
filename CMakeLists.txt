cmake_minimum_required(VERSION 3.16)
project(stm32_modbus_mqtt_node C ASM)
set(CMAKE_C_STANDARD 11)

set(APP_VERSION 1 CACHE STRING "Application version stored in the signed image")
set(SIGNING_KEY ${CMAKE_SOURCE_DIR}/keys/private.pem CACHE FILEPATH "Ed25519 private key used to sign the image")

# ---------------------------------------------------------------- application (behind the bootloader)
set(FREERTOS ${CMAKE_SOURCE_DIR}/third_party/FreeRTOS-Kernel)
add_executable(firmware src/startup.c src/main.c src/uart.c src/hooks.c src/modbus_core.c src/modbus_port.c src/i2c.c
    ${FREERTOS}/tasks.c ${FREERTOS}/queue.c ${FREERTOS}/list.c
    ${FREERTOS}/portable/GCC/ARM_CM4F/port.c ${FREERTOS}/portable/MemMang/heap_4.c)
target_include_directories(firmware PRIVATE include ${FREERTOS}/include ${FREERTOS}/portable/GCC/ARM_CM4F)
target_compile_definitions(firmware PRIVATE APP_VERSION=${APP_VERSION})
target_compile_options(firmware PRIVATE -Wall -Wextra -Os -g)
# third-party kernel sources are not held to -Wextra
set_source_files_properties(${FREERTOS}/tasks.c ${FREERTOS}/queue.c ${FREERTOS}/list.c ${FREERTOS}/portable/GCC/ARM_CM4F/port.c ${FREERTOS}/portable/MemMang/heap_4.c PROPERTIES COMPILE_OPTIONS "-w")
target_link_options(firmware PRIVATE -T${CMAKE_SOURCE_DIR}/ld/app.ld -Wl,-Map=firmware.map)

# raw binary, then wrap it in the signed image format
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware> ${CMAKE_BINARY_DIR}/firmware.bin
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/sign_image.py --key ${SIGNING_KEY} --version ${APP_VERSION}
            --in ${CMAKE_BINARY_DIR}/firmware.bin --out ${CMAKE_BINARY_DIR}/firmware_signed.bin
    COMMENT "Signing application image (version ${APP_VERSION})")

# ---------------------------------------------------------------- bootloader (needs TweetNaCl in third_party/)
set(NACL ${CMAKE_SOURCE_DIR}/third_party/tweetnacl)
if(EXISTS ${NACL}/tweetnacl.c)
    execute_process(COMMAND ${CMAKE_SOURCE_DIR}/tools/make_keys.sh RESULT_VARIABLE keys_rc)
    if(NOT keys_rc EQUAL 0)
        message(FATAL_ERROR "tools/make_keys.sh failed")
    endif()
    add_executable(bootloader bootloader/startup_boot.c bootloader/main.c bootloader/sha256.c bootloader/verify.c
        src/uart.c ${NACL}/tweetnacl.c)
    target_include_directories(bootloader PRIVATE include bootloader ${NACL} ${CMAKE_BINARY_DIR}/generated)
    target_compile_options(bootloader PRIVATE -Wall -Wextra -Os -g)
    set_source_files_properties(${NACL}/tweetnacl.c PROPERTIES COMPILE_OPTIONS "-w")
    target_link_options(bootloader PRIVATE -T${CMAKE_SOURCE_DIR}/bootloader/boot.ld -Wl,-Map=bootloader.map)
else()
    message(WARNING "third_party/tweetnacl/tweetnacl.c not found: the bootloader target is skipped (see README)")
endif()

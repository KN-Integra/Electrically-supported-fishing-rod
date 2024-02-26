# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(MotorController)

set(CONTROLLER_PATH "MotorControllerFirmware/BrushedMotorControllerFirmware")

target_include_directories(app PRIVATE
     "${CONTROLLER_PATH}/include"
     "src"
)

file(GLOB SRC_files_MC
     "${CONTROLLER_PATH}/include/*.c"
)

file(GLOB SRC_files
     "src/*.c"
)

target_sources(app PRIVATE ${SRC_files_MC} ${SRC_files})

if ( CMAKE_COMPILER_IS_GNUCC )
     if ( CMAKE_C_COMPILER_VERSION LESS 13 )
          target_compile_options(app PRIVATE -Wno-unknown-pragmas)
     endif ()
endif ()

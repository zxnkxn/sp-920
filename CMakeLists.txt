cmake_minimum_required(VERSION 3.13)

set(APP_NAME "sp920")

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(sp920)

sdk_compile_definitions(INIT_EXT_RAM_FOR_DATA=1)

sdk_app_src(
    src/main.c
    src/march_test.c
    src/ram_test.c
    src/read_mac.c
    src/rs485.c
    src/clock_test.c
)

generate_ide_projects()

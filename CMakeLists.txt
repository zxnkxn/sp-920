cmake_minimum_required(VERSION 3.13)

set(APP_NAME "sp920")

# Enable lwIP and multi-port ENET support
set(CONFIG_LWIP 1)
set(CONFIG_ENET_PHY 1)
set(APP_USE_ENET_PORT_COUNT 2)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(${APP_NAME})

# SDRAM init during startup
sdk_compile_definitions(INIT_EXT_RAM_FOR_DATA=1)

# lwIP / ENET sample-style configuration
sdk_compile_definitions(-DNO_SYS=1)
sdk_compile_definitions(-D__DISABLE_AUTO_NEGO=1)
sdk_compile_definitions(-D__ENABLE_ENET_RECEIVE_INTERRUPT=1)
sdk_compile_definitions(-DENET_MULTIPLE_PORT=1)
sdk_compile_definitions(-DLWIP_DHCP=0)
sdk_compile_definitions(-DUSE_LWIPOPTS_APP_H=1)
sdk_compile_definitions(-DLWIP_SUPPORT_CUSTOM_PBUF=1)

# Static IP configuration for two interfaces
sdk_compile_definitions(-DIP0_CONFIG=\"192.168.100.10\")
sdk_compile_definitions(-DNETMASK0_CONFIG=\"255.255.255.0\")
sdk_compile_definitions(-DGW0_CONFIG=\"192.168.100.1\")

sdk_compile_definitions(-DIP1_CONFIG=\"192.168.200.10\")
sdk_compile_definitions(-DNETMASK1_CONFIG=\"255.255.255.0\")
sdk_compile_definitions(-DGW1_CONFIG=\"192.168.200.1\")

# MAC addresses are read from external 24AA025E48 EEPROMs at runtime
# sdk_compile_definitions(-DMAC0_CONFIG=\"02:12:34:56:78:00\")
# sdk_compile_definitions(-DMAC1_CONFIG=\"02:12:34:56:78:01\")

# Application include paths
sdk_inc(src)
sdk_inc(src/app)
sdk_inc(src/arch)

# Main application sources
sdk_app_src(
    src/main.c
    src/lwip.c
    src/common.c
    src/netconf.c
    src/ethernetif.c
    src/sys_arch.c
    src/ksz8463.c
    src/app/tcp_echo.c
    src/march_test.c
    src/ram_test.c
    src/read_mac.c
    src/rs485.c
    src/clock_test.c
    src/mac_eeprom.c
    src/apm_uart_test.c
)

generate_ide_projects()

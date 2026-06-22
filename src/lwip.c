/*
 * Dual-port lwIP TCP echo application for SP-920
 *
 * This file is based on the HPM SDK multi-port lwIP sample.
 */

#include <stdio.h>
#include "common.h"
#include "netconf.h"
#include "sys_arch.h"
#include "lwip/init.h"
#include "tcp_echo.h"
#include "ksz8463.h"
#include "ethernetif.h"

int lwip_main(void)
{
    // Initialize board basics: clocks, console, PMP
    //board_init();

    printf("\r\nSP-920 Ethernet test\r\n");
    printf("LwIP Version: %s\r\n\r\n", LWIP_VERSION_STRING);

    // Initialize ENET RMII pinmux for both interfaces
    board_init_multiple_enet_pins();

    // Reset both external KSZ8463 switches
    board_reset_multiple_enet_phy();

    // Give external switches some time after reset
    board_delay_ms(20);

    // Initialize the SPI interface used to access KSZ8463 registers
    if (ksz8463_spi_init() != status_success) {
        printf("KSZ8463 SPI init failed!\r\n\r\n");
        return -1;
    }

    // Configure both KSZ8463 devices for RMII host port and fiber mode
    if (ksz8463_init_device(0) != status_success) {
        printf("KSZ8463 #0 init failed!\r\n\r\n");
        return -1;
    }

    if (ksz8463_init_device(1) != status_success) {
        printf("KSZ8463 #1 init failed!\r\n\r\n");
        return -1;
    }

    // Initialize ENET clocks for both ports
    board_init_multiple_enet_clock();

    // Initialize I2C0 for reading factory MAC addresses from EEPROM
    board_init_i2c(HPM_I2C0);

    // Initialize MAC and DMA for both interfaces
    for (uint8_t i = 0; i < BOARD_ENET_COUNT; i++) {
        if (enet_init(i) == status_success) {
            printf("ENET%d init passed!\r\n", i);
        } else {
            printf("ENET%d init failed!\r\n", i);
            return -1;
        }
    }

    // Initialize lwIP core
    lwip_init();

    // Configure network interfaces and start TCP echo on both ports
    for (uint8_t i = 0; i < BOARD_ENET_COUNT; i++) {
        netif_config(&gnetif[i], i);
        enet_services(&gnetif[i]);
        tcp_echo_init(&gnetif[i]);
    }

    // Start the lwIP periodic timer
    board_timer_create(LWIP_APP_TIMER_INTERVAL, sys_timer_callback);

    // Poll both interfaces forever
    while (1) {
        for (uint8_t i = 0; i < BOARD_ENET_COUNT; i++) {
            enet_common_handler(&gnetif[i]);
        }
    }

    return 0;
}

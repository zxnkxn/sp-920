/*
 * Main entry point for the SP-920 application.
 */

#include <stdio.h>

#include "board.h"
#include "ksz8463.h"
#include "lwip.h"
#include "ram_test.h"
#include "read_mac.h"
#include "rs485.h"
#include "clock_test.h"
#include "apm_uart_test.h"

#define RUN_RAM_TEST        1
#define RUN_READ_MAC        1
#define RUN_RS485_TEST      1
#define RUN_CLOCK_TEST      1
#define RUN_APM_UART_TEST   0

#define RUN_KSZ_SPI_TEST    0
#define RUN_LWIP_TCP_ECHO   1

int main(void)
{
    board_init();

#if RUN_RAM_TEST
    ram_test_run();
#endif

#if RUN_READ_MAC
    read_mac_run();
#endif

#if RUN_RS485_TEST
    rs485_run();
#endif

#if RUN_CLOCK_TEST
    clock_test_run();
#endif

#if RUN_APM_UART_TEST
    apm_uart_test_run();
#endif

#if RUN_KSZ_SPI_TEST
    return (ksz8463_bringup_test() == status_success) ? 0 : -1;
#endif

#if RUN_LWIP_TCP_ECHO
    return lwip_main();
#endif

    while (1) {
    }
}

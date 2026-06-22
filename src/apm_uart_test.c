#include <stdio.h>
#include <string.h>

#include "board.h"
#include "hpm_uart_drv.h"
#include "apm_uart_test.h"

#define APM_UART          HPM_UART7
#define APM_UART_BAUDRATE (115200U)

static void apm_uart_init(void)
{
    uart_config_t config = {0};

    board_init_uart(APM_UART);

    uart_default_config(APM_UART, &config);
    config.baudrate = APM_UART_BAUDRATE;
    config.fifo_enable = true;
    config.src_freq_in_hz = board_init_uart_clock(APM_UART);

    if (uart_init(APM_UART, &config) != status_success) {
        printf("APM UART7 init failed\r\n");
        while (1) {
        }
    }

    printf("APM UART7 initialized: 115200 8N1\r\n");
}

static void apm_uart_send_string(const char *s)
{
    while (*s) {
        while (!uart_check_status(APM_UART, uart_stat_tx_slot_avail)) {
        }
        uart_send_byte(APM_UART, (uint8_t)*s++);
    }

    while (!uart_check_status(APM_UART, uart_stat_transmitter_empty)) {
    }
}

static void apm_uart_poll_rx(uint32_t timeout_ms)
{
    uint32_t timeout_us = timeout_ms * 1000U;
    bool got_any = false;

    while (timeout_us > 0U) {
        if (uart_check_status(APM_UART, uart_stat_data_ready)) {
            //uint8_t ch = uart_receive_byte(APM_UART);
            uint8_t ch = 0;

            if (uart_receive_byte(APM_UART, &ch) != status_success) {
                continue;
            }

            if (!got_any) {
                printf("RX from APM: ");
                got_any = true;
            }

            if ((ch >= 32U) && (ch <= 126U)) {
                printf("%c", ch);
            } else if (ch == '\r') {
                printf("\\r");
            } else if (ch == '\n') {
                printf("\\n");
            } else {
                printf("[0x%02X]", ch);
            }
        } else {
            board_delay_us(10);
            timeout_us -= 10U;
        }
    }

    if (got_any) {
        printf("\r\n");
    } else {
        printf("RX timeout: no answer from APM\r\n");
    }
}

void apm_uart_test_run(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("SP-920 HPM <-> APM UART test\r\n");
    printf("HPM side: UART7\r\n");
    printf("HPM TX_MCU1: PA11 / UART7_TXD\r\n");
    printf("HPM RX_MCU1: PA10 / UART7_RXD\r\n");
    printf("Baudrate: 115200 8N1\r\n");
    printf("========================================\r\n");

    apm_uart_init();

    while (1) {
        const char *msg = "PING_FROM_HPM\r\n";

        printf("TX to APM: %s", msg);
        apm_uart_send_string(msg);

        apm_uart_poll_rx(500);

        printf("----\r\n");
        board_delay_ms(1000);
    }
}

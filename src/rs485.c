#include <stdio.h>
#include "board.h"
#include <string.h>
#include "hpm_uart_drv.h"
#include "hpm_gpio_drv.h"
#include "rs485.h"

// RS485_2 is connected to UART3
#define RS485_2_UART              HPM_UART3
#define RS485_2_BAUDRATE          (115200U)
#define RS485_2_DIR_GPIO          HPM_GPIO0
#define RS485_2_DIR_PAD           IOC_PAD_PA09

// RS485_3 is connected to UART1
#define RS485_3_UART              HPM_UART1
#define RS485_3_BAUDRATE          (115200U)
#define RS485_3_DIR_GPIO          HPM_GPIO0
#define RS485_3_DIR_PAD           IOC_PAD_PA08

// Convert pad number to GPIO port/pin indexes
#define RS485_2_DIR_GPIO_INDEX    GPIO_GET_PORT_INDEX(RS485_2_DIR_PAD)
#define RS485_2_DIR_GPIO_PIN      GPIO_GET_PIN_INDEX(RS485_2_DIR_PAD)

#define RS485_3_DIR_GPIO_INDEX    GPIO_GET_PORT_INDEX(RS485_3_DIR_PAD)
#define RS485_3_DIR_GPIO_PIN      GPIO_GET_PIN_INDEX(RS485_3_DIR_PAD)

/*
 * With RE and DE tied together:
 * DIR = 0 -> receiver enabled, transmitter disabled
 * DIR = 1 -> transmitter enabled, receiver disabled
 */
#define RS485_DIR_RX_MODE         (0U)
#define RS485_DIR_TX_MODE         (1U)

static void rs485_2_set_rx_mode(void)
{
    // Enable receiver, disable transmitter
    gpio_write_pin(RS485_2_DIR_GPIO, RS485_2_DIR_GPIO_INDEX, RS485_2_DIR_GPIO_PIN, RS485_DIR_RX_MODE);
}

static void rs485_2_set_tx_mode(void)
{
    // Enable transmitter, disable receiver
    gpio_write_pin(RS485_2_DIR_GPIO, RS485_2_DIR_GPIO_INDEX, RS485_2_DIR_GPIO_PIN, RS485_DIR_TX_MODE);
}

static void rs485_3_set_rx_mode(void)
{
    // Enable receiver, disable transmitter
    gpio_write_pin(RS485_3_DIR_GPIO, RS485_3_DIR_GPIO_INDEX, RS485_3_DIR_GPIO_PIN, RS485_DIR_RX_MODE);
}

static void rs485_3_set_tx_mode(void)
{
    // Enable transmitter, disable receiver
    gpio_write_pin(RS485_3_DIR_GPIO, RS485_3_DIR_GPIO_INDEX, RS485_3_DIR_GPIO_PIN, RS485_DIR_TX_MODE);
}

static void rs485_2_init(void)
{
    uart_config_t config = {0};

    // Configure UART3 pins and UART3 clock
    board_init_uart(RS485_2_UART);

    // Configure PA09 as GPIO for RS485_2_DIR
    init_rs485_2_dir_pin();
    gpio_set_pin_output_with_initial(RS485_2_DIR_GPIO,
                                     RS485_2_DIR_GPIO_INDEX,
                                     RS485_2_DIR_GPIO_PIN,
                                     RS485_DIR_RX_MODE);

    // Configure UART3
    uart_default_config(RS485_2_UART, &config);
    config.baudrate = RS485_2_BAUDRATE;
    config.fifo_enable = true;
    config.src_freq_in_hz = board_init_uart_clock(RS485_2_UART);

    if (uart_init(RS485_2_UART, &config) != status_success) {
        printf("RS485_2 UART init failed\r\n");
        while (1) {
        }
    }

    // Default state after init: receive mode
    rs485_2_set_rx_mode();
}

static void rs485_3_init(void)
{
    uart_config_t config = {0};

    // Configure UART1 pins and UART1 clock
    board_init_uart(RS485_3_UART);

    // Configure PA08 as GPIO for RS485_3_DIR
    init_rs485_3_dir_pin();
    gpio_set_pin_output_with_initial(RS485_3_DIR_GPIO,
                                     RS485_3_DIR_GPIO_INDEX,
                                     RS485_3_DIR_GPIO_PIN,
                                     RS485_DIR_RX_MODE);

    // Configure UART1
    uart_default_config(RS485_3_UART, &config);
    config.baudrate = RS485_3_BAUDRATE;
    config.fifo_enable = true;
    config.src_freq_in_hz = board_init_uart_clock(RS485_3_UART);

    if (uart_init(RS485_3_UART, &config) != status_success) {
        printf("RS485_3 UART init failed\r\n");
        while (1) {
        }
    }

    // Default state after init: receive mode
    rs485_3_set_rx_mode();
}

static void rs485_send_blocking(UART_Type *uart, const uint8_t *data, uint32_t len, void (*set_tx)(void), void (*set_rx)(void))
{
    uint32_t i;

    if ((uart == NULL) || (data == NULL) || (len == 0U)) {
        return;
    }

    // Switch the transceiver to transmit mode
    set_tx();

    // Give the transceiver a short time to enable the driver
    board_delay_us(2);

    // Send payload byte-by-byte through UART
    for (i = 0; i < len; i++) {
        // Wait until TX FIFO has free slot
        while (!uart_check_status(uart, uart_stat_tx_slot_avail)) {
        }
        uart_send_byte(uart, data[i]);
    }

    // Wait until the UART finished transmitting everything
    while (!uart_check_status(uart, uart_stat_transmitter_empty)) {
    }

    // Give the transceiver a short time before returning to RX mode
    board_delay_us(2);

    // Switch the transceiver back to receive mode
    set_rx();
}

static void rs485_2_send_text(const char *text)
{
    if (text == NULL) {
        return;
    }

    rs485_send_blocking(RS485_2_UART,
                        (const uint8_t *)text,
                        (uint32_t)strlen(text),
                        rs485_2_set_tx_mode,
                        rs485_2_set_rx_mode);
}

static void rs485_3_send_text(const char *text)
{
    if (text == NULL) {
        return;
    }

    rs485_send_blocking(RS485_3_UART,
                        (const uint8_t *)text,
                        (uint32_t)strlen(text),
                        rs485_3_set_tx_mode,
                        rs485_3_set_rx_mode);
}

void rs485_run(void)
{
    printf("\r\n--- RS485 TEST ---\r\n");

    rs485_2_init();
    rs485_3_init();

    printf("RS485_2 and RS485_3 are initialized\r\n");
    printf("UART0 is used as console interface\r\n");
    printf("Test messages will be sent to PC via UPort 1150:\r\n");
    printf("- RS485_2: UART3 -> DD12 -> RS-485 line -> PC\r\n");
    printf("- RS485_3: UART1 -> DD13 -> RS-485 line -> PC\r\n");

    printf("Sending test message via RS485_2 (UART3 -> DD12 -> PC)...\r\n");
    rs485_2_send_text("Test message from RS485_2: UART3 -> DD12 -> PC via UPort 1150\r\n");

    board_delay_ms(100);

    printf("Sending test message via RS485_3 (UART1 -> DD13 -> PC)...\r\n");
    rs485_3_send_text("Test message from RS485_3: UART1 -> DD13 -> PC via UPort 1150\r\n");

    printf("RS485 test completed\r\n");
}

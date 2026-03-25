#include <stdio.h>
#include "board.h"
#include <string.h>
#include "hpm_i2c_drv.h"
#include "read_mac.h"

static uint8_t i2c_rx_buff[16] = {0};
static uint8_t i2c_addr[2] = {0x50, 0x52};
static uint8_t i2c_addr_reg = 0xFA;

// Read MAC (EUI-48) addresses from two external EEPROMs (24AA025E48) via I2C
// and print them to the board console (system UART) using printf().
// -----------------------------------------------------------------------------
//
// - HPM_I2C0:
//   Base pointer to the I2C0 peripheral registers inside the HPM6754 MCU.
//   This selects WHICH I2C controller hardware block is used (I2C0).
//
// - i2c_addr[]:
//   7-bit I2C slave addresses of the external EEPROM devices on the I2C bus.
//   Example: 0x50 and 0x52 (depends on the board wiring / A0-A2 strap).
//
// - i2c_addr_reg:
//   Internal memory address inside the EEPROM from which we start reading.
//   For 24AA025E48, the factory-programmed EUI-48 (MAC) is commonly stored at 0xFA..0xFF.
//
// - i2c_master_address_read():
//   Performs an EEPROM "random read":
//     START + (slave addr + W) + (mem addr) + RESTART + (slave addr + R) + data... + STOP
//
static void read_mac_i2c(void)
{
    const uint32_t mac_len = 6;   // EUI-48 length in bytes

    // Read MAC from two EEPROMs (EEPROM1 and EEPROM2)
    for (int i = 0; i < 2; i++)
    {
        // Read 6 bytes starting from EEPROM internal address i2c_addr_reg (0xFA).
        // The API expects:
        //   - pointer to "memory address bytes" (here 1 byte: 0xFA)
        //   - size of that address field (1)
        hpm_stat_t status = i2c_master_address_read(
            HPM_I2C0,          // MCU I2C controller instance (I2C0 inside HPM6754)
            i2c_addr[i],       // 7-bit I2C slave address of the EEPROM (e.g. 0x50 / 0x52)
            &i2c_addr_reg,     // EEPROM internal address pointer (start address = 0xFA)
            1,                 // internal address size in bytes (24AA025 uses 1-byte addressing)
            i2c_rx_buff,       // RX buffer where data will be stored
            mac_len            // number of bytes to read (6 bytes MAC)
        );

        // Small optional delay; can be removed if not needed.
        board_delay_ms(10);

        if (status == status_success)
        {
            // Print MAC in standard colon-separated format.
            // printf() output goes to the board console (system UART) if console is configured.
            printf("MAC%d (I2C_ADDR=0x%02X, MEM=0x%02X): %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                   i,
                   i2c_addr[i],
                   i2c_addr_reg,
                   i2c_rx_buff[0], i2c_rx_buff[1], i2c_rx_buff[2],
                   i2c_rx_buff[3], i2c_rx_buff[4], i2c_rx_buff[5]);
        }
        else
        {
            // Print an error message with useful debug info.
            printf("MAC%d read FAILED (I2C_ADDR=0x%02X, MEM=0x%02X), status=%d\r\n",
                   i, i2c_addr[i], i2c_addr_reg, (int)status);
        }

        // Clear RX buffer (optional, but helps debugging).
        memset(i2c_rx_buff, 0, sizeof(i2c_rx_buff));
    }
}

void read_mac_run(void)
{
    printf("\r\n--- READ MAC ---\r\n");

    // Initialize I2C controller
    board_init_i2c(HPM_I2C0);

    // Read MAC addresses from EEPROM
    read_mac_i2c();
}

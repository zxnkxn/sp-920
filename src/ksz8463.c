/*
 * KSZ8463FRLI register access and SPI bring-up diagnostics for SP-920.
 *
 * SP-920 has two KSZ8463 devices connected to HPM6754 SPI0. The SPI bus is
 * shared; each device has a separate GPIO-controlled chip select.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "ksz8463.h"

#include "hpm_gpio_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_spi_drv.h"

#define KSZ_CR4_DISABLE_TX_BIT       (1U << 14)//
#define KSZ_CR4_DISABLE_FEF_BIT      (1U << 12)//

#define KSZ_SPI_SCLK_HZ              (1000000U)
#define KSZ_DWORD_SIZE               (4U)

/* Set to 1 only after read/write register access is fully verified. */
#define KSZ_ENABLE_FIBER_CONFIG      (1U)

/* P1CR4 / P2CR4 bits */
#define KSZ_CR4_AUTONEG_EN_BIT       (1U << 7)
#define KSZ_CR4_FORCE_SPEED_BIT      (1U << 6)
#define KSZ_CR4_FORCE_DUPLEX_BIT     (1U << 5)

/* P1SR / P2SR bits */
#define KSZ_SR_SPEED_100_BIT         (1U << 10)
#define KSZ_SR_DUPLEX_FULL_BIT       (1U << 9)
#define KSZ_SR_LINK_UP_BIT           (1U << 5)

/* CFGR bits for fiber mode */
#define KSZ_CFGR_FIBER_PORT1_BIT     (1U << 6)
#define KSZ_CFGR_FIBER_PORT2_BIT     (1U << 7)

/* DSP_CTRL_6 bit that must be cleared for fiber mode */
#define KSZ_DSP_CTRL6_FIBER_DIS_BIT  (1U << 13)

static SPI_Type *const s_ksz_spi = BOARD_KSZ_SPI_BASE;

static void ksz_select(uint8_t dev_idx)
{
    if (dev_idx == 0U) {
        gpio_write_pin(BOARD_KSZ0_CS_GPIO,
                       BOARD_KSZ0_CS_GPIO_INDEX,
                       BOARD_KSZ0_CS_GPIO_PIN,
                       0);
    } else {
        gpio_write_pin(BOARD_KSZ1_CS_GPIO,
                       BOARD_KSZ1_CS_GPIO_INDEX,
                       BOARD_KSZ1_CS_GPIO_PIN,
                       0);
    }
}

static void ksz_deselect(uint8_t dev_idx)
{
    if (dev_idx == 0U) {
        gpio_write_pin(BOARD_KSZ0_CS_GPIO,
                       BOARD_KSZ0_CS_GPIO_INDEX,
                       BOARD_KSZ0_CS_GPIO_PIN,
                       1);
    } else {
        gpio_write_pin(BOARD_KSZ1_CS_GPIO,
                       BOARD_KSZ1_CS_GPIO_INDEX,
                       BOARD_KSZ1_CS_GPIO_PIN,
                       1);
    }
}

static void ksz_build_spi_cmd(uint8_t is_write, uint16_t reg, uint8_t byte_enable, uint8_t cmd[2])
{
    uint16_t value;
    uint16_t dword_addr = (uint16_t)(reg >> 2);

    /*
     * KSZ8463 SPI command format:
     * [15]    = opcode: 0 - read, 1 - write
     * [14:6]  = register address A[10:2]
     * [5:2]   = byte enable B[3:0]
     * [1:0]   = don't care / turnaround bits
     */
    value = (uint16_t)(((uint16_t)is_write << 15) |
                       ((dword_addr & 0x01FFU) << 6) |
                       ((uint16_t)(byte_enable & 0x0FU) << 2));

    cmd[0] = (uint8_t)(value >> 8);
    cmd[1] = (uint8_t)(value & 0xFFU);
}

static hpm_stat_t ksz_spi_transaction(uint8_t dev_idx,
                                      uint8_t *cmd,
                                      uint8_t *tx_data,
                                      uint32_t tx_len,
                                      uint8_t *rx_data,
                                      uint32_t rx_len)
{
    hpm_stat_t status;
    spi_control_config_t control_config;
    uint8_t tx_buf[8] = {0};
    uint8_t rx_buf[8] = {0};
    uint32_t total_len;

    if ((cmd == NULL) || (dev_idx > 1U) || ((2U + tx_len + rx_len) > sizeof(tx_buf))) {
        return status_invalid_argument;
    }

    spi_master_get_default_control_config(&control_config);

    /*
     * The KSZ8463 command is transmitted as two ordinary SPI data bytes.
     * This avoids HPM SPI command-phase length/alignment issues and matches
     * the verified SP-920 bring-up behavior.
     */
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.master_config.token_enable = false;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = (spi_dummy_count_t)0;
    control_config.common_config.trans_mode = spi_trans_write_read_together;

    tx_buf[0] = cmd[0];
    tx_buf[1] = cmd[1];

    if ((tx_data != NULL) && (tx_len > 0U)) {
        memcpy(&tx_buf[2], tx_data, tx_len);
    }

    total_len = 2U + tx_len + rx_len;

    ksz_select(dev_idx);
    status = spi_transfer(s_ksz_spi,
                          &control_config,
                          NULL,
                          NULL,
                          tx_buf,
                          total_len,
                          rx_buf,
                          total_len);
    ksz_deselect(dev_idx);

    if (status != status_success) {
        return status;
    }

    if ((rx_data != NULL) && (rx_len > 0U)) {
        memcpy(rx_data, &rx_buf[2U + tx_len], rx_len);
    }

    return status_success;
}

static void ksz_bb_init_pins(void)
{
    uint32_t out_pad_ctl = IOC_PAD_PAD_CTL_DS_SET(6) |
                           IOC_PAD_PAD_CTL_PE_SET(1) |
                           0x08;

    uint32_t in_pad_ctl = IOC_PAD_PAD_CTL_DS_SET(6) |
                          IOC_PAD_PAD_CTL_PE_SET(1) |
                          0x08 |
                          IOC_PAD_PAD_CTL_PS_SET(1);

    HPM_IOC->PAD[IOC_PAD_PZ03].FUNC_CTL = IOC_PZ03_FUNC_CTL_GPIO_Z_03;
    HPM_BIOC->PAD[IOC_PAD_PZ03].FUNC_CTL = BIOC_PZ03_FUNC_CTL_SOC_PZ_03;
    HPM_IOC->PAD[IOC_PAD_PZ03].PAD_CTL = out_pad_ctl;

    HPM_IOC->PAD[IOC_PAD_PZ04].FUNC_CTL = IOC_PZ04_FUNC_CTL_GPIO_Z_04;
    HPM_BIOC->PAD[IOC_PAD_PZ04].FUNC_CTL = BIOC_PZ04_FUNC_CTL_SOC_PZ_04;
    HPM_IOC->PAD[IOC_PAD_PZ04].PAD_CTL = out_pad_ctl;

    HPM_IOC->PAD[IOC_PAD_PZ05].FUNC_CTL = IOC_PZ05_FUNC_CTL_GPIO_Z_05;
    HPM_BIOC->PAD[IOC_PAD_PZ05].FUNC_CTL = BIOC_PZ05_FUNC_CTL_SOC_PZ_05;
    HPM_IOC->PAD[IOC_PAD_PZ05].PAD_CTL = in_pad_ctl;

    gpio_set_pin_output_with_initial(HPM_GPIO0, GPIO_DI_GPIOZ, 3U, 0);
    gpio_set_pin_output_with_initial(HPM_GPIO0, GPIO_DI_GPIOZ, 4U, 0);
    gpio_set_pin_input(HPM_GPIO0, GPIO_DI_GPIOZ, 5U);
}

static void ksz_bb_delay(void)
{
    board_delay_us(2);
}

static uint8_t ksz_bb_transfer_byte(uint8_t out)
{
    uint8_t in = 0;

    for (int bit = 7; bit >= 0; bit--) {
        gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOZ, 4U, (out >> bit) & 1U);
        ksz_bb_delay();

        gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOZ, 3U, 1);
        ksz_bb_delay();

        in <<= 1;
        if (gpio_read_pin(HPM_GPIO0, GPIO_DI_GPIOZ, 5U)) {
            in |= 1U;
        }

        gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOZ, 3U, 0);
        ksz_bb_delay();
    }

    return in;
}

static void ksz_bb_write16(uint8_t dev_idx, uint16_t reg, uint16_t value)
{
    uint8_t cmd[2];
    uint8_t be = ((reg & 0x2U) == 0U) ? 0x03U : 0x0CU;

    ksz_build_spi_cmd(1U, reg, be, cmd);

    ksz_select(dev_idx);
    ksz_bb_transfer_byte(cmd[0]);
    ksz_bb_transfer_byte(cmd[1]);
    ksz_bb_transfer_byte((uint8_t)(value & 0xFFU));
    ksz_bb_transfer_byte((uint8_t)((value >> 8) & 0xFFU));
    ksz_deselect(dev_idx);
}

static uint16_t ksz_bb_read16(uint8_t dev_idx, uint16_t reg)
{
    uint8_t cmd[2];
    uint8_t be = ((reg & 0x2U) == 0U) ? 0x03U : 0x0CU;
    uint8_t lo;
    uint8_t hi;

    ksz_build_spi_cmd(0U, reg, be, cmd);

    ksz_select(dev_idx);
    ksz_bb_transfer_byte(cmd[0]);
    ksz_bb_transfer_byte(cmd[1]);
    lo = ksz_bb_transfer_byte(0x00U);
    hi = ksz_bb_transfer_byte(0x00U);
    ksz_deselect(dev_idx);

    return (uint16_t)(lo | ((uint16_t)hi << 8));
}

static void ksz8463_debug_bitbang_test(uint8_t dev_idx)
{
    uint16_t sgcr1_before;
    uint16_t sgcr1_after;
    uint16_t p1cr1_before;
    uint16_t p1cr1_after;

    printf("\r\nKSZ%u BITBANG SPI TEST\r\n", dev_idx);

    ksz_bb_init_pins();

    sgcr1_before = ksz_bb_read16(dev_idx, 0x0002U);
    p1cr1_before = ksz_bb_read16(dev_idx, 0x006CU);

    printf("BB before: SGCR1=0x%04X P1CR1=0x%04X\r\n",
           sgcr1_before, p1cr1_before);

    ksz_bb_write16(dev_idx, 0x0002U, 0x3451U);
    board_delay_ms(10);

    sgcr1_after = ksz_bb_read16(dev_idx, 0x0002U);

    ksz_bb_write16(dev_idx, 0x006CU, 0x1234U);
    board_delay_ms(10);

    p1cr1_after = ksz_bb_read16(dev_idx, 0x006CU);

    printf("BB after:  SGCR1=0x%04X P1CR1=0x%04X\r\n",
           sgcr1_after, p1cr1_after);
}

static hpm_stat_t ksz_read_dword(uint8_t dev_idx, uint16_t reg, uint8_t data[KSZ_DWORD_SIZE])
{
    uint8_t cmd[2];
    uint16_t dword_base = (uint16_t)(reg & ~0x3U);

    if ((data == NULL) || (dev_idx > 1U)) {
        return status_invalid_argument;
    }

    ksz_build_spi_cmd(0U, dword_base, 0x0FU, cmd);

    if (ksz_spi_transaction(dev_idx, cmd, NULL, 0U, data, KSZ_DWORD_SIZE) != status_success) {
        return status_fail;
    }

    return status_success;
}

static void ksz_debug_print_cmd_data(const char *name,
                                     uint8_t cmd0,
                                     uint8_t cmd1,
                                     const uint8_t *data,
                                     uint32_t data_len)
{
    printf("%s: CMD=%02X %02X DATA=", name, cmd0, cmd1);

    for (uint32_t i = 0; i < data_len; i++) {
        printf("%02X ", data[i]);
    }

    printf("\r\n");
}

static hpm_stat_t ksz_debug_raw_write(uint8_t dev_idx,
                                      const char *name,
                                      uint8_t cmd0,
                                      uint8_t cmd1,
                                      const uint8_t *data,
                                      uint32_t data_len)
{
    uint8_t cmd[2];
    uint8_t tx_data[4] = {0};
    uint16_t p1cr4 = 0;

    if ((dev_idx > 1U) || (data_len > sizeof(tx_data))) {
        return status_invalid_argument;
    }

    cmd[0] = cmd0;
    cmd[1] = cmd1;

    for (uint32_t i = 0; i < data_len; i++) {
        tx_data[i] = data[i];
    }

    ksz_debug_print_cmd_data(name, cmd[0], cmd[1], tx_data, data_len);

    if (ksz_spi_transaction(dev_idx,
                            cmd,
                            tx_data,
                            data_len,
                            NULL,
                            0U) != status_success) {
        printf("%s: write transaction failed\r\n", name);
        return status_fail;
    }

    board_delay_ms(10);

    if (ksz8463_read16(dev_idx, KSZ_REG_P1CR4, &p1cr4) != status_success) {
        printf("%s: readback failed\r\n", name);
        return status_fail;
    }

    printf("%s: P1CR4 readback = 0x%04X\r\n\r\n", name, p1cr4);

    return status_success;
}

static void ksz8463_debug_write_matrix(uint8_t dev_idx)
{
    uint8_t cmd[2];
    uint16_t before = 0;
    uint16_t after = 0;

    uint8_t data_2_ff00[2] = {0xFFU, 0x00U};
    uint8_t data_2_00ff[2] = {0x00U, 0xFFU};

    uint8_t data_4_offset23_ff00[4] = {0x00U, 0x00U, 0xFFU, 0x00U};
    uint8_t data_4_offset23_00ff[4] = {0x00U, 0x00U, 0x00U, 0xFFU};

    uint8_t data_4_offset01_ff00[4] = {0xFFU, 0x00U, 0x00U, 0x00U};
    uint8_t data_4_offset01_00ff[4] = {0x00U, 0xFFU, 0x00U, 0x00U};

    printf("\r\n");
    printf("========================================\r\n");
    printf("KSZ%u SPI WRITE MATRIX TEST\r\n", dev_idx);
    printf("Target register: P1CR4 = 0x%04X\r\n", KSZ_REG_P1CR4);
    printf("Expected target value for success: 0x00FF\r\n");
    printf("========================================\r\n");

    ksz8463_read16(dev_idx, KSZ_REG_P1CR4, &before);
    printf("Before matrix: P1CR4 = 0x%04X\r\n\r\n", before);

    /*
     * Variant A: current logical variant for register offset +2:
     * BE = 0x0C, data phase = 2 bytes, value = FF 00.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x0CU, cmd);
    ksz_debug_raw_write(dev_idx, "A: normal cmd, BE=0C, len=2, FF 00",
                        cmd[0], cmd[1], data_2_ff00, sizeof(data_2_ff00));

    /*
     * Variant B: same BE, but reversed value byte order.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x0CU, cmd);
    ksz_debug_raw_write(dev_idx, "B: normal cmd, BE=0C, len=2, 00 FF",
                        cmd[0], cmd[1], data_2_00ff, sizeof(data_2_00ff));

    /*
     * Variant C: same command, but send full 4-byte data phase,
     * with useful bytes at offsets 2 and 3.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x0CU, cmd);
    ksz_debug_raw_write(dev_idx, "C: normal cmd, BE=0C, len=4, offset23 FF 00",
                        cmd[0], cmd[1], data_4_offset23_ff00, sizeof(data_4_offset23_ff00));

    /*
     * Variant D: full 4-byte data phase, reversed value at offsets 2 and 3.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x0CU, cmd);
    ksz_debug_raw_write(dev_idx, "D: normal cmd, BE=0C, len=4, offset23 00 FF",
                        cmd[0], cmd[1], data_4_offset23_00ff, sizeof(data_4_offset23_00ff));

    /*
     * Variant E: test whether BE bit numbering is interpreted opposite.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x03U, cmd);
    ksz_debug_raw_write(dev_idx, "E: normal cmd, BE=03, len=2, FF 00",
                        cmd[0], cmd[1], data_2_ff00, sizeof(data_2_ff00));

    /*
     * Variant F: BE=03, full 4-byte phase, useful bytes at offsets 0 and 1.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x03U, cmd);
    ksz_debug_raw_write(dev_idx, "F: normal cmd, BE=03, len=4, offset01 FF 00",
                        cmd[0], cmd[1], data_4_offset01_ff00, sizeof(data_4_offset01_ff00));

    /*
     * Variant G: same as A, but command bytes swapped.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x0CU, cmd);
    ksz_debug_raw_write(dev_idx, "G: swapped cmd, BE=0C, len=2, FF 00",
                        cmd[1], cmd[0], data_2_ff00, sizeof(data_2_ff00));

    /*
     * Variant H: swapped command bytes, BE=03.
     */
    ksz_build_spi_cmd(1U, KSZ_REG_P1CR4, 0x03U, cmd);
    ksz_debug_raw_write(dev_idx, "H: swapped cmd, BE=03, len=2, FF 00",
                        cmd[1], cmd[0], data_2_ff00, sizeof(data_2_ff00));

    // Variant I
    uint8_t data_4_first_ff00[4] = {0xFFU, 0x00U, 0x00U, 0x00U};

    ksz_debug_raw_write(dev_idx, "I: colleague-like cmd, CMD=87 FC, len=4, FF 00 00 00",
                        0x87U, 0xFCU, data_4_first_ff00, sizeof(data_4_first_ff00));

    // Variant J
    uint8_t sgcr1_data_2[2] = {0x51U, 0x34U};

    ksz_debug_raw_write(dev_idx,
                        "J: SGCR1 write, reg=0002, BE=0C, len=2, 51 34",
                        0x80U, 0xB0U,
                        sgcr1_data_2,
                        sizeof(sgcr1_data_2));

    uint16_t sgcr1_after = 0;
    ksz8463_read16(dev_idx, 0x0002U, &sgcr1_after);
    printf("J: SGCR1 final readback = 0x%04X\r\n\r\n", sgcr1_after);

    // Variant K: test write to another Port 1 RW register, P1CR1 = 0x006C
    uint8_t p1cr1_data_2[2] = {0x34U, 0x12U};

    ksz_debug_raw_write(dev_idx,
                        "K: P1CR1 write, reg=006C, BE=03, len=2, 34 12",
                        0x86U, 0xC4U,
                        p1cr1_data_2,
                        sizeof(p1cr1_data_2));

    uint16_t p1cr1_after = 0;
    ksz8463_read16(dev_idx, 0x006CU, &p1cr1_after);
    printf("K: P1CR1 final readback = 0x%04X\r\n\r\n", p1cr1_after);

    // Variant L: test active ksz8463_write16()
    printf("L: active ksz8463_write16 test\r\n");

    ksz8463_write16(dev_idx, 0x006CU, 0x1234U);
    board_delay_ms(10);

    uint16_t p1cr1_after_l = 0;
    ksz8463_read16(dev_idx, 0x006CU, &p1cr1_after_l);
    printf("L: P1CR1 after ksz8463_write16 = 0x%04X\r\n\r\n", p1cr1_after_l);

    ksz8463_read16(dev_idx, KSZ_REG_P1CR4, &after);

    printf("========================================\r\n");
    printf("KSZ%u MATRIX FINISHED\r\n", dev_idx);
    printf("Before: 0x%04X, After: 0x%04X\r\n", before, after);
    printf("========================================\r\n\r\n");
}

static hpm_stat_t ksz_modify16(uint8_t dev_idx, uint16_t reg, uint16_t clear_mask, uint16_t set_mask)
{
    hpm_stat_t status;
    uint16_t value;

    status = ksz8463_read16(dev_idx, reg, &value);
    if (status != status_success) {
        return status;
    }

    value &= (uint16_t)(~clear_mask);
    value |= set_mask;

    return ksz8463_write16(dev_idx, reg, value);
}

static const char *ksz_link_to_str(uint16_t port_status)
{
    return ((port_status & KSZ_SR_LINK_UP_BIT) != 0U) ? "up" : "down";
}

static const char *ksz_speed_to_str(uint16_t port_status)
{
    if ((port_status & KSZ_SR_LINK_UP_BIT) == 0U) {
        return "unknown";
    }

    return ((port_status & KSZ_SR_SPEED_100_BIT) != 0U) ? "100M" : "10M";
}

static const char *ksz_duplex_to_str(uint16_t port_status)
{
    if ((port_status & KSZ_SR_LINK_UP_BIT) == 0U) {
        return "unknown";
    }

    return ((port_status & KSZ_SR_DUPLEX_FULL_BIT) != 0U) ? "full" : "half";
}

hpm_stat_t ksz8463_spi_init(void)
{
    spi_timing_config_t timing_config;
    spi_format_config_t format_config;

    clock_add_to_group(BOARD_KSZ_SPI_CLK_NAME, 0);

    gpio_set_pin_output_with_initial(BOARD_KSZ0_CS_GPIO,
                                     BOARD_KSZ0_CS_GPIO_INDEX,
                                     BOARD_KSZ0_CS_GPIO_PIN,
                                     1);

    gpio_set_pin_output_with_initial(BOARD_KSZ1_CS_GPIO,
                                     BOARD_KSZ1_CS_GPIO_INDEX,
                                     BOARD_KSZ1_CS_GPIO_PIN,
                                     1);

    spi_reset(s_ksz_spi);
    spi_transmit_fifo_reset(s_ksz_spi);
    spi_receive_fifo_reset(s_ksz_spi);

    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.clk_src_freq_in_hz = clock_get_frequency(BOARD_KSZ_SPI_CLK_NAME);
    timing_config.master_config.sclk_freq_in_hz = KSZ_SPI_SCLK_HZ;
    timing_config.master_config.cs2sclk = spi_cs2sclk_half_sclk_2;
    timing_config.master_config.csht = spi_csht_half_sclk_2;

    if (spi_master_timing_init(s_ksz_spi, &timing_config) != status_success) {
        return status_fail;
    }

    spi_master_get_default_format_config(&format_config);
    format_config.common_config.data_len_in_bits = 8;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = false;
    format_config.common_config.lsb = spi_msb_first;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_low_idle;
    format_config.common_config.cpha = spi_sclk_sampling_odd_clk_edges;

    spi_format_init(s_ksz_spi, &format_config);

    return status_success;
}

hpm_stat_t ksz8463_read8(uint8_t dev_idx, uint16_t reg, uint8_t *value)
{
    uint8_t cmd[2];
    uint8_t rx_data[1] = {0};
    uint8_t byte_enable;

    if ((dev_idx > 1U) || (value == NULL)) {
        return status_invalid_argument;
    }

    byte_enable = (uint8_t)(1U << (reg & 0x3U));

    ksz_build_spi_cmd(0U, reg, byte_enable, cmd);

    if (ksz_spi_transaction(dev_idx,
                            cmd,
                            NULL,
                            0U,
                            rx_data,
                            sizeof(rx_data)) != status_success) {
        return status_fail;
    }

    *value = rx_data[0];
    return status_success;
}

hpm_stat_t ksz8463_read16(uint8_t dev_idx, uint16_t reg, uint16_t *value)
{
    uint8_t cmd[2];
    uint8_t rx_data[2] = {0};
    uint8_t byte_enable;

    if ((dev_idx > 1U) || (value == NULL) || ((reg & 0x1U) != 0U)) {
        return status_invalid_argument;
    }

    byte_enable = ((reg & 0x2U) == 0U) ? 0x03U : 0x0CU;

    ksz_build_spi_cmd(0U, reg, byte_enable, cmd);

    if (ksz_spi_transaction(dev_idx,
                            cmd,
                            NULL,
                            0U,
                            rx_data,
                            sizeof(rx_data)) != status_success) {
        return status_fail;
    }

    *value = (uint16_t)((uint16_t)rx_data[0] |
                       ((uint16_t)rx_data[1] << 8));

    return status_success;
}

hpm_stat_t ksz8463_write8(uint8_t dev_idx, uint16_t reg, uint8_t value)
{
    uint8_t cmd[2];
    uint8_t tx_data[4] = {0};
    uint8_t byte_offset;
    uint8_t byte_enable;

    if (dev_idx > 1U) {
        return status_invalid_argument;
    }

    byte_offset = (uint8_t)(reg & 0x3U);
    byte_enable = (uint8_t)(1U << byte_offset);

    tx_data[byte_offset] = value;

    ksz_build_spi_cmd(1U, reg, byte_enable, cmd);

    if (ksz_spi_transaction(dev_idx,
                            cmd,
                            tx_data,
                            sizeof(tx_data),
                            NULL,
                            0U) != status_success) {
        return status_fail;
    }

    return status_success;
}

hpm_stat_t ksz8463_write16(uint8_t dev_idx, uint16_t reg, uint16_t value)
{
    uint8_t cmd[2];
    uint8_t tx_data[4] = {0};

    if ((dev_idx > 1U) || ((reg & 0x1U) != 0U)) {
        return status_invalid_argument;
    }

    /*
     * KSZ8463 expects DWORD-aligned write payload.
     * Data bytes must be placed into positions selected
     * by byte enable bits.
     */

    switch (reg & 0x3U) {

    case 0x0U:
        tx_data[0] = (uint8_t)(value & 0xFFU);
        tx_data[1] = (uint8_t)((value >> 8) & 0xFFU);

        ksz_build_spi_cmd(1U, reg, 0x03U, cmd);
        break;

    case 0x2U:
        tx_data[0] = (uint8_t)(value & 0xFFU);
        tx_data[1] = (uint8_t)((value >> 8) & 0xFFU);

        ksz_build_spi_cmd(1U, reg, 0x0CU, cmd);
        break;

    default:
        return status_invalid_argument;
    }

    if (ksz_spi_transaction(dev_idx,
                            cmd,
                            tx_data,
                            sizeof(tx_data),
                            NULL,
                            0U) != status_success) {
        return status_fail;
    }

    return status_success;
}

hpm_stat_t ksz8463_get_link_status(uint8_t dev_idx, bool *link_up)
{
    uint16_t p1sr;
    uint16_t p2sr;

    if ((link_up == NULL) || (dev_idx > 1U)) {
        return status_invalid_argument;
    }

    if ((ksz8463_read16(dev_idx, KSZ_REG_P1SR, &p1sr) != status_success) ||
        (ksz8463_read16(dev_idx, KSZ_REG_P2SR, &p2sr) != status_success)) {
        *link_up = false;
        return status_fail;
    }

    *link_up = (((p1sr | p2sr) & KSZ_SR_LINK_UP_BIT) != 0U);
    return status_success;
}

void ksz8463_dump_basic_info(uint8_t dev_idx)
{
    uint16_t cider = 0U;
    uint16_t sgcr1 = 0U;
    uint16_t p1cr4 = 0U;
    uint16_t p2cr4 = 0U;
    uint16_t p1sr = 0U;
    uint16_t p2sr = 0U;
    uint16_t p1scslmd = 0U;
    uint16_t p2scslmd = 0U;
    uint16_t p1phyctrl = 0U;
    uint16_t p2phyctrl = 0U;
    uint16_t dsp6 = 0U;
    uint16_t cfgr = 0U;

    ksz8463_read16(dev_idx, 0x0000U, &cider);
    ksz8463_read16(dev_idx, 0x0002U, &sgcr1);
    ksz8463_read16(dev_idx, KSZ_REG_P1CR4, &p1cr4);
    ksz8463_read16(dev_idx, KSZ_REG_P2CR4, &p2cr4);
    ksz8463_read16(dev_idx, KSZ_REG_P1SR, &p1sr);
    ksz8463_read16(dev_idx, KSZ_REG_P2SR, &p2sr);
    ksz8463_read16(dev_idx, KSZ_REG_P1SCSLMD, &p1scslmd);
    ksz8463_read16(dev_idx, KSZ_REG_P2SCSLMD, &p2scslmd);
    ksz8463_read16(dev_idx, KSZ_REG_P1PHYCTRL, &p1phyctrl);
    ksz8463_read16(dev_idx, KSZ_REG_P2PHYCTRL, &p2phyctrl);
    ksz8463_read16(dev_idx, KSZ_REG_DSP_CTRL_6, &dsp6);
    ksz8463_read16(dev_idx, KSZ_REG_CFGR, &cfgr);

    printf("KSZ%u CIDER=0x%04X SGCR1=0x%04X CFGR=0x%04X DSP6=0x%04X\r\n",
           dev_idx, cider, sgcr1, cfgr, dsp6);

    printf("KSZ%u P1CR4=0x%04X P1SR=0x%04X P1SCSLMD=0x%04X P1PHYCTRL=0x%04X\r\n",
           dev_idx, p1cr4, p1sr, p1scslmd, p1phyctrl);

    printf("KSZ%u P2CR4=0x%04X P2SR=0x%04X P2SCSLMD=0x%04X P2PHYCTRL=0x%04X\r\n",
           dev_idx, p2cr4, p2sr, p2scslmd, p2phyctrl);
    
    printf("KSZ%u P1SR link=%u speed100=%u full=%u\r\n",
           dev_idx,
           (p1sr & KSZ_SR_LINK_UP_BIT) ? 1U : 0U,
           (p1sr & KSZ_SR_SPEED_100_BIT) ? 1U : 0U,
           (p1sr & KSZ_SR_DUPLEX_FULL_BIT) ? 1U : 0U);

    printf("KSZ%u P2SR link=%u speed100=%u full=%u\r\n",
           dev_idx,
           (p2sr & KSZ_SR_LINK_UP_BIT) ? 1U : 0U,
           (p2sr & KSZ_SR_SPEED_100_BIT) ? 1U : 0U,
           (p2sr & KSZ_SR_DUPLEX_FULL_BIT) ? 1U : 0U);
}

hpm_stat_t ksz8463_init_device(uint8_t dev_idx)
{
    uint16_t cfgr = 0U;
    uint16_t dsp6 = 0U;
    uint16_t p1cr4 = 0U;
    uint16_t p2cr4 = 0U;

    if (dev_idx > 1U) {
        return status_invalid_argument;
    }

    if (ksz8463_read16(dev_idx, KSZ_REG_CFGR, &cfgr) != status_success) {
        return status_fail;
    }

    cfgr &= (uint16_t)~(KSZ_CFGR_FIBER_PORT1_BIT | KSZ_CFGR_FIBER_PORT2_BIT);

    if (ksz8463_write16(dev_idx, KSZ_REG_CFGR, cfgr) != status_success) {
        return status_fail;
    }

    if (ksz8463_read16(dev_idx, KSZ_REG_DSP_CTRL_6, &dsp6) != status_success) {
        return status_fail;
    }

    dsp6 &= (uint16_t)~KSZ_DSP_CTRL6_FIBER_DIS_BIT;

    if (ksz8463_write16(dev_idx, KSZ_REG_DSP_CTRL_6, dsp6) != status_success) {
        return status_fail;
    }

    /*
     * P1CR4 / P2CR4:
     * bit 12 = disable FEF
     * bit 6  = force 100M
     * bit 5  = force full-duplex
     */
    if (ksz8463_write16(dev_idx, KSZ_REG_P1CR4, 0x1060U) != status_success) {
        return status_fail;
    }

    if (ksz8463_write16(dev_idx, KSZ_REG_P2CR4, 0x1060U) != status_success) {
        return status_fail;
    }

    ksz8463_read16(dev_idx, KSZ_REG_CFGR, &cfgr);
    ksz8463_read16(dev_idx, KSZ_REG_DSP_CTRL_6, &dsp6);
    ksz8463_read16(dev_idx, KSZ_REG_P1CR4, &p1cr4);
    ksz8463_read16(dev_idx, KSZ_REG_P2CR4, &p2cr4);

    //printf("KSZ%u CFGR after  = 0x%04X\r\n", dev_idx, cfgr);
    //printf("KSZ%u DSP6=0x%04X P1CR4=0x%04X P2CR4=0x%04X\r\n",
    //       dev_idx, dsp6, p1cr4, p2cr4);

    return status_success;
}

hpm_stat_t ksz8463_bringup_test(void)
{
    hpm_stat_t status;

    board_init();

    printf("\r\n========================================\r\n");
    printf("SP-920 KSZ8463 SPI / optical link test\r\n");
    printf("========================================\r\n");
    
    /*
     * Important order:
     * 1. Configure ENET/RMII pins.
     * 2. Configure SPI pins and KSZ control pins.
     *    board_init_ksz_ctrl_pins() must keep both KSZ devices in reset low.
     * 3. Configure ENET RMII reference clocks.
     * 4. Release/reset KSZ devices.
     * 5. Initialize SPI master and access KSZ registers.
     */
    status = board_init_multiple_enet_pins();
    if (status != status_success) {
        printf("board_init_multiple_enet_pins failed\r\n");
        return status;
    }

    status = board_init_multiple_enet_clock();
    if (status != status_success) {
        printf("board_init_multiple_enet_clock failed\r\n");
        return status;
    }

    status = board_reset_multiple_enet_phy();
    if (status != status_success) {
        printf("board_reset_multiple_enet_phy failed\r\n");
        return status;
    }

    board_delay_ms(100);

    status = ksz8463_spi_init();
    if (status != status_success) {
        printf("KSZ8463 SPI init failed\r\n");
        return status;
    }

    printf("KSZ8463 SPI init passed\r\n\r\n");

    printf("Raw KSZ read test before configuration:\r\n\r\n");

    for (uint8_t dev = 0; dev < 2U; dev++) {
        uint16_t cider = 0;
        uint16_t sgcr1 = 0;
        uint16_t p1cr4 = 0;
        uint16_t p1sr = 0;
        uint16_t p2cr4 = 0;
        uint16_t p2sr = 0;
        uint16_t cfgr = 0;

        printf("---- KSZ%u raw read ----\r\n", dev);

        ksz8463_read16(dev, 0x0000U, &cider);
        ksz8463_read16(dev, 0x0002U, &sgcr1);
        ksz8463_read16(dev, KSZ_REG_P1CR4, &p1cr4);
        ksz8463_read16(dev, KSZ_REG_P1SR, &p1sr);
        ksz8463_read16(dev, KSZ_REG_P2CR4, &p2cr4);
        ksz8463_read16(dev, KSZ_REG_P2SR, &p2sr);
        ksz8463_read16(dev, KSZ_REG_CFGR, &cfgr);

        printf("CIDER=0x%04X SGCR1=0x%04X CFGR=0x%04X\r\n", cider, sgcr1, cfgr);
        printf("P1CR4=0x%04X P1SR=0x%04X P2CR4=0x%04X P2SR=0x%04X\r\n\r\n",
               p1cr4, p1sr, p2cr4, p2sr);
    }

    for (uint8_t dev = 0; dev < 2U; dev++) {
        printf("---- KSZ%u init ----\r\n", dev);

        status = ksz8463_init_device(dev);
        if (status != status_success) {
            printf("KSZ%u init failed\r\n", dev);
            return status;
        }

        ksz8463_dump_basic_info(dev);
        printf("\r\n");
    }

    printf("Periodic status dump follows.\r\n");
    printf("Connect optical link partner and watch P1SR/P2SR changes.\r\n\r\n");

    while (1) {
        for (uint8_t dev = 0; dev < 2U; dev++) {
            ksz8463_dump_basic_info(dev);
        }

        printf("----\r\n");
        board_delay_ms(1000);
    }
}



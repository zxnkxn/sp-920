/*
 * KSZ8463FRLI register access and SPI bring-up diagnostics for SP-920.
 */

#ifndef KSZ8463_H
#define KSZ8463_H

#include <stdint.h>
#include <stdbool.h>
#include "hpm_common.h"

typedef enum {
    ksz_dev_0 = 0,
    ksz_dev_1 = 1
} ksz_dev_t;

typedef enum {
    ksz_spi_read = 0,
    ksz_spi_write = 1
} ksz_spi_op_t;

/* KSZ8463 register addresses used during SPI/optical-channel bring-up. */
#define KSZ_REG_CHIP_ID0         (0x0002U)
#define KSZ_REG_CHIP_ID1         (0x0003U)
#define KSZ_REG_P1CR4            (0x007EU)
#define KSZ_REG_P1SR             (0x0080U)
#define KSZ_REG_P2CR4            (0x0096U)
#define KSZ_REG_P2SR             (0x0098U)
#define KSZ_REG_CFGR             (0x00D8U)
#define KSZ_REG_DSP_CTRL_6       (0x0734U)

#define KSZ_REG_P1SCSLMD         (0x007CU)//
#define KSZ_REG_P2SCSLMD         (0x0094U)//
#define KSZ_REG_P1PHYCTRL        (0x0066U)//
#define KSZ_REG_P2PHYCTRL        (0x006AU)//

hpm_stat_t ksz8463_spi_init(void);
hpm_stat_t ksz8463_init_device(uint8_t dev_idx);

hpm_stat_t ksz8463_read8(uint8_t dev_idx, uint16_t reg, uint8_t *value);
hpm_stat_t ksz8463_read16(uint8_t dev_idx, uint16_t reg, uint16_t *value);
hpm_stat_t ksz8463_write8(uint8_t dev_idx, uint16_t reg, uint8_t value);
hpm_stat_t ksz8463_write16(uint8_t dev_idx, uint16_t reg, uint16_t value);

hpm_stat_t ksz8463_get_link_status(uint8_t dev_idx, bool *link_up);

void ksz8463_dump_basic_info(uint8_t dev_idx);
hpm_stat_t ksz8463_bringup_test(void);

#endif /* KSZ8463_H */

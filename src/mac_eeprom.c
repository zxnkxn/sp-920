#include <string.h>

#include "board.h"
#include "hpm_i2c_drv.h"

#include "mac_eeprom.h"

static const uint8_t mac_eeprom_i2c_addr[SP920_MAC_EEPROM_COUNT] = {
    0x50,
    0x52
};

static const uint8_t mac_eeprom_mem_addr = 0xFA;

hpm_stat_t sp920_read_mac_from_eeprom(uint8_t idx, uint8_t mac[SP920_MAC_LEN])
{
    uint8_t mem_addr = mac_eeprom_mem_addr;

    if ((idx >= SP920_MAC_EEPROM_COUNT) || (mac == NULL)) {
        return status_invalid_argument;
    }

    memset(mac, 0, SP920_MAC_LEN);

    return i2c_master_address_read(HPM_I2C0,
                                   mac_eeprom_i2c_addr[idx],
                                   &mem_addr,
                                   1,
                                   mac,
                                   SP920_MAC_LEN);
}

#ifndef MAC_EEPROM_H
#define MAC_EEPROM_H

#include <stdint.h>
#include "hpm_common.h"

#define SP920_MAC_EEPROM_COUNT  2U
#define SP920_MAC_LEN           6U

hpm_stat_t sp920_read_mac_from_eeprom(uint8_t idx, uint8_t mac[SP920_MAC_LEN]);

#endif

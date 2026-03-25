#include <stdio.h>
#include <stdint.h>
#include "board.h"
#include "march_test.h"
#include "ram_test.h"

static const char* ram_test_name(int t)
{
    switch (t) {
    case MATS_PLUS:     return "MATS+";
    case MARCH_C_MINUS: return "March C-";
    case MARCH_A:       return "March A";
    case MARCH_LA:      return "March LA";
    default:            return "UNKNOWN";
    }
}

static void ram_run_one(int test_type)
{
    volatile uint32_t *begin = (volatile uint32_t *)BOARD_SDRAM_ADDRESS;
    volatile uint32_t *end   = (volatile uint32_t *)(BOARD_SDRAM_ADDRESS + BOARD_SDRAM_SIZE);

    printf("\r\n--- Run %s ---\r\n", ram_test_name(test_type));
    printf("Range: [0x%08lX .. 0x%08lX)\r\n",
           (uint32_t)begin, (uint32_t)end);

    if (end <= begin) {
        printf("FAIL: invalid range (end <= begin)\r\n");
        return;
    }
    if ((((uintptr_t)begin | (uintptr_t)end) & 3u) != 0u) {
        printf("FAIL: addresses not 4-byte aligned\r\n");
        return;
    }

    int rc = march_test(test_type, begin, end);

    if (rc == MEMORY_TEST_OK) {
        printf("PASS\r\n");
    } else {
        printf("FAIL (code=%d)\r\n", rc);
    }
}

void ram_test_run(void)
{
    printf("\r\n--- RUN SDRAM TESTS ---\r\n");

    ram_run_one(MATS_PLUS);
    ram_run_one(MARCH_C_MINUS);
    ram_run_one(MARCH_A);
    ram_run_one(MARCH_LA);

    printf("\r\nSDRAM TESTS COMPLETED\r\n");
}

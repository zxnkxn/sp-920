#ifndef MARCH_TEST_H
#define MARCH_TEST_H

#include <stdint.h>

// Return codes
#define MEMORY_TEST_OK   0  // Memory test passed successfully
#define MEMORY_TEST_ERR  1  // Memory test failed

// Supported March test algorithms
#define MATS_PLUS      0u // MATS+
#define MARCH_C_MINUS  1u // MARCH C-
#define MARCH_A        2u // MARCH A
#define MARCH_LA       3u // MARCH LA

/*
 * Performs a March memory test on the specified address range.
 *
 * The function tests the memory region from begin (inclusive)
 * to end (exclusive) using the selected March algorithm.
 *
 * test_type - type of the test to execute:
 *             MATS_PLUS, MARCH_C_MINUS, MARCH_A or MARCH_LA
 * begin     - start address of the memory range (inclusive)
 * end       - end address of the memory range (exclusive)
 *
 * Returns:
 *   MEMORY_TEST_OK  - no errors were detected
 *   MEMORY_TEST_ERR - a memory error was detected
 */
int march_test(int test_type, volatile uint32_t *begin, volatile uint32_t *end);

#endif // MARCH_TEST_H

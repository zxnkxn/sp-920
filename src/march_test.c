#include <stdint.h>
#include "march_test.h"

#define MARCH_PATTERN0 0x00000000u
#define MARCH_PATTERN1 0xFFFFFFFFu

/* MATS+
 * { up, down(w0);
 *   up(r0, w1);
 *   down(r1, w0) }
 */
static int mats_plus(volatile uint32_t *begin, volatile uint32_t *end)
{
    const uint32_t w0 = MARCH_PATTERN0;
    const uint32_t w1 = MARCH_PATTERN1;

    // Valid range: end must be greater than begin
    if (end <= begin) return MEMORY_TEST_ERR;

    // 32-bit alignment for both addresses
    if ((((uintptr_t)begin | (uintptr_t)end) & 3u) != 0u) return MEMORY_TEST_ERR;

    volatile uint32_t *last = end - 1;

    // 1.1. up(w0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        *p = w0;
    }

    // 1.2. down(w0)
    for (volatile uint32_t *p = last; ; --p) {
        *p = w0;
        if (p == begin) break; // stop before pointer underflow
    }

    // 2. up(r0, w1)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
    }

    // 3. down(r1, w0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
        if (p == begin) break;
    }

    return MEMORY_TEST_OK;
}

/* March C-
 * { up, down(w0);
 *   up(r0, w1);
 *   up(r1, w0);
 *   down(r0, w1);
 *   down(r1, w0);
 *   up, down(r0) }
 */
static int march_c_minus(volatile uint32_t *begin, volatile uint32_t *end)
{
    const uint32_t w0 = MARCH_PATTERN0;
    const uint32_t w1 = MARCH_PATTERN1;

    // Valid range: end must be greater than begin
    if (end <= begin) return MEMORY_TEST_ERR;

    // 32-bit alignment for both addresses
    if ((((uintptr_t)begin | (uintptr_t)end) & 3u) != 0u) return MEMORY_TEST_ERR;

    volatile uint32_t *last = end - 1;

    // 1.1. up(w0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        *p = w0;
    }

    // 1.2. down(w0)
    for (volatile uint32_t *p = last; ; --p) {
        *p = w0;
        if (p == begin) break; // stop before pointer underflow
    }

    // 2. up(r0, w1)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
    }

    // 3. up(r1, w0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
    }

    // 4. down(r0, w1)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
        if (p == begin) break;
    }

    // 5. down (r1, w0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
        if (p == begin) break;
    }

    // 6.1. up(r0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w0) return MEMORY_TEST_ERR;
    }

    // 6.2. down(r0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        if (p == begin) break;
    }

    return MEMORY_TEST_OK;
}

/* March A
 * { up, down(w0);
 *   up(r0,w1,w0,w1);
 *   up(r1,w0,w1);
 *   down(r1,w0,w1,w0);
 *   down(r0,w1,w0) }
 */
static int march_a(volatile uint32_t *begin, volatile uint32_t *end)
{
    const uint32_t w0 = MARCH_PATTERN0;
    const uint32_t w1 = MARCH_PATTERN1;

    // Valid range: end must be greater than begin
    if (end <= begin) return MEMORY_TEST_ERR;

    // 32-bit alignment for both addresses
    if ((((uintptr_t)begin | (uintptr_t)end) & 3u) != 0u) return MEMORY_TEST_ERR;

    volatile uint32_t *last = end - 1;

    // 1.1. up(w0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        *p = w0;
    }

    // 1.2. down(w0)
    for (volatile uint32_t *p = last; ; --p) {
        *p = w0;
        if (p == begin) break; // stop before pointer underflow
    }

    // 2. up(r0,w1,w0,w1)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
        *p = w0;
        *p = w1;
    }

    // 3. up(r1,w0,w1)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
        *p = w1;
    }

    // 4. down(r1,w0,w1,w0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
        *p = w1;
        *p = w0;
        if (p == begin) break;
    }

    // 5. down(r0,w1,w0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
        *p = w0;
        if (p == begin) break;
    }

    return MEMORY_TEST_OK;
}

/* March LA
 * { up, down(w0);
 *   down(r0,w1,w0,w1,r1);
 *   up(r1,w0,w1,w0,r0);
 *   down(r0,w1,w0,w1,r1);
 *   down(r1,w0,w1,w0,r0);
 *   down(r0) }
 */
static int march_la(volatile uint32_t *begin, volatile uint32_t *end)
{
    const uint32_t w0 = MARCH_PATTERN0;
    const uint32_t w1 = MARCH_PATTERN1;

    // Valid range: end must be greater than begin
    if (end <= begin) return MEMORY_TEST_ERR;

    // 32-bit alignment for both addresses
    if ((((uintptr_t)begin | (uintptr_t)end) & 3u) != 0u) return MEMORY_TEST_ERR;

    volatile uint32_t *last = end - 1;

    // 1.1. up(w0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        *p = w0;
    }

    // 1.2. down(w0)
    for (volatile uint32_t *p = last; ; --p) {
        *p = w0;
        if (p == begin) break; // stop before pointer underflow
    }

    // 2. down(r0,w1,w0,w1,r1)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
        *p = w0;
        *p = w1;
        if (*p != w1) return MEMORY_TEST_ERR;
        if (p == begin) break;
    }

    // 3. up(r1,w0,w1,w0,r0)
    for (volatile uint32_t *p = begin; p < end; ++p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
        *p = w1;
        *p = w0;
        if (*p != w0) return MEMORY_TEST_ERR;
    }

    // 4. down(r0,w1,w0,w1,r1)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        *p = w1;
        *p = w0;
        *p = w1;
        if (*p != w1) return MEMORY_TEST_ERR;
        if (p == begin) break;
    }

    // 5. down(r1,w0,w1,w0,r0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w1) return MEMORY_TEST_ERR;
        *p = w0;
        *p = w1;
        *p = w0;
        if (*p != w0) return MEMORY_TEST_ERR;
        if (p == begin) break;
    }

    // 6. down(r0)
    for (volatile uint32_t *p = last; ; --p) {
        if (*p != w0) return MEMORY_TEST_ERR;
        if (p == begin) break;
    }

    return MEMORY_TEST_OK;
}

int march_test(int test_type, volatile uint32_t *begin, volatile uint32_t *end)
{
    if (end <= begin)
        return MEMORY_TEST_ERR;

    switch (test_type)
    {
        case MATS_PLUS:
            return mats_plus(begin, end);

        case MARCH_C_MINUS:
            return march_c_minus(begin, end);

        case MARCH_A:
            return march_a(begin, end);

        case MARCH_LA:
            return march_la(begin, end);

        default:
            return MEMORY_TEST_ERR;
    }
}

#include <stdio.h>
#include "board.h"
#include "clock_test.h"

// Monitor slice 0 is used for all frequency measurements
#define CLOCK_MONITOR_INDEX        (0U)

// Nominal frequencies used for comparison
#define EXT_XTAL24M_NOMINAL_HZ     (24000000UL)
#define EXT_XTAL32K_NOMINAL_HZ     (32768UL)
#define IRC24M_NOMINAL_HZ          (24000000UL)

/*
 * Measure the frequency of one selected clock target.
 *
 * The measurement is performed by the hardware SYSCTL monitor.
 * We use the external 24 MHz clock as the reference source
 * and request 1 Hz measurement accuracy.
 */
static uint32_t monitor_measure_freq_1hz(monitor_target_t target)
{
    monitor_config_t cfg;

    // Load default monitor settings first
    sysctl_monitor_get_default_config(HPM_SYSCTL, &cfg);

    // Override the fields that matter for this test
    cfg.mode = monitor_work_mode_record;      // Store measured frequency in CURRENT
    cfg.accuracy = monitor_accuracy_1hz;      // Fine resolution, important for 32.768 kHz
    cfg.reference = monitor_reference_24mhz;  // Use external 24 MHz as reference
    cfg.divide_by = 1;                        // No divider for monitor output path
    cfg.start_measure = true;                 // Start measurement immediately
    cfg.enable_output = false;                // Do not route monitor output to a pin
    cfg.target = target;                      // Select which clock we want to measure

    // Write the configuration into the selected monitor slice
    sysctl_monitor_init(HPM_SYSCTL, CLOCK_MONITOR_INDEX, &cfg);

    // Wait until the result becomes valid and return measured frequency in Hz
    return sysctl_monitor_get_current_result(HPM_SYSCTL, CLOCK_MONITOR_INDEX);
}


/*
 * Print measured frequency, nominal frequency and percentage deviation.
 *
 * The calculation is done with integer arithmetic only.
 * diff_x10000 stores "percent * 100", so we can print two digits after the dot
 * without using floating-point types.
 */
static void print_deviation_percent(const char *name, uint32_t measured_hz, uint32_t nominal_hz)
{
    int64_t diff_hz = (int64_t) measured_hz - (int64_t) nominal_hz;
    int64_t diff_x10000 = diff_hz * 10000LL / (int64_t) nominal_hz;

    char sign = '+';
    if (diff_x10000 < 0) {
        sign = '-';
        diff_x10000 = -diff_x10000;
    }

    printf("%s\r\n", name);
    printf("  measured : %lu Hz\r\n", measured_hz);
    printf("  nominal  : %lu Hz\r\n", nominal_hz);
    printf("  deviation: %c%lld.%02lld %%\r\n",
           sign,
           diff_x10000 / 100,
           diff_x10000 % 100);
}

// Run the generator health test using XTAL24M as the reference source
void clock_test_run(void)
{
    uint32_t freq_xtal32k;
    uint32_t freq_irc24m;

    printf("\r\n--- CLOCK TEST ---\r\n");
    printf("Reference clock: external XTAL24M (24 MHz)\r\n");

    // Measure the 32 kHz clock domain against the external 24 MHz reference
    freq_xtal32k = monitor_measure_freq_1hz(monitor_target_clk_32k);
    print_deviation_percent("ZQ4 / XTAL32K test", freq_xtal32k, EXT_XTAL32K_NOMINAL_HZ);

    // Measure the internal RC24M clock against the external 24 MHz reference
    freq_irc24m = monitor_measure_freq_1hz(monitor_target_clk_irc24m);
    print_deviation_percent("Internal RC24M test", freq_irc24m, IRC24M_NOMINAL_HZ);

    printf("\r\nConclusion:\r\n");
    printf("- If XTAL32K frequency is close to 32768 Hz, ZQ4 is working.\r\n");
    printf("- If IRC24M is close to 24000000 Hz, the internal 24 MHz RC oscillator is working.\r\n");
    printf("- Since both measurements use XTAL24M as the reference, this also indirectly confirms ZQ3 operation.\r\n");
}

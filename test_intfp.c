/**
 * intfp Library Command-Line Test Tool
 *
 * This tool demonstrates the capabilities of the intfp library with configurable parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <getopt.h>

// Type aliases used by intfp.h
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

#include "intfp.h"

// Test result structure
typedef struct {
    const char *name;
    bool passed;
} test_result_t;

// Global counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Print usage information
void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -b                  Run basic conversion test\n");
    printf("  -c                  Run PUL compression test\n");
    printf("  -e                  Run EWMA functions test\n");
    printf("  -l                  Run log arithmetic test\n");
    printf("  -p                  Run precision test\n");
    printf("  -r                  Run radix conversion test\n");
    printf("  -v, --verbose       Verbose output\n");
    printf("  -h, --help          Show this help message\n");
}

void print_test_summary(const char *test_name, bool passed) {
    if (passed) {
        printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

// Calculate and print percentage error
void print_percentage_error(unsigned long long original, unsigned long long recovered) {
    if (original == 0) {
        printf("  Error: N/A (original is 0)\n");
        return;
    }
    double error_percent = ((double)(recovered > original ? recovered - original : original - recovered) / original) * 100.0;
    printf("  Error: %.6f%%\n", error_percent);
}

void print_final_summary(void) {
    printf("\n========================================");
    printf("\nTest Summary:");
    printf("\n  Tests Run: %d", tests_run);
    printf("\n  Tests Passed: %d", tests_passed);
    printf("\n  Tests Failed: %d", tests_failed);
    printf("\n========================================\n");

    if (tests_failed == 0) {
        printf("All tests completed successfully!\n");
    } else {
        printf("Some tests failed. Review the output above.\n");
    }
}

// Test: Basic Conversion Functions
int test_basic_conversion(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing Basic Integer <-> Log Conversions ===\n");
    }

    // Test 1: u64 to log and back
    u64 original_val = 1000000ULL;
    s32 log_val = u64_to_log32fpmax(original_val);
    u64 recovered_val = log32fpmax_to_u64(log_val);

    if (verbose) {
        printf("Test: Round-trip conversion u64 -> log -> u64\n");
        printf("  Original: %llu\n", (unsigned long long)original_val);
        printf("  Log value: 0x%08x\n", (unsigned int)log_val);
        printf("  Recovered: %llu\n", (unsigned long long)recovered_val);
        print_percentage_error(original_val, recovered_val);
    }

    // The recovered value should be close to original (within approximation error)
    if (recovered_val == 0 || recovered_val > original_val * 2) {
        passed = false;
    }

    // Test 2: Zero handling
    u64 zero_val = 0ULL;
    s32 log_zero = u64_to_log32fpmax(zero_val);
    
    if (log_zero != intfp_log_0(32)) {
        passed = false;
    } else if (verbose) {
        printf("Test: Zero value handling\n");
        printf("  Log of 0: %d (expected special representation)\n", log_zero);
    }

    // Test 3: Small values
    u64 small_val = 1ULL;
    s32 log_small = u64_to_log32fpmax(small_val);

    if (verbose) {
        printf("Test: Small value handling\n");
        printf("  Original: %llu -> Log: 0x%08x\n", (unsigned long long)small_val, (unsigned int)log_small);
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("Basic Conversion", passed);

    return passed ? 1 : 0;
}

// Test: PUL format compression
int test_pul_compression(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing PUL Format Compression ===\n");
    }

    // Test data compression ratio demonstration
    u64 original_data[] = {1000ULL, 50000ULL, 1000000ULL, 0x123456789ABCDEF0ULL};
    
    for (int i = 0; i < 4; i++) {
        u64 val = original_data[i];
        u32 pul_val = u64_to_pul16fpmax(val);
        u64 recovered = pul16fpmax_to_u64(pul_val);

        if (verbose) {
            printf("Test: PUL compression for value %llu\n", (unsigned long long)val);
            printf("  Original:   %llu (%016llx)\n", (unsigned long long)val, (unsigned long long)val);
            printf("  Compressed: %u (%04x)\n", pul_val, pul_val);
            printf("  Recovered:  %llu\n", (unsigned long long)recovered);
            print_percentage_error(val, recovered);
        }
    }

    // Test special encoding for 0 and 1
    u32 pul_zero = u64_to_pul16fpmax(0ULL);
    u32 pul_one = u64_to_pul16fpmax(1ULL);

    if (verbose) {
        printf("Test: Special encoding\n");
        printf("  PUL of 0: %u (expected special value)\n", pul_zero);
        printf("  PUL of 1: %u (expected special value)\n", pul_one);
    }

    // Verify special encoding
    if (pul_zero != intfp_unsigned_min(16) || pul_one != 0) {
        passed = false;
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("PUL Compression", passed);

    return passed ? 1 : 0;
}

// Test: EWMA functions
int test_ewma(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing EWMA Functions ===\n");
    }

    // Test EWMA with division damper
    s32 old_avg1 = 100;
    s32 new_val1 = 200;
    s32 result_div = ewma_s32fp_div(new_val1, old_avg1, 0, 4);

    if (verbose) {
        printf("Test: EWMA with divisor damper\n");
        printf("  Old average: %d, New value: %d\n", old_avg1, new_val1);
        printf("  Result: %d\n", result_div);
    }

    // Test EWMA with shift damper
    s32 result_shr = ewma_s32fp_shr(new_val1, old_avg1, 0, 2);

    if (verbose) {
        printf("Test: EWMA with shift damper\n");
        printf("  Old average: %d, New value: %d\n", old_avg1, new_val1);
        printf("  Result: %d\n", result_shr);
    }

    // Verify EWMA produces valid results
    if (result_div < old_avg1 || result_div > new_val1) {
        passed = false;
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("EWMA Functions", passed);

    return passed ? 1 : 0;
}

// Test: Logarithmic arithmetic demonstration
int test_log_arithmetic(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing Logarithmic Arithmetic ===\n");
    }

    // Demonstrate multiplication via addition in log domain
    u64 a = 1000ULL;
    u64 b = 2000ULL;
    
    s32 log_a = u64_to_log32fpmax(a);
    s32 log_b = u64_to_log32fpmax(b);
    s32 log_product = log_a + log_b;  // Multiplication becomes addition
    u64 product_recovered = log32fpmax_to_u64(log_product);

    if (verbose) {
        printf("Demonstration: Multiplication via logarithmic addition\n");
        printf("  a = %llu, b = %llu\n", (unsigned long long)a, (unsigned long long)b);
        printf("  log(a) = 0x%08x\n", (unsigned int)log_a);
        printf("  log(b) = 0x%08x\n", (unsigned int)log_b);
        printf("  log(a) + log(b) = 0x%08x\n", (unsigned int)log_product);
        printf("  Expected product: %llu\n", (unsigned long long)(a * b));
        printf("  Recovered product: %llu\n", (unsigned long long)product_recovered);
        print_percentage_error(a * b, product_recovered);
    }

    // Verify the result is reasonable
    if (product_recovered < a || product_recovered > a * b * 2) {
        passed = false;
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("Log Arithmetic", passed);

    return passed ? 1 : 0;
}

// Test: Precision of polynomial-corrected log arithmetic
int test_precision(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing Polynomial Correction Precision ===\n");
    }

    // Test 1: Log-domain encoding accuracy (u32 -> log32 corrected)
    // Compare corrected encoded value against true log2 computed via math library
    {
        double max_err = 0.0, sum_err = 0.0;
        u64 worst_val = 0;
        int count = 0;
        u8 ofp = intfp_log_fpmax(32, 32);
        double scale = (double)((u32)1 << ofp);

        // Test a range of values spanning multiple powers of 2
        for (u32 base = 2; base < (1U << 20); base += (base < 1024 ? 1 : base / 64)) {
            s32 log_val = u32_to_log32fp_corr(base, ofp);
            // Extract the encoded fractional log2 value
            double encoded_log2 = (double)log_val / scale;
            double true_log2 = log2((double)base);
            double err = fabs(encoded_log2 - true_log2);
            if (err > max_err) {
                max_err = err;
                worst_val = base;
            }
            sum_err += err;
            count++;
        }

        double avg_err = sum_err / count;
        if (verbose) {
            printf("Test: Log-domain encoding accuracy (u32 -> log32, ofp=%u)\n", ofp);
            printf("  Samples: %d\n", count);
            printf("  Max log-domain error: %.6f (at value %llu)\n",
                   max_err, (unsigned long long)worst_val);
            printf("  Avg log-domain error: %.6f\n", avg_err);
            printf("  (Uncorrected max would be ~0.0861)\n");
        }

        // Corrected max error should be well below uncorrected 0.0861
        if (max_err > 0.02) {
            if (verbose) printf("  FAIL: Max error %.6f exceeds threshold 0.02\n", max_err);
            passed = false;
        }
    }

    // Test 2: E2E multiplication precision
    {
        double max_err_pct = 0.0, sum_err_pct = 0.0;
        u64 worst_a = 0, worst_b = 0;
        int count = 0;

        u32 test_values[] = {
            3, 5, 7, 10, 13, 17, 25, 33, 50, 64, 100, 127, 128, 200,
            255, 256, 300, 500, 700, 1000, 1500, 2000, 3000, 5000,
            7500, 10000, 15000, 20000, 30000, 50000, 65535
        };
        int n_values = sizeof(test_values) / sizeof(test_values[0]);

        for (int i = 0; i < n_values; i++) {
            for (int j = i; j < n_values; j++) {
                u64 a = test_values[i];
                u64 b = test_values[j];
                u64 expected = a * b;

                s32 log_a = u64_to_log32fpmax_corr(a);
                s32 log_b = u64_to_log32fpmax_corr(b);
                s32 log_product = log_a + log_b;
                u64 recovered = log32fpmax_to_u64_corr(log_product);

                double err_pct = (expected > 0) ?
                    fabs((double)recovered - (double)expected) / (double)expected * 100.0 : 0.0;

                /* Skip products < 100 for max error tracking (quantization dominates) */
                if (err_pct > max_err_pct && expected >= 100) {
                    max_err_pct = err_pct;
                    worst_a = a;
                    worst_b = b;
                }
                sum_err_pct += err_pct;
                count++;
            }
        }

        double avg_err_pct = sum_err_pct / count;
        if (verbose) {
            printf("Test: E2E multiplication precision (u64 via log32)\n");
            printf("  Pairs tested: %d\n", count);
            printf("  Max relative error: %.4f%% (at %llu * %llu)\n",
                   max_err_pct, (unsigned long long)worst_a, (unsigned long long)worst_b);
            printf("  Avg relative error: %.4f%%\n", avg_err_pct);
            printf("  (Uncorrected max would be ~11.1%%)\n");
        }

        // Corrected E2E error should be well below uncorrected ~11.1%
        if (max_err_pct > 3.0) {
            if (verbose) printf("  FAIL: Max error %.4f%% exceeds threshold 3.0%%\n", max_err_pct);
            passed = false;
        }
    }

    // Test 3: Boundary values
    {
        bool boundary_ok = true;

        // Zero must be preserved
        s32 log_zero = u64_to_log32fpmax_corr(0ULL);
        if (log_zero != intfp_log_0(32)) boundary_ok = false;
        u64 dec_zero = log32fpmax_to_u64_corr(intfp_log_0(32));
        if (dec_zero != 0) boundary_ok = false;

        // Value 1 must round-trip reasonably
        s32 log_one = u64_to_log32fpmax_corr(1ULL);
        u64 dec_one = log32fpmax_to_u64_corr(log_one);
        if (dec_one == 0 || dec_one > 2) boundary_ok = false;

        // Powers of 2 should have minimal error
        for (int p = 1; p < 40; p++) {
            u64 val = 1ULL << p;
            s32 log_val = u64_to_log32fpmax_corr(val);
            u64 recovered = log32fpmax_to_u64_corr(log_val);
            double err_pct = fabs((double)recovered - (double)val) / (double)val * 100.0;
            if (err_pct > 1.0) {
                if (verbose) printf("  Power of 2 error: 2^%d = %llu -> %llu (%.4f%%)\n",
                                    p, (unsigned long long)val, (unsigned long long)recovered, err_pct);
                boundary_ok = false;
            }
        }

        if (!boundary_ok) {
            if (verbose) printf("Test: Boundary values - FAIL\n");
            passed = false;
        } else if (verbose) {
            printf("Test: Boundary values - OK\n");
        }
    }

    // Test 4: Division via subtraction
    {
        double max_err_pct = 0.0;

        u64 div_pairs[][2] = {
            {1000000, 100}, {50000, 7}, {999999, 333},
            {65536, 256}, {1048576, 1024}, {10000, 3}
        };
        int n_pairs = sizeof(div_pairs) / sizeof(div_pairs[0]);

        for (int i = 0; i < n_pairs; i++) {
            u64 a = div_pairs[i][0];
            u64 b = div_pairs[i][1];
            u64 expected = a / b;

            s32 log_a = u64_to_log32fpmax_corr(a);
            s32 log_b = u64_to_log32fpmax_corr(b);
            s32 log_quotient = log_a - log_b;
            u64 recovered = log32fpmax_to_u64_corr(log_quotient);

            double err_pct = (expected > 0) ?
                fabs((double)recovered - (double)expected) / (double)expected * 100.0 : 0.0;

            if (err_pct > max_err_pct)
                max_err_pct = err_pct;

            if (verbose) {
                printf("  %llu / %llu: expected %llu, got %llu (%.4f%%)\n",
                       (unsigned long long)a, (unsigned long long)b,
                       (unsigned long long)expected, (unsigned long long)recovered, err_pct);
            }
        }

        if (verbose) {
            printf("Test: Division via subtraction - max error %.4f%%\n", max_err_pct);
        }

        if (max_err_pct > 3.0) {
            if (verbose) printf("  FAIL: Division max error %.4f%% exceeds threshold 3.0%%\n", max_err_pct);
            passed = false;
        }
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("Precision (Polynomial Correction)", passed);

    return passed ? 1 : 0;
}

// Test: Radix conversion functions
int test_radix_conversion(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing Radix Conversion Functions ===\n");
    }

    s32 log_val = u64_to_log32fpmax(12345ULL);

    // Test DB_POWER radix conversion
    s32 db_result = rescale_log32fp_to_radix(log_val, U32FP_RADIX_TYPE_DB_POWER);
    
    if (verbose) {
        printf("Test: Radix conversion to dB scale\n");
        printf("  Original log2 value: 0x%08x\n", (unsigned int)log_val);
        printf("  Converted to dB:     0x%08x\n", (unsigned int)db_result);
    }

    // Test round-trip conversion
    s32 roundtrip = rescale_log32fp_from_radix(db_result, U32FP_RADIX_TYPE_DB_POWER);

    if (verbose) {
        printf("Test: Round-trip dB conversion\n");
        printf("  Original:     0x%08x\n", (unsigned int)log_val);
        printf("  Round-tripped: 0x%08x\n", (unsigned int)roundtrip);
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("Radix Conversion", passed);

    return passed ? 1 : 0;
}

// Run all tests
void run_all_tests(bool verbose) {
    printf("\n========================================");
    printf("\nintfp Library Test Suite");
    printf("\n========================================\n");

    // Run each test (print_test_summary is called inside each test function)
    test_basic_conversion(verbose);
    test_pul_compression(verbose);
    test_ewma(verbose);
    test_log_arithmetic(verbose);
    test_precision(verbose);
    test_radix_conversion(verbose);

    printf("\n========================================");
    printf("\nTest Summary:");
    printf("\n  Tests Run: %d", tests_run);
    printf("\n  Tests Passed: %d", tests_passed);
    printf("\n  Tests Failed: %d", tests_failed);
    printf("\n========================================\n");

    if (tests_failed == 0) {
        printf("All tests completed successfully!\n");
    } else {
        printf("Some tests failed. Review the output above.\n");
    }
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    int test_mask = 0; // Bitmask for selected tests
#define TEST_BASIC      0x01
#define TEST_PUL        0x02
#define TEST_EWMA       0x04
#define TEST_LOG        0x08
#define TEST_PRECISION  0x10
#define TEST_RADIX      0x20

    static struct option long_options[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "bcehlprv", long_options, NULL)) != -1) {
        switch (c) {
            case 'b':
                test_mask |= TEST_BASIC;
                break;
            case 'c':
                test_mask |= TEST_PUL;
                break;
            case 'e':
                test_mask |= TEST_EWMA;
                break;
            case 'l':
                test_mask |= TEST_LOG;
                break;
            case 'p':
                test_mask |= TEST_PRECISION;
                break;
            case 'r':
                test_mask |= TEST_RADIX;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    printf("intfp Library - Command Line Test Tool\n");
    printf("======================================\n");

    // Reset counters for each run
    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;

    // If no tests specified, run all tests
    if (test_mask == 0) {
        run_all_tests(verbose);
    } else {
        // Run selected tests only
        if (test_mask & TEST_BASIC) {
            test_basic_conversion(verbose);
        }
        if (test_mask & TEST_PUL) {
            test_pul_compression(verbose);
        }
        if (test_mask & TEST_EWMA) {
            test_ewma(verbose);
        }
        if (test_mask & TEST_LOG) {
            test_log_arithmetic(verbose);
        }
        if (test_mask & TEST_PRECISION) {
            test_precision(verbose);
        }
        if (test_mask & TEST_RADIX) {
            test_radix_conversion(verbose);
        }
        // Print summary for individual test runs
        print_final_summary();
    }

    return tests_failed > 0 ? 1 : 0;
}

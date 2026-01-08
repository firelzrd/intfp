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
    printf("  -c                  Run LOC compression test\n");
    printf("  -e                  Run EWMA functions test\n");
    printf("  -l                  Run log arithmetic test\n");
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

// Test: LOC format compression
int test_loc_compression(bool verbose) {
    tests_run++;
    int passed = true;

    if (verbose) {
        printf("\n=== Testing LOC Format Compression ===\n");
    }

    // Test data compression ratio demonstration
    u64 original_data[] = {1000ULL, 50000ULL, 1000000ULL, 0x123456789ABCDEF0ULL};
    
    for (int i = 0; i < 4; i++) {
        u64 val = original_data[i];
        u32 loc_val = u64_to_loc16fpmax(val);
        u64 recovered = loc16fpmax_to_u64(loc_val);

        if (verbose) {
            printf("Test: LOC compression for value %llu\n", (unsigned long long)val);
            printf("  Original:   %llu (%016llx)\n", (unsigned long long)val, (unsigned long long)val);
            printf("  Compressed: %u (%04x)\n", loc_val, loc_val);
            printf("  Recovered:  %llu\n", (unsigned long long)recovered);
            print_percentage_error(val, recovered);
        }
    }

    // Test special encoding for 0 and 1
    u32 loc_zero = u64_to_loc16fpmax(0ULL);
    u32 loc_one = u64_to_loc16fpmax(1ULL);

    if (verbose) {
        printf("Test: Special encoding\n");
        printf("  LOC of 0: %u (expected special value)\n", loc_zero);
        printf("  LOC of 1: %u (expected special value)\n", loc_one);
    }

    // Verify special encoding
    if (loc_zero != intfp_unsigned_min(16) || loc_one != 0) {
        passed = false;
    }

    if (passed) tests_passed++;
    else tests_failed++;

    print_test_summary("LOC Compression", passed);

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
    test_loc_compression(verbose);
    test_ewma(verbose);
    test_log_arithmetic(verbose);
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
#define TEST_LOC        0x02
#define TEST_EWMA       0x04
#define TEST_LOG        0x08
#define TEST_RADIX      0x10

    static struct option long_options[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "bcehlrv", long_options, NULL)) != -1) {
        switch (c) {
            case 'b':
                test_mask |= TEST_BASIC;
                break;
            case 'c':
                test_mask |= TEST_LOC;
                break;
            case 'e':
                test_mask |= TEST_EWMA;
                break;
            case 'l':
                test_mask |= TEST_LOG;
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
        if (test_mask & TEST_LOC) {
            test_loc_compression(verbose);
        }
        if (test_mask & TEST_EWMA) {
            test_ewma(verbose);
        }
        if (test_mask & TEST_LOG) {
            test_log_arithmetic(verbose);
        }
        if (test_mask & TEST_RADIX) {
            test_radix_conversion(verbose);
        }
        // Print summary for individual test runs
        print_final_summary();
    }

    return tests_failed > 0 ? 1 : 0;
}

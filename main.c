#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

// Define type aliases used by intfp.h
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

// Include the new header file
#include "intfp.h"

void test_rescale(s64 v, s8 gain, u8 fp, enum u32fp_radix_type type) {
	double expected = (double)v * ((type == U32FP_RADIX_TYPE_DB_POWER) ?
		pow(10, gain / 10.0):
		pow(1.25, gain));
	printf("Applying %d gain to %" PRIu64 "...\n", gain, v);
	s32 log_start = u64_to_log32fp(v, fp);
	s32 log_gain_offset = rescale_log32fp_from_radix(s32_to_s32fp(gain, fp), type);
	s32 log_result = log_start + log_gain_offset;
	u64 result_val = log32fp_to_u64(log_result, fp);
	printf("  - Result: %" PRIu64 " (Expected: ~%.0f)\n\n", result_val, expected);
}

int main()
{
	u8 LOG_FRACT_BITS = intfp_log_fpmax(64, 32); /* = 25 (= 32-1-6) */
	
	// --- Demo 1: Basic Integer <-> Log Conversions ---
	// Purpose: Verify that the core conversion to and from the log format works correctly.
	printf("--- 1. Integer <-> Log Representation Demo ---\n");
	u64 val = 1000000;
	s32 log_val = u64_to_log32fp(val, LOG_FRACT_BITS);
	u64 recovered_val = log32fp_to_u64(log_val, LOG_FRACT_BITS);
	printf("Original: %-10" PRIu64 " -> log: 0x%08x -> Recovered: %-10" PRIu64 " (Error: %" PRId64 ")\n\n",
	       val, log_val, recovered_val, (s64)recovered_val - (s64)val);


	// --- Demo 2: Scaled Gain Multiplication ---
	// Purpose: Demonstrate the correct, intended method for applying a scaled gain
	// using the rescale functions. This is a practical application test.
	printf("--- 2. Scaled Gain Multiplication Demo ---\n");

	u64 base_value = 1000000;
	test_rescale(base_value, -10, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,  -5, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,  -3, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,  -2, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,  -1, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,   1, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,   2, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,   3, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,   5, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value,  10, LOG_FRACT_BITS, U32FP_RADIX_TYPE_DB_POWER);
	test_rescale(base_value, -10, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,  -5, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,  -3, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,  -2, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,  -1, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,   1, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,   2, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,   3, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,   5, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);
	test_rescale(base_value,  10, LOG_FRACT_BITS, U32FP_RADIX_TYPE_1_25);


	// --- Demo 3: Direct Radix Constant Verification ---
	// Purpose: Directly test the mathematical correctness of the rescale constants
	// via a round-trip conversion. This isolates the constants from other logic.
	printf("--- 3. Direct Radix Constant Verification (Round-trip test) ---\n");
	s32 test_log_val = u64_to_log32fp(12345, LOG_FRACT_BITS); // An arbitrary test value

	// [Test 3a: SUCCESS Case] DB_POWER constants are mathematically consistent.
	printf("Testing DB_POWER constants...\n");
	// Convert from log2 -> log_db -> log2
	s32 intermediate_db = rescale_log32fp_to_radix(test_log_val, U32FP_RADIX_TYPE_DB_POWER);
	s32 roundtrip_db = rescale_log32fp_from_radix(intermediate_db, U32FP_RADIX_TYPE_DB_POWER);
	printf("  - Original log2: 0x%08x, Round-tripped log2: 0x%08x\n", test_log_val, roundtrip_db);
	printf("  - Difference: %d\n\n", roundtrip_db - test_log_val);

	// [Test 3b: FAILURE Case] RADIX_TYPE_1_25 constants are not consistent.
	printf("Testing RADIX_TYPE_1_25 constants...\n");
	// Convert from log2 -> log_1.25 -> log2
	s32 intermediate_1_25 = rescale_log32fp_to_radix(test_log_val, U32FP_RADIX_TYPE_1_25);
	s32 roundtrip_1_25 = rescale_log32fp_from_radix(intermediate_1_25, U32FP_RADIX_TYPE_1_25);
	printf("  - Original log2: 0x%08x, Round-tripped log2: 0x%08x\n", test_log_val, roundtrip_1_25);
	printf("  - Difference: %d\n\n", roundtrip_1_25 - test_log_val);


	// --- Demo 4: Other Library Features ---
	printf("--- 4. Other Library Features ---\n");

	// EWMA (Exponentially Weighted Moving Average)
	s32 old_avg = s32_to_s32fp(100, 8); // 100.0 in Q24.8 format
	s32 new_val = s32_to_s32fp(200, 8); // 200.0 in Q24.8 format
	s32 next_avg = ewma_s32fp_div(new_val, old_avg, 0, 4); // damper=4
	printf("EWMA: old=100.0, new=200.0 -> next=%.2f (Expected: 125.0)\n", (double)next_avg / (1 << 8));

	// LOC format (for storage, not calculation)
	u64 loc_original = 50000;
	u32 loc_val = u64_to_loc32fp(loc_original, LOG_FRACT_BITS);
	u64 loc_recovered = loc32fp_to_u64(loc_val, LOG_FRACT_BITS);
	printf("LOC Format: Original: %-7" PRIu64 " -> loc: 0x%08x -> Recovered: %-7" PRIu64 "\n",
	       loc_original, loc_val, loc_recovered);

	return 0;
}
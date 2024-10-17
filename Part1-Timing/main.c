#include "utility.h"

#ifndef VISUAL
#define PRINT_FUNC print_results_plaintext
#else
#define PRINT_FUNC print_results_for_visualization
#endif

#define LINE_SIZE 64
#define L1_SIZE 32768  // 32KiB per physical core
#define L2_SIZE 262144 // 256KiB per physical core
#define L3_SIZE 6291456 // 6MiB shared

//#define SMALL_BUFF

/* Small eviciton set results in no secondary bump
 * for L2 access latencies at ~40 cycles. However,
 * it increses rate of ~25 cycle access latencies
 * from L3.
 */
#ifdef SMALL_BUFF
#define BUFF_SIZE 311296
#define BUFF_SIZE_L2 65536
#else
#define BUFF_SIZE 522752
#define BUFF_SIZE_L2 131072
#endif

int main (int ac, char **av) {

    // create 4 arrays to store the latency numbers
    // the arrays are initialized to 0
    uint64_t dram_latency[SAMPLES] = {0};
    uint64_t l1_latency[SAMPLES] = {0};
    uint64_t l2_latency[SAMPLES] = {0};
    uint64_t l3_latency[SAMPLES] = {0};

    // A temporary variable we can use to load addresses
    uint8_t tmp;

    // Allocate a buffer of LINE_SIZE Bytes
    // The volatile keyword tells the compiler to not put this variable into a
    // register -- it should always try to be loaded from memory / cache.
    volatile uint8_t *target_buffer = (uint8_t *)malloc(LINE_SIZE);

    if (NULL == target_buffer) {
        perror("Unable to malloc target");
        return EXIT_FAILURE;
    }

    // Eviction buffer of 128KiB
    volatile uint8_t *eviction_buffer = (uint8_t *)malloc(BUFF_SIZE);

    if (NULL == eviction_buffer) {
	perror("Unable to malloc eviction");
	return EXIT_FAILURE;
    }

    // Measure L1 access latency, store results in l1_latency array
    for (int i=0; i<SAMPLES; i++){
        // Step 1: bring the target cache line into L1 by simply accessing
        //         the line
        tmp = target_buffer[0];

        // Step 2: measure the access latency
        l1_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }

    // ======
    // [1.2] Measure DRAM Latency, store results in dram_latency array
    // ======
    //
    for (int i = 0; i < SAMPLES; i++) {
	
	// Load cache line, then flush to DRAM
	tmp = target_buffer[0];
	clflush((void *)target_buffer);

	dram_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }

    // ======
    // [1.2] Measure L2 Latency, store results in l2_latency array
    // ======
    //
    for (int i = 0; i < SAMPLES; i++) {
	
	tmp = target_buffer[0];
	//lfence(); // may have an effect, though if it does it is negligible
	
	for (int j = 0; j < BUFF_SIZE_L2; j += LINE_SIZE)
	    eviction_buffer[j] = 0;

	l2_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }

    // ======
    // [1.2] Measure L3 Latency, store results in l3_latency array
    // ======
    //
    for (int i = 0; i < SAMPLES; i++) {

	tmp = target_buffer[0];
	//lfence();
	
	for (int j = 0; j < BUFF_SIZE; j += LINE_SIZE)
	    eviction_buffer[j] = 0;

	l3_latency[i] = measure_one_block_access_time((uint64_t)target_buffer);
    }


    // Print the results to the screen
    // When compile to main and used by `make run`,
    // it uses print_results_plaintext
    // When compile to main-visual and used by `run.py`,
    // it uses print_results_for_visualization
    PRINT_FUNC(dram_latency, l1_latency, l2_latency, l3_latency);

    free((uint8_t *)target_buffer);
    free((uint8_t *)eviction_buffer);
    
    return 0;
}


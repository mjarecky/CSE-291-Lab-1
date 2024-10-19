#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

#define NUM_SETS 256
#define NUM_ADDRESSES 12 // Need at least 12 (8 way L1 + 4 way L2), max 32 (5 bits controllable L2 tag)
#define THRESHOLD 34 // threshold lowered from 40 to 34 based on experimental results
#define BUFF_SIZE (1 << 21)


int main(int argc, char **argv)
{
    // to store access time for each unique address for a specific set
    CYCLES set_access_time[NUM_ADDRESSES];
    

    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE |
		     MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);

    if (buf == (void *) -1) {
	perror("mmap() error\n");
	exit(EXIT_FAILURE);
    }

    *((char *)buf) = 1;

    ADDR_PTR page_base = (ADDR_PTR)buf;


    printf("Please press enter.\n");
    char text_buf[2];
    fgets(text_buf, sizeof(text_buf), stdin);

    printf("Receiver now listening.\n");


    int sender_set;
    while (true) {

	// fences to isolate prime from rest of program
	asm volatile("mfence");

	// prime
	// performed many times to saturate L2 sets
	for (int k = 0; k < 100; k++) {
	    for (int i = 0; i < NUM_SETS; i++) {
	        for (int j = 0; j < NUM_ADDRESSES; j++)
	            *((char *)get_address(j, i, page_base)) = 0;
	    }
	}
	
	asm volatile("mfence");

	// wait
	// number of cycles is arbitrary
	// but allows time for sender to saturate
	// L2 set with its addresses
	int cycles = 0;
	while (cycles < 5000) cycles++;

	// prime/probe
	// done in reverse order to avoid thrashing
	for (int i = NUM_SETS-1; i >= 0; i--) {

	    int ex_threshold = 0;
	    //int sub_threshold = 0;

	    for (int j = NUM_ADDRESSES-1; j >= 0; j--) {
		
		// measure access time for each unique address that maps to the same L2 set
	        set_access_time[j] = measure_one_block_access_time(get_address(j, i, page_base));

		if (set_access_time[j] >= THRESHOLD)
		    ex_threshold++;

		//if (set_access_time[j] >= 34 && set_access_time[j] < THRESHOLD)
		//    pad++;
	    }
		
	    // if every access to the uinque addresses for a particular set
	    // results in a latency greater than or equal to the threshold then every address
	    // is most likely in the L3 cache or lower, therefore that particular
	    // set was most likely accessed by the sender
	    if (ex_threshold == NUM_ADDRESSES) {
	    //if (ex_threshold > (NUM_ADDRESSES - 4) && pad == (NUM_ADDRESSES - ex_threshold)) {
		sender_set = i;
		goto finish;
	    }
	}
    }

finish:

    printf("Sender message: %d\n", sender_set);

    for (int i = 0; i < NUM_ADDRESSES; i++)
	printf("%d\n", set_access_time[i]);

    printf("Receiver finished.\n");

    return 0;
}

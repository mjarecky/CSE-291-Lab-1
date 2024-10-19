#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

#define NUM_SETS 256
#define NUM_ADDRESSES 12
#define THRESHOLD 40
#define BUFF_SIZE (1 << 21)


int main(int argc, char **argv)
{
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

	asm volatile("mfence");

	// prime
	for (int k = 0; k < 100; k++) {
	    for (int i = 0; i < NUM_SETS; i++) {
	        for (int j = 0; j < NUM_ADDRESSES; j++)
	            *((char *)get_address(j, i, page_base)) = 0;
	    }
	}
	
	asm volatile("mfence");

	// wait
	int cycles = 0;
	while (cycles < 5000) cycles++;

	// prime/probe
	for (int i = NUM_SETS-1; i >= 0; i--) {

	    int ex_threshold = 0;

	    for (int j = NUM_ADDRESSES-1; j >= 0; j--) {
		
	        set_access_time[j] = measure_one_block_access_time(get_address(j, i, page_base));

		if (set_access_time[j] > THRESHOLD)
		    ex_threshold++;
	    }

	    if (ex_threshold == NUM_ADDRESSES) {
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

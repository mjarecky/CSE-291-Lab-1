#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

#define MAX_CHAR 255

/* Using a hugepage of 2MiB, the number of page offset bits is 21.
 * The L2 cache contains 1024 sets, thus 10 bits are used for the set
 * index. 6 bits are also used for the block offet. 21 - (10 + 6) = 5 bits.
 * Therefore, 5 bits of the address that correspond to the tag in the L2
 * cache can be modified. As such, 32 unique addresses can be made.
 */
#define NUM_ADDRESSES 32

// Many samples to force runtime of program to be long
// and saturate specific L2 cache set 
#define SAMPLES 20000

#define BUFF_SIZE (1 << 21)


int main(int argc, char **argv)
{
    // Allocate a buffer using huge page
    // See the handout for details about hugepage management
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE |
                     MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    
    if (buf == (void *) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    // The first access to a page triggers overhead associated with
    // page allocation, TLB insertion, etc.
    // Thus, we use a dummy write here to trigger page allocation
    // so later access will not suffer from such overhead.
    *((char *)buf) = 1; // dummy write to trigger page allocation

    ADDR_PTR page_base = (ADDR_PTR)buf;


    printf("Please type a message.\n");
    char text_buf[128];

input:
    fgets(text_buf, sizeof(text_buf), stdin);

    if (strcmp(text_buf, "exit\n") == 0)
	goto exit;

    int unval_message = string_to_int(text_buf);

    if (unval_message > MAX_CHAR || unval_message < 0) {
        printf("Message value must be between 0 and 255 inclusive.\n");
	goto input;
    }

    char message = (char)unval_message;


    for (int i = 0; i < SAMPLES; i++) {

	// fences added to isolate cache accesses
	// from rest of program and to force accesses
	// into specific blocks
	asm volatile("mfence");
	
	// access message set
	// addresses accessed many times to further increase
	// program runtime and saturate L2 cache set
	for (int k = 0; k < 100; k++) {
            for (int i = 0; i < NUM_ADDRESSES; i++)
	        *((char *)get_address(i, message, page_base)) = 0;
	}

	asm volatile("mfence");
    }

    //goto input;

exit:
    printf("Sender finished.\n");
    return 0;
}

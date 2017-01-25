#include <stdio.h>  
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define NUM_CORES 4
#define GIG_SIZE   1073741824 * sizeof(unsigned char)
#define ARRAY_SIZE   GIG_SIZE * 5
#define PARTIAL_FILL_SIZE   ARRAY_SIZE/NUM_CORES


unsigned char *shared_mem;
unsigned array_multiplicity = 256; 
unsigned long thread_id_array[NUM_CORES] = {0};
unsigned long long global_start_pos = 0; 
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;



void *fill_array_partial()
{
	static unsigned thread_counter = 0;

	//begin critical section
	int rv = pthread_mutex_lock(&mtx);
	if (rv != 0)
	{
		printf("locking failed\n");
		exit(1);
	}
	
	thread_id_array[thread_counter] = pthread_self();
	thread_counter++;

	unsigned long long thread_start_pos;
	thread_start_pos = global_start_pos;
	global_start_pos += PARTIAL_FILL_SIZE;

	unsigned long long end_pos = thread_start_pos + PARTIAL_FILL_SIZE;
	//printf("some thread set start_pos == %llu, end_pos == %llu\n", thread_start_pos, end_pos);
	rv = pthread_mutex_unlock(&mtx);
	//end critical section

	unsigned num_fills;			// for testing relative complexity (no memory left to increase size of array itself, so have to simulate it)
	for (num_fills = 0; num_fills < array_multiplicity; num_fills++)
	{	
		unsigned long long counter;
		for (counter = thread_start_pos; counter < end_pos; counter++)
		{
			shared_mem[counter] = 1;
		}
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	shared_mem = malloc(ARRAY_SIZE);
	memset(shared_mem, 0, ARRAY_SIZE);


#if NUM_CORES > 1
	struct timeval start_time_struct;
	gettimeofday(&start_time_struct, NULL);
	unsigned long start_time = start_time_struct.tv_sec;

	pthread_t thread;
	unsigned core_counter;
	for (core_counter = 0; core_counter < NUM_CORES; core_counter++)
	{
		pthread_create(&thread, NULL, fill_array_partial, NULL);
	}

	while (thread_id_array[0] == 0)
	{
		// wait for first id to be populated before calling join() on the id
	}

	for (core_counter = 0; core_counter < NUM_CORES; core_counter++)
	{
		pthread_join(thread_id_array[core_counter], NULL);
		//printf("in main(), thread %lu joined\n", thread_id_array[core_counter]);
	}
	
	struct timeval end_time_struct;
	gettimeofday(&end_time_struct, NULL);
	unsigned long end_time = end_time_struct.tv_sec;
	
	unsigned long time_to_fill = end_time - start_time;


	printf("\n");
	unsigned long long counter;
	unsigned long long total_errors = 0;
	unsigned long long flawed_pos;
	for (counter = 0; counter < ARRAY_SIZE; counter++)
	{
		if (shared_mem[counter] != 1)
		{
			total_errors++;
			flawed_pos = counter;
		}
	}
	free(shared_mem); 

	if (total_errors > 0)
	{
		printf("ERROR (multiplicity %u): from the main thread's perspective, the array was not filled successfully: last error's position: %llu\n", 
							array_multiplicity, flawed_pos);
		printf("    %llu errors total\n", total_errors);
		return 1;
	}
	else
	{
		printf("SUCCESS: From the main thread's perspective, the array was filled successfully.\n");
		printf("         With %u cores, it took %lu seconds (%f minutes) to fill the array (multiplicity %u).\n", 
							NUM_CORES, time_to_fill, time_to_fill/(float)60, array_multiplicity); 
	}


#else
	struct timeval start_time_struct;
	gettimeofday(&start_time_struct, NULL);
	unsigned long start_time = start_time_struct.tv_sec;

	unsigned num_fills;					// for testing relative complexity (no memory left to increase size of array itself, so have to simulate it)
	for (num_fills = 0; num_fills < array_multiplicity; num_fills++)		
	{
		unsigned long long counter;
		for (counter = 0; counter < ARRAY_SIZE; counter++)
		{
			shared_mem[counter] = 1;
		}
	}	

	struct timeval end_time_struct;
	gettimeofday(&end_time_struct, NULL);
	unsigned long end_time = end_time_struct.tv_sec;

	unsigned long time_to_fill = end_time - start_time;


	int error = 0;
	unsigned long long counter;
	for (counter = 0; counter < ARRAY_SIZE; counter++)
	{
		if (shared_mem[counter] != 1)
		{
			error = 1;
		}
	}	
	free(shared_mem);

	if (error == 1)
	{
		printf("error filling array by single thread\n");
		return 1;
	}
	else
	{
		printf("SUCCESS: The array was filled successfully.\n");
		printf("         With a single thread, it took %lu seconds (%f minutes) to fill the array (multiplicity %u).\n", 
							time_to_fill, time_to_fill/(float)60, array_multiplicity);  
	}
#endif

	return 0;
}

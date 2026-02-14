#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define NUM_THREADS 4
#define ITERATIONS 500000000

void* heavy_computation(void* arg) {
    int thread_id = *(int*)arg;
    int current_cpu;
    
    for (int step = 0; step < 5; step++) {
        for (volatile long i = 0; i < ITERATIONS / 5; i++);

        current_cpu = sched_getcpu();
        
        printf("Thread %d is running on CPU Core: %d\n", thread_id, current_cpu);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;

        if (pthread_create(&threads[i], NULL, heavy_computation, &thread_ids[i]) != 0) {
            perror("Thread create failed");

            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}

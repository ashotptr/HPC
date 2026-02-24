#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4
#define NUM_STAGES 3

pthread_barrier_t stage_barrier;
pthread_barrier_t print_barrier;

void* worker_task(void* arg) {
    int thread_id = *(int*)arg;

    for (int stage = 1; stage <= NUM_STAGES; stage++) {
        sleep(thread_id % 2); 

        printf("Thread %d completed work for Stage %d\n", thread_id, stage);

        pthread_barrier_wait(&stage_barrier);
        
        pthread_barrier_wait(&print_barrier);
    }

    printf("Thread %d is exiting the pipeline.\n", thread_id);
    
    return NULL;
}

int main() {
    pthread_t workers[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    int total_threads = NUM_THREADS + 1;

    pthread_barrier_init(&stage_barrier, NULL, total_threads);
    pthread_barrier_init(&print_barrier, NULL, total_threads);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;

        if (pthread_create(&workers[i], NULL, worker_task, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
        
            return 1;
        }
    }

    for (int stage = 1; stage <= NUM_STAGES; stage++) {
        pthread_barrier_wait(&stage_barrier);
        
        printf("\nAll threads finished stage %d\n\n", stage);
        
        pthread_barrier_wait(&print_barrier);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }

    pthread_barrier_destroy(&stage_barrier);
    pthread_barrier_destroy(&print_barrier);

    return 0;
}

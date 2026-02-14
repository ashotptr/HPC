#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>

#define ARRAY_SIZE 50000000
#define NUM_THREADS 4

typedef struct {
    int* array;
    int start_index;
    int end_index;
    int local_max;
} ThreadArgs;

double get_time_in_seconds() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void* find_partial_max(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    
    int max_val = INT_MIN;
    
    for (int i = args->start_index; i < args->end_index; i++) {
        if (args->array[i] > max_val) {
            max_val = args->array[i];
        }
    }

    args->local_max = max_val;

    return NULL;
}

int main() {
    int* array = malloc(ARRAY_SIZE * sizeof(int));
    
    if (!array) {
        perror("Allocation failed");

        return 1;
    }

    srand(42);

    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = rand();
    }

    double start_seq = get_time_in_seconds();    
    int seq_max = INT_MIN;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] > seq_max) {
            seq_max = array[i];
        }
    }
    
    double end_seq = get_time_in_seconds();

    printf("Sequential Max: %d\n", seq_max);
    printf("Sequential Time: %.4f seconds\n", end_seq - start_seq);
    
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    double start_par = get_time_in_seconds();
    int chunk_size = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].array = array;
        args[i].start_index = i * chunk_size;
        
        if (i == NUM_THREADS - 1) {
            args[i].end_index = ARRAY_SIZE;
        }
        else {
            args[i].end_index = (i + 1) * chunk_size;
        }

        pthread_create(&threads[i], NULL, find_partial_max, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    int par_max = INT_MIN;

    for (int i = 0; i < NUM_THREADS; i++) {
        if (args[i].local_max > par_max) {
            par_max = args[i].local_max;
        }
    }

    double end_par = get_time_in_seconds();

    printf("Parallel Max: %d\n", par_max);
    printf("Parallel Time: %.4f seconds\n", end_par - start_par);
    
    if (seq_max == par_max) {
        printf("\nValues match\n");
    }
    else {
        printf("\nValues do not match\n");
    }

    free(array);

    return 0;
}

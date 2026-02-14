#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define ARRAY_SIZE 50000000
#define NUM_THREADS 4

typedef struct {
    int* array;
    int start_index;
    int end_index;
} ThreadArgs;

double get_time_in_seconds() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void* compute_partial_sum(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    
    long long* partial_sum = malloc(sizeof(long long)); 
    *partial_sum = 0;

    for (int i = args->start_index; i < args->end_index; i++) {
        *partial_sum += args->array[i];
    }

    return (void*)partial_sum;
}

int main() {
    int* array = malloc(ARRAY_SIZE * sizeof(int));

    if (array == NULL) {
        perror("Failed to allocate array");

        return 1;
    }

    srand(42);

    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = rand() % 100;
    }

    double start_seq = get_time_in_seconds();
    long long total_sum_seq = 0;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        total_sum_seq += array[i];
    }
    
    double end_seq = get_time_in_seconds();

    printf("Sequential Sum: %lld\n", total_sum_seq);
    printf("Sequential Time: %.4f seconds\n", end_seq - start_seq);

    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];
    double start_par = get_time_in_seconds();
    int chunk_size = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].array = array;
        thread_args[i].start_index = i * chunk_size;
        
        if (i == NUM_THREADS - 1) {
            thread_args[i].end_index = ARRAY_SIZE;
        }
        else {
            thread_args[i].end_index = (i + 1) * chunk_size;
        }

        if (pthread_create(&threads[i], NULL, compute_partial_sum, &thread_args[i]) != 0) {
            perror("Thread creation failed");

            return 1;
        }
    }

    long long total_sum_par = 0;
    void* retval;

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], &retval);
        
        long long* partial_result = (long long*)retval;
        total_sum_par += *partial_result;
        
        free(partial_result);
    }

    double end_par = get_time_in_seconds();

    printf("Parallel Sum: %lld\n", total_sum_par);
    printf("Parallel Time: %.4f seconds\n", end_par - start_par);
    
    if (total_sum_seq == total_sum_par) {
        printf("\nSums match\n");
    }
    else {
        printf("\nSums do not match\n");
    }

    free(array);

    return 0;
}

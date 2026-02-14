#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define RANGE_LIMIT 20000000
#define NUM_THREADS 4

typedef struct {
    int start;
    int end;
    int local_count;
} ThreadArgs;

double get_time_in_seconds() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int is_prime(int n) {
    if (n <= 1) {
        return 0; 
    }

    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return 0;
        }
    }

    return 1;
}

void* count_primes_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int count = 0;
    
    for (int i = args->start; i < args->end; i++) {
        if (is_prime(i)) {
            count++;
        }
    }
    
    args->local_count = count;

    return NULL;
}

int main() {
    printf("Counting primes up to %d\n", RANGE_LIMIT);

    double start_seq = get_time_in_seconds();
    
    int total_primes_seq = 0;

    for (int i = 1; i < RANGE_LIMIT; i++) {
        if (is_prime(i)) {
            total_primes_seq++;
        }
    }
    
    double end_seq = get_time_in_seconds();

    printf("Sequential Count: %d\n", total_primes_seq);
    printf("Sequential Time: %.4f seconds\n", end_seq - start_seq);
    
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    double start_par = get_time_in_seconds();

    int range_per_thread = RANGE_LIMIT / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start = (i * range_per_thread) + 1;
        
        if (i == NUM_THREADS - 1) {
            args[i].end = RANGE_LIMIT;
        }
        else {
            args[i].end = (i + 1) * range_per_thread;
        }
        
        pthread_create(&threads[i], NULL, count_primes_thread, &args[i]);
    }

    int total_primes_par = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);

        total_primes_par += args[i].local_count;
    }

    double end_par = get_time_in_seconds();

    printf("Parallel Count: %d\n", total_primes_par);
    printf("Parallel Time: %.4f seconds\n", end_par - start_par);

    if (total_primes_seq == total_primes_par) {
        printf("\nCounts match\n");
    }
    else {
        printf("\nCounts do not match\n");
    }

    return 0;
}

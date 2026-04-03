#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_ORDERS 10000
#define NUM_THREADS 4

#define HIGH 0
#define NORMAL 1

typedef struct {
    int order_id;
    int distance_km;
    int priority;
} Order;

static Order orders[NUM_ORDERS];
static int thread_high_count[NUM_THREADS];
static int threshold;

int main(void)
{
    omp_set_num_threads(NUM_THREADS);

    srand(42);

    for (int i = 0; i < NUM_ORDERS; i++) {
        orders[i].order_id = i + 1;
        orders[i].distance_km = rand() % 50;
        orders[i].priority = NORMAL;
    }

    for (int t = 0; t < NUM_THREADS; t++) {
        thread_high_count[t] = 0;
    }

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();

        #pragma omp single
        {
            threshold = 20;
        }
        
        #pragma omp for
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].distance_km < threshold) {
                orders[i].priority = HIGH;
            }
            else {
                orders[i].priority = NORMAL;
            }
        }

        #pragma omp barrier

        #pragma omp single
        {
            printf("[Thread %d] Priority assignment finished.\n", tid);
        }

        #pragma omp for
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].priority == HIGH) {
                thread_high_count[tid]++;
            }
        }
        
        #pragma omp barrier

        #pragma omp single
        {
            int total = 0;

            for (int t = 0; t < NUM_THREADS; t++) {
                printf("Thread %d HIGH orders: %d\n", t, thread_high_count[t]);

                total += thread_high_count[t];
            }

            printf("Total HIGH priority orders: %d / %d\n", total, NUM_ORDERS);
            printf("Total NORMAL priority orders: %d / %d\n", NUM_ORDERS - total, NUM_ORDERS);
        }

    }

    return 0;
}

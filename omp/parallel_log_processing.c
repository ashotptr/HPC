#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_LOGS 20000
#define NUM_THREADS 4

#define FAST 0
#define MEDIUM 1
#define SLOW 2

typedef struct {
    int request_id;
    int user_id;
    int response_time_ms;
    int category;
} LogEntry;

static LogEntry logs[NUM_LOGS];
static int fast_count = 0;
static int medium_count = 0;
static int slow_count = 0;

int main(void)
{
    omp_set_num_threads(NUM_THREADS);

    #pragma omp parallel
    {
        #pragma omp single
        {
            srand(42);

            for (int i = 0; i < NUM_LOGS; i++) {
                logs[i].request_id = i + 1;
                logs[i].user_id = (rand() % 1000) + 1;
                logs[i].response_time_ms = rand() % 500;
                logs[i].category = -1;
            }
        }
        
        #pragma omp barrier

        #pragma omp for 
        for (int i = 0; i < NUM_LOGS; i++) {
            int ms = logs[i].response_time_ms;

            if (ms < 100) {
                logs[i].category = FAST;
            }
            else if (ms <= 300) {
                logs[i].category = MEDIUM;
            }
            else {
                logs[i].category = SLOW;
            }
        }

        #pragma omp barrier

        #pragma omp single
        {
            for (int i = 0; i < NUM_LOGS; i++) {
                switch (logs[i].category) {
                    case FAST:
                        fast_count++;
                        break;
                    case MEDIUM: 
                        medium_count++; 
                        break;
                    case SLOW:
                        slow_count++;
                        break;
                    default:
                        fprintf(stderr, "Unclassified %d!\n", i);
                }
            }

            printf("Total entries  : %d\n", NUM_LOGS);
            printf("FAST   (<100ms): %d\n", fast_count);
            printf("MEDIUM (100-300ms): %d\n", medium_count);
            printf("SLOW   (>300ms): %d\n", slow_count);
        }

    }

    return 0;
}

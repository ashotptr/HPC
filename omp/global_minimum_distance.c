#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <omp.h>

#define N 50000000
#define NUM_THREADS 4

int main(void)
{
    double *A = malloc(N * sizeof(double));
    
    if (!A) { 
        perror("malloc"); 
        
        return 1; 
    }

    srand(42);
    
    for (int i = 0; i < N; i++) {
        A[i] = (double)rand() / RAND_MAX;
    }

    double min_diff;
    double t0, t1;

    omp_set_num_threads(NUM_THREADS);

    min_diff = DBL_MAX;
    t0 = omp_get_wtime();
    
    for (int i = 1; i < N; i++) {
        double diff = fabs(A[i] - A[i - 1]);

        if (diff < min_diff) {
            min_diff = diff;
        }    
    }
    
    t1 = omp_get_wtime();
    
    printf("Sequential: min_diff = %.10e  time = %.3f s\n", min_diff, t1 - t0);

    min_diff = DBL_MAX;
    t0 = omp_get_wtime();

    #pragma omp parallel for reduction(min: min_diff)
    for (int i = 1; i < N; i++) {
        double diff = fabs(A[i] - A[i-1]);

        if (diff < min_diff) {
            min_diff = diff;
        }
    }

    t1 = omp_get_wtime();
    
    printf("Parallel: min_diff = %.10e  time = %.3f s\n", min_diff, t1 - t0);

    free(A);
    
    return 0;
}

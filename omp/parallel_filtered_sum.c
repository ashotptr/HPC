#include <stdio.h>
#include <stdlib.h>
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

    double t0 = omp_get_wtime();
    double max_val = -DBL_MAX;

    #pragma omp parallel for reduction(max: max_val)
    for (int i = 0; i < N; i++) {
        if (A[i] > max_val) {
            max_val = A[i];
        }
    }

    double T = 0.8 * max_val;
    double filtered_sum = 0.0;

    #pragma omp parallel for reduction(+: filtered_sum)
    for (int i = 0; i < N; i++) {
        if (A[i] > T) {
            filtered_sum += A[i];
        }
    }

    double t1 = omp_get_wtime();

    printf("max(A) = %.10f\n", max_val);
    printf("Filtered sum = %.6f\n", filtered_sum);
    printf("Time = %.3f s\n", t1 - t0);

    free(A);
    
    return 0;
}

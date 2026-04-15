#include <stdio.h>
#include <omp.h>

#define THRESHOLD 10

long long fibonacci(int n)
{
    if (n <= 1) {
        return n;
    }
    
    if (n <= THRESHOLD) {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }

    long long x, y;

    #pragma omp task shared(x) firstprivate(n)
    x = fibonacci(n - 1);

    #pragma omp task shared(y) firstprivate(n)
    y = fibonacci(n - 2);

    #pragma omp taskwait

    return x + y;
}

int main(void)
{
    int num;
 
    printf("Enter a non-negative integer: ");

    if (scanf("%d", &num) != 1 || num < 0) {
        fprintf(stderr, "Invalid input. Enter a non-negative integer.\n");
        
        return 1;
    }

    long long result;
    double start = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            result = fibonacci(num);
        }
    }

    double elapsed = omp_get_wtime() - start;

    printf("F(%d) = %lld\n", num, result);
    printf("Computed in %.6f seconds using %d thread(s).\n", elapsed, omp_get_max_threads());

    return 0;
}

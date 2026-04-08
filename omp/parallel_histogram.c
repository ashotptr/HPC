#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define N 100000000
#define BINS 256

static double now(void)
{
    return omp_get_wtime();
}

void hist_sequential(const unsigned char *A, long long *hist)
{
    memset(hist, 0, BINS * sizeof(long long));

    for (int i = 0; i < N; i++) {
        hist[A[i]]++;
    }
}

void hist_naive(const unsigned char *A, long long *hist)
{
    memset(hist, 0, BINS * sizeof(long long));

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        hist[A[i]]++;
    }
}

void hist_critical(const unsigned char *A, long long *hist)
{
    memset(hist, 0, BINS * sizeof(long long));

    #pragma omp parallel
    {
        long long local_hist[BINS] = {0};

        #pragma omp for
        for (int i = 0; i < N; i++) {
            local_hist[A[i]]++;
        }

        #pragma omp critical
        for (int b = 0; b < BINS; b++) {
            hist[b] += local_hist[b];
        }
    }
}

void hist_reduction(const unsigned char *A, long long *hist)
{
    memset(hist, 0, BINS * sizeof(long long));

    #pragma omp parallel for reduction(+: hist[:BINS])
    for (int i = 0; i < N; i++) {
        hist[A[i]]++;
    }
}

static long long total(const long long *hist)
{
    long long s = 0;
    
    for (int b = 0; b < BINS; b++){
        s += hist[b];
    }
    
    return s;
}

static int equal(const long long *a, const long long *b)
{
    for (int i = 0; i < BINS; i++) {
        if (a[i] != b[i]){
            return 0;
        }
    }

    return 1;
}

int main(void)
{
    unsigned char *A = malloc(N);
    
    if (!A) { 
        perror("malloc");
        
        return 1;
    }

    srand(42);

    for (int i = 0; i < N; i++) {
        A[i] = (unsigned char)(rand() % BINS);
    }

    long long ref [BINS];
    long long hist[BINS];
    double t0, t1;

    t0 = now();
    
    hist_sequential(A, ref);
    
    t1 = now();
    printf("Sequential: %.3f s  (total=%lld)\n", t1 - t0, total(ref));

    t0 = now();
    
    hist_naive(A, hist);
    
    t1 = now();
    printf("Naive (race): %.3f s  total=%lld  correct=%s\n", t1 - t0, total(hist), equal(hist, ref) ? "yes" : "no");

    t0 = now();
    
    hist_critical(A, hist);
    
    t1 = now();
    printf("Critical: %.3f s  total=%lld  correct=%s\n", t1 - t0, total(hist), equal(hist, ref) ? "yes" : "no");

    t0 = now();
    
    hist_reduction(A, hist);
    
    t1 = now();
    printf("Reduction: %.3f s  total=%lld  correct=%s\n", t1 - t0, total(hist), equal(hist, ref) ? "yes" : "no");

    free(A);
    
    return 0;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <immintrin.h>

#define DNA_SIZE_MB 256ULL
#define DNA_SIZE (DNA_SIZE_MB * 1024ULL * 1024ULL)
#define NUM_THREADS 4

#define A_IDX 0
#define C_IDX 1
#define G_IDX 2
#define T_IDX 3

static inline double now_sec() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static char *dna_buf = NULL;

static void generate_dna(char *buf, size_t n) {
    static const char table[4] = {'A', 'C', 'G', 'T'};
    unsigned int seed = 42;
    
    for (size_t i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        buf[i] = table[(seed >> 30) & 3];
    }
}

static void count_scalar(const char *buf, size_t n, long long counts[4]) {
    long long a = 0, c = 0, g = 0, t = 0;

    for (size_t i = 0; i < n; i++) {
        char ch = buf[i];
        a += (ch == 'A');
        c += (ch == 'C');
        g += (ch == 'G');
        t += (ch == 'T');
    }

    counts[A_IDX] = a;
    counts[C_IDX] = c;
    counts[G_IDX] = g;
    counts[T_IDX] = t;
}

static long long mt_global[4];
static pthread_mutex_t mt_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    const char *start;
    size_t len;
} ThreadArg;

static void *mt_worker(void *arg) {
    const ThreadArg *a = (ThreadArg *)arg;
    const char *p = a->start;
    size_t n = a->len;

    long long la = 0, lc = 0, lg = 0, lt = 0;

    for (size_t i = 0; i < n; i++) {
        char ch = p[i];
        la += (ch == 'A');
        lc += (ch == 'C');
        lg += (ch == 'G');
        lt += (ch == 'T');
    }

    pthread_mutex_lock(&mt_mutex);

    mt_global[A_IDX] += la;
    mt_global[C_IDX] += lc;
    mt_global[G_IDX] += lg;
    mt_global[T_IDX] += lt;
    
    pthread_mutex_unlock(&mt_mutex);

    return NULL;
}

static void count_multithreaded(const char *buf, size_t n, long long counts[4], int nthreads) {
    memset(mt_global, 0, sizeof(mt_global));

    pthread_t tids[nthreads];
    ThreadArg args[nthreads];
    size_t chunk = n / nthreads;

    for (int i = 0; i < nthreads; i++) {
        args[i].start = buf + (size_t)i * chunk;
        args[i].len = (i == nthreads - 1) ? (n - (size_t)i * chunk) : chunk;

        pthread_create(&tids[i], NULL, mt_worker, &args[i]);
    
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }

    for (int k = 0; k < 4; k++) {
        counts[k] = mt_global[k];
    }
}

static inline long long collapse_epi64(__m256i acc) {
    __m128i lo = _mm256_castsi256_si128(acc);
    __m128i hi = _mm256_extracti128_si256(acc, 1);
    __m128i s = _mm_add_epi64(lo, hi);

    return _mm_cvtsi128_si64(s) + _mm_extract_epi64(s, 1);
}

static void count_simd(const char *buf, size_t n, long long counts[4]) {
    const size_t VLEN = 32;
    const __m256i zero = _mm256_setzero_si256();
    const __m256i one = _mm256_set1_epi8(1);
    __m256i va = _mm256_set1_epi8('A');
    __m256i vc = _mm256_set1_epi8('C');
    __m256i vg = _mm256_set1_epi8('G');
    __m256i vt = _mm256_set1_epi8('T');
    __m256i acc_a = zero, acc_c = zero, acc_g = zero, acc_t = zero;
    size_t i = 0;

    for (; i + VLEN <= n; i += VLEN) {
        __m256i chunk = _mm256_load_si256((const __m256i *)(buf + i));
        acc_a = _mm256_add_epi64(acc_a, _mm256_sad_epu8(_mm256_and_si256(_mm256_cmpeq_epi8(chunk, va), one), zero));
        acc_c = _mm256_add_epi64(acc_c, _mm256_sad_epu8(_mm256_and_si256(_mm256_cmpeq_epi8(chunk, vc), one), zero));
        acc_g = _mm256_add_epi64(acc_g, _mm256_sad_epu8(_mm256_and_si256(_mm256_cmpeq_epi8(chunk, vg), one), zero));
        acc_t = _mm256_add_epi64(acc_t, _mm256_sad_epu8(_mm256_and_si256(_mm256_cmpeq_epi8(chunk, vt), one), zero));
    }

    counts[A_IDX] = collapse_epi64(acc_a);
    counts[C_IDX] = collapse_epi64(acc_c);
    counts[G_IDX] = collapse_epi64(acc_g);
    counts[T_IDX] = collapse_epi64(acc_t);

    for (; i < n; i++) {
        char ch = buf[i];
        counts[A_IDX] += (ch == 'A');
        counts[C_IDX] += (ch == 'C');
        counts[G_IDX] += (ch == 'G');
        counts[T_IDX] += (ch == 'T');
    }
}

typedef struct {
    const char *start;
    size_t len;
    long long counts[4];
} SimdThreadArg;

static void *simd_mt_worker(void *arg) {
    SimdThreadArg *a = (SimdThreadArg *)arg;

    count_simd(a->start, a->len, a->counts);
    
    return NULL;
}

static void count_simd_mt(const char *buf, size_t n, long long counts[4], int nthreads) {
    pthread_t tids[nthreads];
    SimdThreadArg args[nthreads];

    size_t chunk = n / nthreads;

    for (int i = 0; i < nthreads; i++) {
        args[i].start = buf + (size_t)i * chunk;
        args[i].len = (i == nthreads - 1) ? (n - (size_t)i * chunk) : chunk;

        pthread_create(&tids[i], NULL, simd_mt_worker, &args[i]);
    }

    memset(counts, 0, 4 * sizeof(long long));

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);

        for (int k = 0; k < 4; k++) {
            counts[k] += args[i].counts[k];
        }
    }
}

static int verify(const long long ref[4], const long long got[4], const char *label) {
    for (int k = 0; k < 4; k++) {
        if (ref[k] != got[k]) {
            fprintf(stderr, "  [FAIL] %s: index %d expected %lld got %lld\n", label, k, ref[k], got[k]);

            return 0;
        }
    }

    return 1;
}

int main() {
    printf("DNA size: %llu MB\n", (unsigned long long)DNA_SIZE_MB);
    printf("Threads used: %d\n\n", NUM_THREADS);

    dna_buf = (char *)aligned_alloc(64, DNA_SIZE);

    if (!dna_buf) { 
        perror("aligned_alloc");
        
        return 1;
    }

    fflush(stdout);
    
    generate_dna(dna_buf, DNA_SIZE);

    long long ref[4], res[4];
    double t0, t1;

    t0 = now_sec();

    count_scalar(dna_buf, DNA_SIZE, ref);
    
    t1 = now_sec();
    double scalar_time = t1 - t0;

    printf("Counts (A C G T):\n %lld %lld %lld %lld\n\n", ref[A_IDX], ref[C_IDX], ref[G_IDX], ref[T_IDX]);
    printf("%-36s %.3f sec\n", "Scalar time:", scalar_time);

    t0 = now_sec();

    count_multithreaded(dna_buf, DNA_SIZE, res, NUM_THREADS);
    
    t1 = now_sec();
    double mt_time = t1 - t0;

    if (verify(ref, res, "Multithreading")) {
        printf("%-36s %.3f sec (speedup: %.2fx)\n", "Multithreading time:", mt_time, scalar_time / mt_time);
    }

    t0 = now_sec();

    count_simd(dna_buf, DNA_SIZE, res);
    
    t1 = now_sec();
    double simd_time = t1 - t0;

    if (verify(ref, res, "SIMD")) {
        printf("%-36s %.3f sec (speedup: %.2fx)\n", "SIMD time:", simd_time, scalar_time / simd_time);
    }

    t0 = now_sec();

    count_simd_mt(dna_buf, DNA_SIZE, res, NUM_THREADS);
    
    t1 = now_sec();
    double simd_mt_time = t1 - t0;

    if (verify(ref, res, "SIMD+MT")) {
        printf("%-36s %.3f sec (speedup: %.2fx)\n", "SIMD + Multithreading time:", simd_mt_time, scalar_time / simd_mt_time);
    }

    free(dna_buf);

    return 0;
}

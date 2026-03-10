#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <immintrin.h>

#define BUF_SIZE_MB 256ULL
#define BUF_SIZE (BUF_SIZE_MB * 1024ULL * 1024ULL)
#define NUM_THREADS 4

static inline double now_sec() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static const char charset[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "!@#$%^&*() ,.-;:'\"/";

static void generate_buf(char *buf, size_t n) {
    unsigned int seed = 0xdeadbeef;

    size_t nchars = sizeof(charset) - 1;

    for (size_t i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        buf[i] = charset[(seed >> 16) % nchars];
    }
}

static void toupper_scalar(char *buf, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char c = buf[i];

        if (c >= 'a' && c <= 'z') {
            buf[i] = c & ~0x20;
        }
    }
}

typedef struct {
    char *start;
    size_t len;
} Chunk;

static void *mt_worker(void *arg) {
    Chunk *c = (Chunk *)arg;

    toupper_scalar(c->start, c->len);
    
    return NULL;
}

static void toupper_mt(char *buf, size_t n, int nthreads) {
    pthread_t tids[nthreads];
    Chunk args[nthreads];
    size_t chunk = n / nthreads;

    for (int i = 0; i < nthreads; i++) {
        args[i].start = buf + (size_t)i * chunk;
        args[i].len = (i == nthreads - 1) ? (n - (size_t)i * chunk) : chunk;

        pthread_create(&tids[i], NULL, mt_worker, &args[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
}

static void toupper_simd(char *buf, size_t n) {
    const size_t VLEN = 32;

    const __m256i a_minus1 = _mm256_set1_epi8('a' - 1);
    const __m256i z_plus1 = _mm256_set1_epi8('z' + 1);
    const __m256i bit5 = _mm256_set1_epi8(0x20);

    size_t i = 0;

    for (; i + VLEN <= n; i += VLEN) {
        __m256i chunk = _mm256_load_si256((const __m256i *)(buf + i));

        __m256i lo_mask = _mm256_cmpgt_epi8(chunk, a_minus1);
        __m256i hi_mask = _mm256_cmpgt_epi8(z_plus1, chunk);
        __m256i lower_mask = _mm256_and_si256(lo_mask, hi_mask);

        __m256i flip = _mm256_and_si256(lower_mask, bit5);
        __m256i result = _mm256_xor_si256(chunk, flip);

        _mm256_store_si256((__m256i *)(buf + i), result);
    }

    for (; i < n; i++) {
        char c = buf[i];
        
        if (c >= 'a' && c <= 'z') {
            buf[i] = c & ~0x20;
        }
    }
}

static void *simd_mt_worker(void *arg) {
    Chunk *c = (Chunk *)arg;

    toupper_simd(c->start, c->len);
    
    return NULL;
}

static void toupper_simd_mt(char *buf, size_t n, int nthreads) {
    pthread_t tids[nthreads];
    Chunk args[nthreads];
    size_t chunk = n / nthreads;

    for (int i = 0; i < nthreads; i++) {
        args[i].start = buf + (size_t)i * chunk;
        args[i].len = (i == nthreads - 1) ? (n - (size_t)i * chunk) : chunk;

        pthread_create(&tids[i], NULL, simd_mt_worker, &args[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
}

static int verify(const char *ref, const char *got, size_t n, const char *label) {
    for (size_t i = 0; i < n; i++) {
        if (ref[i] != got[i]) {
            fprintf(stderr, "  [FAIL] %s: first diff at %zu: expected '%c' got '%c'\n", label, i, ref[i], got[i]);

            return 0;
        }
    }

    return 1;
}

int main(void) {
    printf("Buffer size: %llu MB\n", (unsigned long long)BUF_SIZE_MB);
    printf("Threads used: %d\n\n", NUM_THREADS);

    char *src = (char *)aligned_alloc(64, BUF_SIZE);
    char *buf_mt = (char *)aligned_alloc(64, BUF_SIZE);
    char *buf_si = (char *)aligned_alloc(64, BUF_SIZE);
    char *buf_sm = (char *)aligned_alloc(64, BUF_SIZE);
    char *ref = (char *)aligned_alloc(64, BUF_SIZE);

    if (!src || !buf_mt || !buf_si || !buf_sm || !ref) {
        perror("aligned_alloc");

        free(src);
        free(buf_mt);
        free(buf_si);
        free(buf_sm);
        free(ref);
        
        return 1;
    }

    fflush(stdout);
    
    generate_buf(src, BUF_SIZE);

    memcpy(ref, src, BUF_SIZE);

    toupper_scalar(ref, BUF_SIZE);

    double t0, t1;

    memcpy(buf_mt, src, BUF_SIZE);
    
    t0 = now_sec();
    
    toupper_mt(buf_mt, BUF_SIZE, NUM_THREADS);
    
    t1 = now_sec();
    double mt_time = t1 - t0;
    
    if (verify(ref, buf_mt, BUF_SIZE, "Multithreading")) {
        printf("%-30s %.3f sec\n", "Multithreading time:", mt_time);
    }

    memcpy(buf_si, src, BUF_SIZE);

    t0 = now_sec();
    
    toupper_simd(buf_si, BUF_SIZE);
    
    t1 = now_sec();
    double simd_time = t1 - t0;
    
    if (verify(ref, buf_si, BUF_SIZE, "SIMD")) {
        printf("%-30s %.3f sec\n", "SIMD time:", simd_time);
    }

    memcpy(buf_sm, src, BUF_SIZE);
    
    t0 = now_sec();
    
    toupper_simd_mt(buf_sm, BUF_SIZE, NUM_THREADS);
    
    t1 = now_sec();
    double simd_mt_time = t1 - t0;
    
    if (verify(ref, buf_sm, BUF_SIZE, "SIMD+MT")) {
        printf("%-30s %.3f sec\n", "SIMD + Multithreading:", simd_mt_time);
    }

    free(src);
    free(buf_mt);
    free(buf_si);
    free(buf_sm);
    free(ref);
    
    return 0;
}

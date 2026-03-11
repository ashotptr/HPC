#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <immintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

#define NUM_THREADS 4

static inline double now_sec(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

typedef struct {
    uint8_t *data;
    int width;
    int height;
} Image;

static size_t img_bytes(const Image *im) {
    return (size_t)im->width * im->height * 3;
}

static Image img_alloc(int w, int h) {
    Image im;
    im.width = w;
    im.height = h;
    im.data = (uint8_t *)aligned_alloc(64, img_bytes(&im) + 16);

    return im;
}

static void img_free(Image *im) { 
    free(im->data);
    
    im->data = NULL; 
}

static int ppm_write(const char *path, const Image *im) {
    FILE *f = fopen(path, "wb");

    if (!f) {
        return 0;
    }

    fprintf(f, "P6\n%d %d\n255\n", im->width, im->height);
    
    fwrite(im->data, 1, img_bytes(im), f);
    
    fclose(f);
    
    return 1;
}

static void ppm_skip(FILE *f) {
    int c;
    
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(f)) != EOF && c != '\n');
        }
        else if (!isspace(c)) {
            ungetc(c, f);
            
            return;
        }
    }
}

static int ppm_read_int(FILE *f, int *out) {
    ppm_skip(f);
    
    return fscanf(f, "%d", out) == 1;
}

static int ppm_read(const char *path, Image *out) {
    FILE *f = fopen(path, "rb");

    if (!f) {
        return 0;
    }

    char magic[2];

    ppm_skip(f);

    if (fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6') {
        fclose(f);
        
        return 0;
    }

    int w, h, maxval;

    if (!ppm_read_int(f, &w) || !ppm_read_int(f, &h) || !ppm_read_int(f, &maxval)) {
        fclose(f);
        
        return 0;
    }

    fgetc(f);

    *out = img_alloc(w, h);

    if (fread(out->data, 1, img_bytes(out), f) != img_bytes(out)) {
        img_free(out);

        fclose(f);

        return 0;
    }
    
    fclose(f);
    
    return 1;
}

static void img_generate(Image *im) {
    unsigned int seed = 0xdeadbeef;
    size_t n = img_bytes(im);

    for (size_t i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        im->data[i] = (uint8_t)(seed >> 24);
    }
}

static void gray_scalar_chunk(const uint8_t *in, uint8_t *out, size_t npixels) {
    for (size_t i = 0; i < npixels; i++) {
        uint8_t gray = (uint8_t)roundf(0.299f * in[0] + 0.587f * in[1] + 0.114f * in[2]);
        out[0] = out[1] = out[2] = gray;
        in += 3;
        out += 3;
    }
}

static void gray_scalar(const Image *src, Image *dst) {
    gray_scalar_chunk(src->data, dst->data, (size_t)src->width * src->height);
}

static void gray_simd_chunk(const uint8_t *in, uint8_t *out, size_t npixels) {
    const __m256i wr = _mm256_set1_epi16(77);
    const __m256i wg = _mm256_set1_epi16(150);
    const __m256i wb = _mm256_set1_epi16(29);
    const __m256i zero = _mm256_setzero_si256();

    const __m128i shuf_r4 = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 6, 3, 0);
    const __m128i shuf_g4 = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 7, 4, 1);
    const __m128i shuf_b4 = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 8, 5, 2);

    const __m128i shuf_lo = _mm_set_epi8(-1, -1, -1, -1, 3, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0);
    const __m128i shuf_hi = _mm_set_epi8(-1, -1, -1, -1, 7, 7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 4);

    size_t i = 0;

    for (; i + 8 <= npixels; i += 8) {
        __m128i lo = _mm_loadu_si128((const __m128i *)(in + i * 3));
        __m128i hi = _mm_loadu_si128((const __m128i *)(in + i * 3 + 12));

        __m256i r16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi32(_mm_shuffle_epi8(lo, shuf_r4), _mm_shuffle_epi8(hi, shuf_r4)));
        __m256i g16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi32(_mm_shuffle_epi8(lo, shuf_g4), _mm_shuffle_epi8(hi, shuf_g4)));
        __m256i b16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi32(_mm_shuffle_epi8(lo, shuf_b4), _mm_shuffle_epi8(hi, shuf_b4)));

        __m256i gray16 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_mullo_epi16(r16, wr), _mm256_mullo_epi16(g16, wg)), _mm256_mullo_epi16(b16, wb)), 8);

        __m128i gray8 = _mm_packus_epi16(_mm256_castsi256_si128(gray16), _mm256_extracti128_si256(gray16, 1));

        __m128i out_lo = _mm_shuffle_epi8(gray8, shuf_lo);
        __m128i out_hi = _mm_shuffle_epi8(gray8, shuf_hi);

        _mm_storel_epi64((__m128i *)(out + i * 3), out_lo);
        
        *((uint32_t *)(out + i * 3 + 8)) = (uint32_t)_mm_extract_epi32(out_lo, 2);

        _mm_storel_epi64((__m128i *)(out + i * 3 + 12), out_hi);
        
        *((uint32_t *)(out + i * 3 + 20)) = (uint32_t)_mm_extract_epi32(out_hi, 2);
    }

    gray_scalar_chunk(in + i * 3, out + i * 3, npixels - i);
}

static void gray_simd(const Image *src, Image *dst) {
    gray_simd_chunk(src->data, dst->data, (size_t)src->width * src->height);
}

typedef struct {
    const uint8_t *in;
    uint8_t *out;
    size_t npixels;
} Chunk;

static void *mt_worker(void *arg) {
    Chunk *c = (Chunk *)arg;

    gray_scalar_chunk(c->in, c->out, c->npixels);

    return NULL;
}

static void gray_mt(const Image *src, Image *dst, int nthreads) {
    pthread_t tids[nthreads];
    Chunk args[nthreads];
    size_t total = (size_t)src->width * src->height;
    size_t chunk = total / nthreads;

    for (int i = 0; i < nthreads; i++) {
        size_t offset = (size_t)i * chunk;
        size_t len = (i == nthreads - 1) ? (total - offset) : chunk;
        args[i].in = src->data + offset * 3;
        args[i].out = dst->data + offset * 3;
        args[i].npixels = len;

        pthread_create(&tids[i], NULL, mt_worker, &args[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
}

static void *simd_mt_worker(void *arg) {
    Chunk *c = (Chunk *)arg;

    gray_simd_chunk(c->in, c->out, c->npixels);
    
    return NULL;
}

static void gray_simd_mt(const Image *src, Image *dst, int nthreads) {
    pthread_t tids[nthreads];
    Chunk args[nthreads];

    size_t total = (size_t)src->width * src->height;
    size_t chunk = total / nthreads;

    for (int i = 0; i < nthreads; i++) {
        size_t offset = (size_t)i * chunk;
        size_t len = (i == nthreads - 1) ? (total - offset) : chunk;
        args[i].in = src->data + offset * 3;
        args[i].out = dst->data + offset * 3;
        args[i].npixels = len;

        pthread_create(&tids[i], NULL, simd_mt_worker, &args[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
}

static int verify(const Image *ref, const Image *got, const char *label) {
    size_t n = img_bytes(ref);

    for (size_t i = 0; i < n; i++) {
        int diff = (int)ref->data[i] - (int)got->data[i];

        if (diff < -1 || diff > 1) {
            fprintf(stderr, "  [FAIL] %s at byte %zu: expected %u got %u\n", label, i, ref->data[i], got->data[i]);

            return 0;
        }
    }

    return 1;
}

int main(int argc, char **argv) {
    const char *in_path = (argc > 1) ? argv[1] : NULL;
    const char *out_path = "gray_output.ppm";

    Image src = {0};

    if (in_path) {
        if (!ppm_read(in_path, &src)) {
            fprintf(stderr, "Failed to read %s\n", in_path);

            return 1;
        }

        printf("Loaded image: %s\n", in_path);
    }
    else {
        src = img_alloc(3840, 2160);
        
        img_generate(&src);

        printf("Generated synthetic image.\n");
    }

    printf("Image size: %d x %d\n", src.width, src.height);
    printf("Threads used: %d\n\n", NUM_THREADS);

    Image dst_sc = img_alloc(src.width, src.height);
    Image dst_si = img_alloc(src.width, src.height);
    Image dst_mt = img_alloc(src.width, src.height);
    Image dst_sm = img_alloc(src.width, src.height);

    double t0, t1;

    t0 = now_sec();

    gray_scalar(&src, &dst_sc);
    
    t1 = now_sec();
    
    printf("%-30s %.3f sec\n", "Scalar time:", t1 - t0);

    t0 = now_sec();

    gray_simd(&src, &dst_si);
    
    t1 = now_sec();
    
    printf("%-30s %.3f sec\n", "SIMD time:", t1 - t0);

    t0 = now_sec();

    gray_mt(&src, &dst_mt, NUM_THREADS);

    t1 = now_sec();

    printf("%-30s %.3f sec\n", "Multithreading time:", t1 - t0);

    t0 = now_sec();

    gray_simd_mt(&src, &dst_sm, NUM_THREADS);

    t1 = now_sec();

    printf("%-30s %.3f sec\n", "Multithreading + SIMD time:", t1 - t0);

    int ok = 1;

    ok &= verify(&dst_sc, &dst_si, "SIMD");
    ok &= verify(&dst_sc, &dst_mt, "Multithreading");
    ok &= verify(&dst_sc, &dst_sm, "Multithreading+SIMD");

    printf("\nVerification: %s\n", ok ? "PASSED" : "FAILED");

    if (ppm_write(out_path, &dst_sc)) {
        printf("Output image: %s\n", out_path);
    }

    img_free(&src);
    img_free(&dst_sc);
    img_free(&dst_si);
    img_free(&dst_mt);
    img_free(&dst_sm);

    return 0;
}

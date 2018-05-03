/**
 * Utility functions.
 */

#ifndef DCCS_UTIL_H
#define DCCS_UTIL_H

#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#define USE_RDTSC 0

#define BILLION 1000000000UL

/* Debug functions */

#define DEBUG 1
#define debug(...) if (DEBUG) fprintf(stderr, __VA_ARGS__)
#define sys_error(...) fprintf(stderr, __VA_ARGS__)
#define sys_warning(...) fprintf(stderr, __VA_ARGS__)

void debug_bin(void *buf, size_t length) {
#if DEBUG
    unsigned char *c;
    for (size_t i = 0; i < length; i++) {
        c = (unsigned char *)buf + i;
        fprintf(stderr, "%02x", *c);
    }
#endif
}

char *bin_to_hex_string(void *buf, size_t length) {
    char *s = malloc(2 * length + 1);
    unsigned char *c;
    char temp[3];
    for (size_t i = 0; i < length; i++) {
        c = (unsigned char *)buf + i;
        sprintf(temp, "%02x", *c);
        strncpy(s + 2 * i, temp, 2);
    }

    s[2 * length] = '\0';
    return s;
}

/* Timing functions */

#define CPU_TO_USE 0

void set_cpu_affinity() {
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(CPU_TO_USE, &set);
  if (sched_setaffinity(0, sizeof(set), &set) == -1) {
    perror("sched_setaffinity failed");
    exit(EXIT_FAILURE);
  }
}

#if defined (__x86_64__) || defined(__i386__)
/* Note: only x86 CPUs which have rdtsc instruction are supported. */
static inline uint64_t get_cycles()
{
#if USE_RDTSC
    unsigned low, high;
    unsigned long long val;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    val = high;
    val = (val << 32) | low;
    return val;
#else
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return (uint64_t)time.tv_sec * BILLION + (uint64_t)time.tv_nsec;
#endif
}

uint64_t get_clock_rate() {
#if USE_RDTSC
    unsigned start_low, start_high, end_low, end_high;
    uint64_t start, end;

    asm volatile ("CPUID\n\t"
        "RDTSC\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t": "=r" (start_high), "=r" (start_low)::"%rax", "%rbx", "%rcx", "%rdx"
    );

    sleep(1);

    asm volatile ("RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "CPUID\n\t": "=r" (end_high), "=r" (end_low)::"%rax", "%rbx", "%rcx", "%rdx"
    );

    start = ((uint64_t) start_high) << 32 | start_low;
    end = ((uint64_t) end_high) << 32 | end_low;

    if (start >= end) {
        fprintf(stderr, "Invalid rdtsc/rdtscp results %lu, %lu.", end, start);
        exit(EXIT_FAILURE);
    } else {
        return end - start;
    }
#else
    return (uint64_t)BILLION;
#endif
}

#endif

double get_time_in_microseconds(uint64_t cycles) {
#if USE_RDTSC
    return (double)cycles / CPU_CLOCK_RATE * 1e6;
#else
    return (double)cycles / 1e3;
#endif
}

int compare_double(const void *a, const void *b)
{
    double da = *(double *)a;
    double db = *(double *)b;
    if (da < db) {
        return -1;
    } else if (da > db) {
        return 1;
    } else {
        return 0;
    }
}

void sort_latencies(double *latencies, size_t count) {
    qsort(latencies, count, sizeof(double), compare_double);
}

/* OpenSSL hashing functions */

/**
 * Call malloc() and fill the memory with random data.
 */
void *malloc_random(size_t size) {
    void *buf;

    if (posix_memalign(&buf, CACHE_LINE_SIZE, size) != 0) {
        perror("posix_memalign");
        sys_warning("Failed to malloc aligned memory, falling back to malloc() ...\n");
        buf = malloc(size);
    }

    if (buf == NULL)
        return buf;

    srand((unsigned int)time(NULL));
    for (size_t n = 0; n < size; n++) {
        *((char *)buf + n) = (char)rand();
    }

    return buf;
}

/**
 * Calculate the SHA1 digest of the given data.
 */
void sha1sum(const void *data, size_t length, unsigned char digest[SHA_DIGEST_LENGTH]) {
    SHA1(data, length, digest);
}

/**
 * Calculate the SHA1 digest of the given array of data.
 */
void sha1sum_array(const void *array[], size_t count, size_t length, unsigned char digest[SHA_DIGEST_LENGTH]) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    for (size_t n = 0; n < count; n++) {
        const void *data = array[n];
        SHA1_Update(&ctx, data, length);
    }

    SHA1_Final(digest, &ctx);
}

#endif // DCCS_UTIL_H

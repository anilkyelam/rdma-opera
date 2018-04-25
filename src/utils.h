/**
 * Utility functions.
 */

#ifndef DCCS_UTIL_H
#define DCCS_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/sha.h>

/* Debug functions */

#define DEBUG 1
#define debug(...) if (DEBUG) fprintf(stderr, __VA_ARGS__)
#define sys_error(...) fprintf(stderr, __VA_ARGS__)
#define sys_warning(...) fprintf(stderr, __VA_ARGS__)

void debug_bin(void *buf, size_t length) {
#if DEBUG
    unsigned char *c;
    for (size_t i = 0; i < length; i++) {
        c = buf + i;
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

#if defined (__x86_64__) || defined(__i386__)
/* Note: only x86 CPUs which have rdtsc instruction are supported. */
static inline uint64_t get_cycles()
{
    unsigned low, high;
    unsigned long long val;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    val = high;
    val = (val << 32) | low;
    return val;
}

uint64_t get_clock_rate() {
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
}

#endif

double get_time_in_microseconds(uint64_t cycles) {
    return (double)cycles / CPU_CLOCK_RATE * 1e6;
}

int compare_double(const void *a, const void *b)
{
    return (*(double*)a - *(double*)b);
}

void sort_latencies(double *latencies, size_t count) {
    qsort(latencies, count, sizeof(double), compare_double);
}

/* OpenSSL hashing functions */

/**
 * Call malloc() and fill the memory with random data.
 */
void *malloc_random(size_t size) {
    void *buf = malloc(size);
    if (buf == NULL)
        return buf;

    srand(time(NULL));
    for (size_t n = 0; n < size; n++) {
        *(char *)(buf + n) = rand();
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


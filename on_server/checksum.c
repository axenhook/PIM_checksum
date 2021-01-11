// compile: gcc -o checksum -Wall -O3 ./checksum.c

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

static void init_data(unsigned int *dat, unsigned int size)
{
    srand(time(NULL));

    for (unsigned int i = 0; i < size; i++)
    {
        dat[i] = rand();
    }

    return;
}

unsigned int calc_checksum(unsigned int *dat, unsigned int size)
{
    unsigned int checksum = 0;

    for (unsigned int i = 0; i < size; i++)
    {
        checksum += dat[i];
    }

    return checksum;
}

static inline unsigned long long get_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec * 1e9 + t.tv_nsec);
}

int main(int argc, char **argv)
{
    unsigned int checksum;
    unsigned long long ns;
    time_t t;

    if (argc < 2)
    {
        printf("usage: %s size_MB\n", argv[0]);
        return -1;
    }

    unsigned int size = (unsigned int)atoi(argv[1]) * 1024 * 1024;
    assert(size > 0);

    unsigned int *dat = malloc(size);
    assert(dat != NULL);

    init_data(dat, size / sizeof(unsigned int));

    while (1)
    {
        ns = get_ns();
        checksum = calc_checksum(dat, size / sizeof(unsigned int));
        ns = get_ns() - ns;
        time(&t);
        printf("data size: %u bytes, checksum: %u, time: %llu ns, speed: %llu MB/s, time: %s", size, checksum, ns, (unsigned long long )(((unsigned long long)size) / 1024 / 1024 * 1e9 / ns), ctime(&t));
    }
}

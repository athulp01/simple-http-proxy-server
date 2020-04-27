/* 
 * E-way set-associative cache simulator
 * LRU is the eviction policy
 * Cache is represented as a 2D array of cache lines
 * Uses valgrind style memory trace as input
*/

#define  _GNU_SOURCE
#include "cachelab.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>

#define CACHE_HIT 1
#define CACHE_MISS 1<<1
#define CACHE_EVICTION 1<<2

#define ADDR_LEN sizeof(long long)*8    // set maximum input address bit length to 8 byte

#define printf if(verbose) printf       //lazy to replace

int num_of_lines;                       // number of cache lines in a set
int num_of_sets, num_of_blocks;
int verbose=0;                          // verbose flag

int misses=0, hits=0, evictions=0;      //Keep track of the counters

char *trace_file;

long counter = 0;                       //used for implementing lru

struct cache_line{
    int tag;
    long counter;
    char valid;
}*cache;

void evict(struct cache_line *cache, int set, int tag) {
    int lru_idx = 0;
    long min = LONG_MAX;
    for(int i=0; i<num_of_lines; i++) {
        if(cache[i].counter <= min) {
            min = cache[i].counter;
            lru_idx = i;
        }
    }
    cache[lru_idx].tag = tag;
    cache[lru_idx].valid = 1;
    cache[lru_idx].counter = ++counter;
}

void cache_search(struct cache_line *cache, int set, int tag) {
    cache += (set*num_of_lines);
    int empty_idx = -1;
    for(int i=0; i<num_of_lines; i++) {
        if(cache[i].valid == 0) empty_idx = i;
        if(cache[i].tag == tag && cache[i].valid == 1) {
            cache[i].counter = ++counter;
            printf(" hit");
            hits++;
            return;
        }
    }
    printf("miss ");
    if(empty_idx == -1) {
        evictions++;
        evict(cache, set, tag);
        printf("eviction ");
    }else {
        cache[empty_idx].tag = tag;
        cache[empty_idx].valid = 1;
        cache[empty_idx].counter = ++counter;
    }
    misses++;
    return;
}

void set_opts(int argc, char *argv[]) {
    int opt;
    while((opt = getopt(argc, argv, "s:E:b:t:v"))!=-1) {
        switch (opt)
        {
        case 's':
            num_of_sets = atoi(optarg);
            break;
        case 'E':
            num_of_lines = atoi(optarg);
            break;
        case 'b':
            num_of_blocks = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbose = 1;
        default:
            break;
        }
    }
}

void parse_trace(char *filename) {
    FILE *file = fopen(filename, "r");
    char mode, *line=NULL;
    unsigned long long addr;
    int set ,tag, block;
    size_t len=0;
    while(-1 != getline(&line, &len, file)) {
        if(line[0] == 'I') continue;
        line[strcspn(line, "\n")] = 0;
        printf("%s ",line);
        sscanf(line, " %c %llx, %d", &mode, &addr, &block);
        unsigned long long mask = (ULLONG_MAX>>(ADDR_LEN-num_of_sets));
        set = (addr>>num_of_blocks) & mask;
        mask = (ULLONG_MAX>>(ADDR_LEN-(num_of_sets+num_of_blocks)));
        tag = (addr>>(num_of_sets + num_of_blocks)) & mask;
        switch(mode) {
            case 'L':
            case 'S':
                cache_search(cache, set, tag);
                printf("\n");
                break;
            case 'M':
                cache_search(cache, set, tag);
                cache_search(cache, set, tag);
                printf("\n");
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    set_opts(argc, argv);
    size_t size = num_of_lines * (1<<num_of_sets) * sizeof(struct cache_line);
    cache = malloc(size);
    memset(cache, 0, size);
    parse_trace(trace_file);
    printSummary(hits, misses, evictions);
    return 0;
}

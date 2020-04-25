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

#define ADDR_LEN 31

#define SET_INDEX(addr, set_size, block_size) \
        addr & ((-1>>(ADDR_LEN-set_size))<<block_size)
#define BLOCK_OFFSET(addr, block_size) \
        (addr & (-1>>(ADDR_LEN - block_size)))
#define TAG(addr, set_size, block_size) \
        addr & ((-1>>(ADDR_LEN-(set_size+block_size))<<(set_size + block_size)

int num_of_lines;
int num_of_sets, num_of_blocks;
int misses=0, hits=0, evictions=0;
char *trace_file;

struct cache_line{
    int tag, count;
    char valid;
}*cache;

void evict(struct cache_line *cache, int set, int tag) {
    int lru_idx = 0, min = INT_MAX;
    for(int i=0; i<num_of_lines; i++) {
        if(cache[i].count <= min) {
            min = cache[i].count;
            lru_idx = i;
        }
    }
    cache[lru_idx].tag = tag;
    cache[lru_idx].valid = 1;
    cache[lru_idx].count = 1;
}

void cache_search(struct cache_line *cache, int set, int tag) {
    cache += (set*num_of_lines);
    int empty_idx = -1;
    for(int i=0; i<num_of_lines; i++) {
        if(cache[i].valid == 0) empty_idx = i;
        if(cache[i].tag == tag && cache[i].valid == 1) {
            cache[i].count++;
            printf(" hit\n");
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
        cache[empty_idx].count = 1;
    }
    printf("\n");
    misses++;
    return;
}

void set_opts(int argc, char *argv[]) {
    int opt;
    while((opt = getopt(argc, argv, "s:E:b:t:"))!=-1) {
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
        default:
            break;
        }
    }
}

void parse_trace(char *filename) {
    FILE *file = fopen(filename, "r");
    char mode, *line=NULL;
    int addr, set ,tag, block;
    size_t len=0;
    while(-1 != getline(&line, &len, file)) {
        if(line[0] == 'I') continue;
        line[strcspn(line, "\n")] = 0;
        printf("%s ",line);
        sscanf(line, " %c %x, %d", &mode, &addr, &block);
        unsigned int tmp = ~(INT_MIN>>(ADDR_LEN-num_of_sets));
        unsigned int mask = tmp<<num_of_blocks;
        set = addr & mask;
        set >>= num_of_blocks;
        tmp = ~(INT_MIN>>(ADDR_LEN-(num_of_sets+num_of_blocks)));
        mask = (tmp<<(num_of_sets + num_of_blocks));
        tag = addr & mask;
        tag >>= (num_of_blocks + num_of_sets);
        switch(mode) {
            case 'L':
            case 'S':
                cache_search(cache, set, tag);
                break;
            case 'M':
                cache_search(cache, set, tag);
                cache_search(cache, set, tag);
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    set_opts(argc, argv);
    size_t size = num_of_lines * 1<<num_of_sets * sizeof(struct cache_line);
    cache = malloc(size);
    memset(cache, 0, size);
    parse_trace(trace_file);
    printSummary(hits, misses, evictions);
    return 0;
}

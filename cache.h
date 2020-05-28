#ifndef CACHE_H
#define CACHE_H

#include "./uthash.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct web_object {
    char *url;
    char *data;
    int size;
    UT_hash_handle hh;
};

extern struct web_object *cache;

struct web_object* search_cache(char *url);
void add_to_cache(char *url, char *data, int bytes);

#endif

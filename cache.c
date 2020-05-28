#include "cache.h"
#include <string.h>
#include <stdlib.h>

struct web_object *cache = NULL;

struct web_object* search_cache(char *url) {
    struct web_object* entry;
    HASH_FIND_STR(cache, url, entry);
    if(entry) {
        HASH_DELETE(hh, cache, entry);
        HASH_ADD_KEYPTR(hh, cache, entry->url, strlen(entry->url), entry);
        return entry;
    }
    return NULL;
}

void add_to_cache(char *url, char *data, int bytes) {
    struct web_object *entry, *tmp;
    entry = malloc(sizeof(struct web_object));
    entry->url = strdup(url);
    entry->data = malloc(bytes);
    entry->size = bytes;
    memcpy(entry->data, data, bytes);
    HASH_ADD_KEYPTR(hh, cache, entry->url, strlen(entry->url), entry);
    if(HASH_COUNT(cache) >= MAX_CACHE_SIZE) {
        HASH_ITER(hh, cache, entry, tmp) {
            HASH_DELETE(hh, cache, entry);
            free(entry->url);
            free(entry->data);
            free(entry);
            break;
        }
    }
}

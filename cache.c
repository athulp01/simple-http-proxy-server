#include "cache.h"
#include "uthash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


struct web_object *cache = NULL;

struct web_object* search_cache(char *url) {
    if(!url) return NULL;
    struct web_object* entry;
    if(pthread_rwlock_rdlock(&lock) != 0) uthash_fatal("can't get readlock");
    HASH_FIND_STR(cache, url, entry);
    pthread_rwlock_unlock(&lock);
    if(entry) {
        if(pthread_rwlock_wrlock(&lock) != 0) uthash_fatal("can't get writelock");
        HASH_DELETE(hh, cache, entry);
        HASH_ADD_KEYPTR(hh, cache, entry->url, strlen(entry->url), entry);
        pthread_rwlock_unlock(&lock);
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
    if(pthread_rwlock_wrlock(&lock) != 0) uthash_fatal("can't get writelock");
    HASH_ADD_KEYPTR(hh, cache, entry->url, strlen(entry->url), entry);
    pthread_rwlock_unlock(&lock);
    if(HASH_COUNT(cache) >= MAX_CACHE_SIZE) {
        if(pthread_rwlock_rdlock(&lock) != 0) uthash_fatal("can't get readlock");
        HASH_ITER(hh, cache, entry, tmp) {
            if(pthread_rwlock_wrlock(&lock) != 0) uthash_fatal("can't get writelock");
            HASH_DELETE(hh, cache, entry);
            pthread_rwlock_unlock(&lock);
            free(entry->url);
            free(entry->data);
            free(entry);
            break;
        }
        pthread_rwlock_unlock(&lock);
    }
}

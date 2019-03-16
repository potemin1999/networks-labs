/**
 * Created by ilya on 3/8/19.
 */

#ifndef NETWORKS_LABS_STORAGE_H
#define NETWORKS_LABS_STORAGE_H

#include "node.h"

struct map_entry {
    uint32_t key;
    uint8_t in_use;
    node_t node;
};
typedef struct map_entry map_entry_t;

struct map {
    float max_load_factor;
    uint32_t size;
    uint32_t capacity;
    map_entry_t *entries;
};
typedef struct map map_t;

int map_get_index_by_hash(map_t *map, uint32_t key_hash) {
    return key_hash % map->capacity;
}

int map_create(map_t *map, uint32_t initial_capacity) {
    size_t alloc_size = initial_capacity * sizeof(map_entry_t);
    map_entry_t *entries = (map_entry_t *) malloc(alloc_size);
    bzero(entries, alloc_size);
    map->entries = entries;
    map->size = 0;
    map->capacity = initial_capacity;
    return 0;
}

int map_destroy(map_t *map) {
    if (map->entries)
        free(map->entries);
}

int map_increase_capacity(map_t *map, uint32_t new_capacity) {
    uint32_t old_size = map->size;
    //uint32_t old_capacity = map;
    map_entry_t *old_entries = map->entries;
    map_entry_t *new_entries = malloc(new_capacity * sizeof(map_entry_t));
    bzero(new_entries, new_capacity * sizeof(map_entry_t));
    map_entry_t *entry = old_entries;
    map->capacity = new_capacity;
    for (int i = 0; i < old_size; i++, entry++) {
        int new_index = map_get_index_by_hash(map, entry->key);
        new_entries[new_index] = *entry;
    }
}

int map_next_node(map_t *map, map_entry_t **out_pointer) {
    map_entry_t *ptr = *out_pointer;
    map_entry_t *end = &(map->entries[map->capacity]);
    ptr++;
    while (ptr <= end & ptr->in_use == 0) {
        ptr++;
    }
    if (ptr > end)
        return -1;
    *out_pointer = ptr;
    return 0;
}

struct storage_iter {
    map_t *map;
    map_entry_t *current_entry;
};

typedef struct storage_iter storage_iter_t;

map_t *storage_create() {
    map_t *map = (map_t *) malloc(sizeof(map_t));
    map_create(map, 16);
    return map;
}

int storage_destroy(map_t *map) {
    map_destroy(map);
    free(map);
}

storage_iter_t storage_new_iterator(map_t *map) {
    storage_iter_t storage_iter;
    storage_iter.map = map;
    storage_iter.current_entry = &(map->entries[-1]);
    return storage_iter;
}

node_t *storage_next(storage_iter_t *iterator) {
    map_entry_t *pointer = iterator->current_entry;
    int res = map_next_node(iterator->map, &pointer);
    if (res == -1) return 0;
    iterator->current_entry = pointer;
    return &(pointer->node);
}

int storage_node_added(map_t *map, node_t node) {
    uint32_t hash = NODE_HASH(&node);
    if (map->size * map->max_load_factor < map->capacity + 1) {
        map_increase_capacity(map, map->capacity * 2);
    }
    int index = map_get_index_by_hash(map, hash);
    int probes = 0;
    map_entry_t *entries = map->entries;
    if (entries[index].key == hash && entries[index].in_use)
        return -1;
    while (probes++ < 10 && entries[index].in_use && entries[index].key != hash) {
        index = (index + 1) % map->capacity;
    }
    if (probes == 10) return -1;
    entries[index].key = hash;
    entries[index].node = node;
    entries[index].in_use = 1;
    map->size += 1;
    return index;
}

int storage_node_removed(map_t *map, node_t node) {
    uint32_t hash = NODE_HASH(&node);
    int index = map_get_index_by_hash(map, hash);
    int probes = 0;
    map_entry_t *entries = map->entries;
    while (probes++ < 10 && entries[index].in_use && entries[index].key != hash) {
        index = (index + 1) % map->capacity;
    }
    if (probes == 10) return -1;
    if (entries[index].key != hash) return -1;
    entries[index].key = 0;
    entries[index].in_use = 0;
    map->size -= 1;
    return index;
}

#endif //NETWORKS_LABS_STORAGE_H

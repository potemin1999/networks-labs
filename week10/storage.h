/**
 * Created by ilya on 3/8/19.
 */

#ifndef NETWORKS_LABS_STORAGE_H
#define NETWORKS_LABS_STORAGE_H

#include "node.h"

#define CREATE_HASH_MAP(name, type_t)                                           \
struct name##_map_entry {                                                       \
    uint32_t key;                                                               \
    uint8_t in_use;                                                             \
    type_t value;                                                               \
};                                                                              \
typedef struct name##_map_entry name##_map_entry_t;                             \
                                                                                \
struct name##_map {                                                             \
    float max_load_factor;                                                      \
    uint32_t size;                                                              \
    uint32_t capacity;                                                          \
    name##_map_entry_t *entries;                                                \
};                                                                              \
typedef struct name##_map name##_map_t;                                         \
                                                                                \
struct name##_map_iterator {                                                    \
    name##_map_t *map;                                                          \
    name##_map_entry_t *current_entry;                                          \
};                                                                              \
typedef struct name##_map_iterator name##_map_iterator_t;                       \
                                                                                \
int name##_map_get_index_by_hash(name##_map_t *map, uint32_t key_hash) {        \
    return key_hash % map->capacity;                                            \
}                                                                               \
int name##_map_create(name##_map_t *map, uint32_t initial_capacity) {           \
    size_t alloc_size = initial_capacity * sizeof(name##_map_entry_t);          \
    name##_map_entry_t *entries = (name##_map_entry_t *) malloc(alloc_size);    \
    bzero(entries, alloc_size);                                                 \
    map->entries = entries;                                                     \
    map->size = 0;                                                              \
    map->capacity = initial_capacity;                                           \
    return 0;                                                                   \
}                                                                               \
int name##_map_destroy(name##_map_t *map) {                                     \
    if (map->entries)                                                           \
        free(map->entries);                                                     \
    return 0;                                                                   \
}                                                                               \
int name##_map_increase_capacity(name##_map_t *map, uint32_t new_capacity) {    \
    uint32_t old_size = map->size;                                              \
    name##_map_entry_t *old_entries = map->entries;                             \
    int map_size = sizeof(name##_map_entry_t);                                  \
    name##_map_entry_t *new_entries = malloc(new_capacity * map_size);          \
    bzero(new_entries, new_capacity * sizeof(name##_map_entry_t));              \
    name##_map_entry_t *entry = old_entries;                                    \
    map->capacity = new_capacity;                                               \
    for (int i = 0; i < old_size; i++, entry++) {                               \
        int new_index = name##_map_get_index_by_hash(map, entry->key);          \
        new_entries[new_index] = *entry;                                        \
    }                                                                           \
    return new_capacity;                                                        \
}                                                                               \
int name##_map_next(name##_map_t *map, name##_map_entry_t **out_pointer) {      \
    name##_map_entry_t *ptr = *out_pointer;                                     \
    name##_map_entry_t *end = &(map->entries[map->capacity]);                   \
    ptr++;                                                                      \
    while ((ptr <= end) && (ptr->in_use == 0)) {                                \
        ptr++;                                                                  \
    }                                                                           \
    if (ptr > end)                                                              \
        return -1;                                                              \
    *out_pointer = ptr;                                                         \
    return 0;                                                                   \
}                                                                               \
                                                                                \
name##_map_iterator_t name##_map_new_iterator(name##_map_t *map) {              \
    name##_map_iterator_t iter;                                                 \
    iter.map = map;                                                             \
    iter.current_entry = &(map->entries[-1]);                                   \
    return iter;                                                                \
}                                                                               \
type_t* name##_map_iterate_next(name##_map_iterator_t *iterator) {              \
    name##_map_entry_t *pointer = iterator->current_entry;                      \
    int res = name##_map_next(iterator->map, &pointer);                         \
    if (res == -1) return 0;                                                    \
    iterator->current_entry = pointer;                                          \
    return &(pointer->value);                                                   \
}                                                                               \
                                                                                \
name##_map_t* name##_map_new(){                                                 \
    name##_map_t *map = (name##_map_t *) malloc(sizeof(name##_map_t));          \
    name##_map_create(map, 256);                                                \
    return map;                                                                 \
}                                                                               \
void name##_map_delete(name##_map_t *map){                                      \
    name##_map_destroy(map);                                                    \
    free(map);                                                                  \
}                                                                               \
                                                                                \
int name##_map_put(name##_map_t *map, uint32_t key, type_t value){              \
    int index = name##_map_get_index_by_hash(map, key);                         \
    int probes = 0;                                                             \
    name##_map_entry_t *entries = map->entries;                                 \
    if (entries[index].key == key && entries[index].in_use)                     \
    return -1;                                                                  \
    while (probes++ < 10 && entries[index].in_use                               \
                         && entries[index].key != key) {                        \
        index = (index + 1) % map->capacity;                                    \
    }                                                                           \
    if (probes == 10) return -1;                                                \
    entries[index].key = key;                                                   \
    entries[index].value = value;                                               \
    entries[index].in_use = 1;                                                  \
    map->size += 1;                                                             \
    return index;                                                               \
}                                                                               \
type_t* name##_map_get(name##_map_t *map, uint32_t key){                        \
    int index = name##_map_get_index_by_hash(map, key);                         \
    int probes = 0;                                                             \
    name##_map_entry_t *entries = map->entries;                                 \
    if (entries[index].key == key && entries[index].in_use)                     \
        return &(entries[index].value);                                         \
    while (probes++ < 10 && entries[index].in_use                               \
                         && entries[index].key != key) {                        \
        index = (index + 1) % map->capacity;                                    \
    }                                                                           \
    if (probes == 10) return 0;                                                 \
    return &(entries[index].value);                                             \
}                                                                               \
int name##_map_remove(name##_map_t *map,uint32_t key,type_t value){             \
    int index = name##_map_get_index_by_hash(map, key);                         \
    int probes = 0;                                                             \
    name##_map_entry_t *entries = map->entries;                                 \
    while (probes++ < 10 && entries[index].in_use                               \
                         && entries[index].key != key) {                        \
        index = (index + 1) % map->capacity;                                    \
    }                                                                           \
    if (probes == 10) return -1;                                                \
    if (entries[index].key != key) return -1;                                   \
    entries[index].key = 0;                                                     \
    entries[index].in_use = 0;                                                  \
    map->size -= 1;                                                             \
    return index;                                                               \
}

CREATE_HASH_MAP(node, node_t)

typedef node_map_t storage_t;
typedef node_map_iterator_t storage_iter_t;
#define storage_new_iterator(storage_ptr) node_map_new_iterator(storage_ptr)
#define storage_next(iterator_ptr) node_map_iterate_next(iterator_ptr)
#define storage_create() node_map_new()
#define storage_destroy(storage_ptr) node_map_delete(storage_ptr)

int storage_node_added(node_map_t *map, node_t node) {
    return node_map_put(map, NODE_HASH(&node) , node);
}

int storage_node_removed(node_map_t *map, node_t node) {
    return node_map_remove(map, NODE_HASH(&node), node);
}

#endif //NETWORKS_LABS_STORAGE_H

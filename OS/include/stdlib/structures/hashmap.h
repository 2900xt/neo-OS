#pragma once

#include <stdlib/structures/string.h>
#include <stdlib/structures/list.h>
#include <stdlib/structures/pair.h>
#include <types.h>

namespace stdlib
{

// Hash functions
size_t hash_int(uint64_t input);
size_t hash_string(stdlib::string s);

// HashMap class template declaration
template <typename K, typename V>
class HashMap {
public:
    typedef size_t (*hash_function_t)(K);
    hash_function_t hash;
    list<list<pair<K, V>>> buckets;

    HashMap(hash_function_t hash_fn, int num_buckets);
    
    void put(K key, V val);
    V remove(K key);
    V &get(K key) const;
    bool contains(K key) const;
    V &operator[](K key) const;
};

}
#include <stdlib/structures/hashmap.h>
#include <kernel/mem/mem.h>
#include <stdlib/assert.h>

namespace stdlib
{

#define MOD (uint64_t)(1e9 + 7)
#define BASE 67

// Hash function implementations
size_t hash_int(uint64_t input)
{
    return input % MOD;
}

size_t hash_string(stdlib::string s)
{
    size_t sum = 0;
    size_t base = 1;
    for(int i = 0; i < s.length(); i++)
    {
        sum += base*s[i];
        base *= BASE;
        sum %= MOD;
        base %= MOD;
    }

    return sum;
}

// HashMap template method implementations
template <typename K, typename V>
HashMap<K, V>::HashMap(hash_function_t hash_fn, int num_buckets) : buckets(num_buckets)
{
    this->hash = hash_fn;
}

template <typename K, typename V>
void HashMap<K, V>::put(K key, V val)
{
    size_t hash_key = hash(key) % buckets.length();
    buckets[hash_key].push_back(val);
}

template <typename K, typename V>
V HashMap<K, V>::remove(K key)
{
    size_t hash_key = hash(key) % buckets.length();
    list<pair<K, V>> &bucket = buckets[hash_key];
    for(size_t i = 0; i < bucket.length(); i++)
    {
        if (bucket[i].first == key)
        {
            V val = bucket[i].second;
            bucket.erase(i);
            return val;
        }
    }
    // If key not found, handle accordingly

    assert(false && "Key not found in HashMap");
    return V();
}

template <typename K, typename V>
V &HashMap<K, V>::get(K key) const
{
    size_t hash_key = hash(key) % buckets.length();
    list<pair<K, V>> &bucket = buckets[hash_key];
    for(size_t i = 0; i < bucket.length(); i++)
    {
        if (bucket[i].first == key)
        {
            return bucket[i].second;
        }
    }
    // If key not found, handle accordingly

    assert(false && "Key not found in HashMap");
    return V();
}

template <typename K, typename V>
bool HashMap<K, V>::contains(K key) const
{
    size_t hash_key = hash(key) % buckets.length();
    list<pair<K, V>> &bucket = buckets[hash_key];
    for(size_t i = 0; i < bucket.length(); i++)
    {
        if (bucket[i].first == key)
        {
            return true;
        }
    }
    return false;
}

template <typename K, typename V>
V &HashMap<K, V>::operator[](K key) const
{
    return get(key);
}

}
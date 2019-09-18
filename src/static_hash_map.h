#pragma once

#include "deque.h"
#include "murmur_hash.h"

namespace nimble
{
template <typename T>
uint64_t create_hash(const T& key)
{
    return murmur_hash_64(&key, sizeof(T), 0);
}

template <typename KEY, typename VALUE, size_t SIZE>
class StaticHashMap
{
public:
    struct FindResult
    {
        uint32_t hash_index;
        uint32_t data_prev_index;
        uint32_t data_index;
    };

    uint32_t              m_hash[SIZE];
    Deque<uint32_t, SIZE> m_free_indices;
    uint64_t              m_key[SIZE];
    uint32_t              m_next[SIZE];
    uint32_t              m_prev[SIZE];
    VALUE                 m_value[SIZE];
    KEY                   m_key_original[SIZE];
    uint32_t              m_num_objects;
    const uint32_t        INVALID_INDEX = 0xffffffffu;

public:
    StaticHashMap()
    {
        for (uint32_t i = 0; i < SIZE; ++i)
        {
            m_hash[i] = INVALID_INDEX;
            m_next[i] = INVALID_INDEX;
            m_prev[i] = INVALID_INDEX;
            m_free_indices.push_back(i);
        }

        m_num_objects = 0;
    }

    ~StaticHashMap()
    {
    }

    void set(const KEY& key, const VALUE& value)
    {
        uint32_t data_index        = find_or_make(create_hash(key));
        m_key_original[data_index] = key;
        m_value[data_index]        = value;
    }

    bool has(const KEY& key)
    {
        uint32_t data_index = find_or_fail(create_hash(key));
        return data_index != INVALID_INDEX;
    }

    bool get(const KEY& key, VALUE& object)
    {
        uint32_t data_index = find_or_fail(create_hash(key));

        if (data_index == INVALID_INDEX)
            return false;
        else
        {
            object = m_value[data_index];
            return true;
        }
    }

    VALUE* get_ptr(const KEY& key)
    {
        uint32_t data_index = find_or_fail(create_hash(key));

        if (data_index == INVALID_INDEX)
            return nullptr;
        else
            return &m_value[data_index];
    }

    void remove(const KEY& key)
    {
        FindResult result = find(create_hash(key));

        // check if key actually exists
        if (result.data_index != INVALID_INDEX)
            erase(result);
    }

    void clear()
    {
        for (uint32_t i = 0; i < m_num_objects; i++)
            remove(m_key[i]);
    }

    uint32_t size()
    {
        return m_num_objects;
    }

private:
    FindResult find(const uint64_t& key)
    {
        FindResult result;

        result.hash_index      = INVALID_INDEX;
        result.data_prev_index = INVALID_INDEX;
        result.data_index      = INVALID_INDEX;

        result.hash_index = key % SIZE;
        result.data_index = m_hash[result.hash_index];

        while (result.data_index != INVALID_INDEX)
        {
            if (m_key[result.data_index] == key)
                return result;

            result.data_prev_index = result.data_index;
            result.data_index      = m_next[result.data_index];
        }

        return result;
    }

    // tries to find an object. if not found returns INVALID_INDEX.
    uint32_t find_or_fail(const uint64_t& key)
    {
        FindResult result = find(key);
        return result.data_index;
    }

    // tries to find an object. if not found creates Hash Entry.
    uint32_t find_or_make(const uint64_t& key)
    {
        FindResult result = find(key);

        if (result.data_index == INVALID_INDEX)
        {
            result.data_index         = m_free_indices.pop_front();
            m_next[result.data_index] = INVALID_INDEX;
            m_key[result.data_index]  = key;
            m_num_objects++;

            if (result.data_prev_index != INVALID_INDEX)
            {
                m_next[result.data_prev_index] = result.data_index;
                m_prev[result.data_index]      = result.data_prev_index;
            }

            if (m_hash[result.hash_index] == INVALID_INDEX)
                m_hash[result.hash_index] = result.data_index;
        }

        return result.data_index;
    }

    void erase(FindResult& result)
    {
        uint32_t last_data_index = m_num_objects - 1;
        m_num_objects--;
        // push last items' index into freelist
        m_free_indices.push_front(last_data_index);

        // Handle the element to be deleted
        if (result.data_prev_index == INVALID_INDEX)
            m_hash[result.hash_index] = INVALID_INDEX;
        else
            m_next[result.data_prev_index] = m_next[result.data_index];

        if (m_next[result.data_index] != INVALID_INDEX)
            m_prev[m_next[result.data_index]] = result.data_prev_index;

        if (result.data_index != last_data_index) // is NOT last element
        {
            // Handle the last element
            if (m_prev[last_data_index] == INVALID_INDEX)
            {
                uint64_t last_hash_index = m_key[last_data_index] % SIZE;
                m_hash[last_hash_index]  = result.data_index;
            }
            else
                m_next[m_prev[last_data_index]] = result.data_index;

            if (m_next[last_data_index] != INVALID_INDEX)
                m_prev[m_next[last_data_index]] = result.data_index;

            // Swap elements
            m_key[result.data_index]   = m_key[last_data_index];
            m_next[result.data_index]  = m_next[last_data_index];
            m_prev[result.data_index]  = m_prev[last_data_index];
            m_value[result.data_index] = m_value[last_data_index];
        }
    }
};
} // namespace nimble
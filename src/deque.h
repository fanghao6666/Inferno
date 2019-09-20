#pragma once

#include <stdint.h>
#include <assert.h>

namespace nimble
{
template <typename T, size_t N>
class Deque
{
private:
    T        _data[N];
    uint32_t _num_objects;
    uint32_t _front;
    uint32_t _back;

public:
    Deque()
    {
        _num_objects = 0;
        _front       = -1;
        _back        = 0;
    }

    ~Deque()
    {
    }

    inline uint32_t size() { return _num_objects; }

    inline T& front() { return _data[_front]; }

    inline T& back() { return _data[_back]; }

    inline void push_back(T value)
    {
        if (_num_objects < N)
        {
            if (_back == N && _front > 0)
                _back = 0;

            _data[_back++] = value;
            _num_objects++;
        }
    }

    inline T pop_back()
    {
        assert(_num_objects > 0 && "No elements in Deque to pop");

        if (_back == 0 && _front < N - 1)
            _back = N - 1;

        _num_objects--;
        return _data[--_back];
    }

    inline void push_front(T value)
    {
        if (_num_objects < N)
        {
            if (_front == -1 && _back < N - 1)
                _front = N - 1;

            _data[_front--] = value;
            _num_objects++;
        }
    }

    inline T pop_front()
    {
        assert(_num_objects > 0 && "No elements in Deque to pop");

        if (_front == N && _back > 0)
            _front = 0;

        _num_objects--;
        return _data[++_front];
    }
};
} // namespace nimble
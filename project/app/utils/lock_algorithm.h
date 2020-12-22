#pragma once

#include <algorithm>

#include "lock_container.h"

namespace beryl::lock
{
    template <typename T>
    [[nodiscard]] bool contains(const vector<T>& vec, const T& value)
    {
        const auto locked = vec.to_read();
        return std::find(locked->begin(), locked->end(), value) != locked->end();
    }

    template <typename T>
    [[nodiscard]] bool contains(const set<T>& set, const T& value)
    {
        const auto locked = set.to_read();
        return locked->find(value) != locked->end();
    }

    template <typename T>
    void insert(vector<T>& vec, const T& value)
    {
        const auto locked = vec.to_write();
        const auto iter = std::find(locked->begin(), locked->end(), value);
        if (iter == locked->end())
            locked->emplace_back(value);
    }

    template <typename T>
    void remove(vector<T>& vec, const T& value)
    {
        const auto locked = vec.to_write();
        const auto iter = std::find(locked->begin(), locked->end(), value);
        if (iter != locked->end())
            locked->erase(iter);
    }
}

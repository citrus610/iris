#pragma once

#include <iostream>
#include <functional>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cassert>
#include <vector>
#include <array>
#include <cctype>
#include <optional>

using usize = size_t;

template <typename T, usize N>
class arrayvec
{
private:
    T data[N];
    usize count;
public:
    arrayvec() : count(0) {};
public:
    [[nodiscard]] inline T& operator [] (usize index) noexcept
    {
        assert(index < this->count);
        return this->data[index];
    };

    [[nodiscard]] inline const T& operator [] (usize index) const noexcept
    {
        assert(index < this->count);
        return this->data[index];
    };
public:
    [[nodiscard]] inline T& front() noexcept
    {
        return this->data[0];
    };

    [[nodiscard]] inline const T& front() const noexcept
    {
        return this->data[0];
    };

    [[nodiscard]] inline T& back() noexcept
    {
        assert(this->count > 0);
        return this->data[this->count - 1];
    };

    [[nodiscard]] inline const T& back() const noexcept
    {
        assert(this->count > 0);
        return this->data[this->count - 1];
    };
public:
    [[nodiscard]] inline usize size() const noexcept
    {
        return this->count;
    };
public:
    [[nodiscard]] inline T* begin() noexcept
    {
        return &this->data[0];
    };

    [[nodiscard]] inline const T* begin() const noexcept
    {
        return &this->data[0];
    };

    [[nodiscard]] inline T* end() noexcept
    {
        return &this->data[0] + this->count;
    };

    [[nodiscard]] inline const T* end() const noexcept
    {
        return &this->data[0] + this->count;
    };
public:
    inline void add(const T& value) noexcept
    {
        assert(this->count < N);
        this->data[this->count++] = value;
    };

    inline void add(T&& value) noexcept
    {
        assert(this->count < N);
        this->data[this->count++] = std::move(value);
    };

    inline void pop() noexcept
    {
        assert(this->count > 0);
        this->count--;
    };

    inline void clear() noexcept
    {
        this->count = 0;
    };
};
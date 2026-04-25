/*

block write when buffer is full (for audio player)

*/

#pragma once

#include <cstddef>
#include <cstring>
#include <array>
#include <atomic>
#include <algorithm>


struct F32StereoFrame
{
    float l;
    float r;
};


template<typename T, std::size_t Capacity>
class SPSCRingBuffer
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    SPSCRingBuffer() : head_(0), tail_(0) {}
    
    std::size_t size() const
    {
        const std::size_t h = head_.load(std::memory_order_acquire);
        const std::size_t t = tail_.load(std::memory_order_acquire);

        return h - t;
    }

    std::size_t free() const
    {
        return Capacity - size();
    }

    bool empty() const
    {
        return size() == 0;
    }

    bool full() const
    {
        return size() == Capacity;
    }

    void clear()
    {
        tail_.store(head_.load(std::memory_order_relaxed), std::memory_order_release);
    }

    std::size_t write(const T* src, std::size_t len)
    {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t t = tail_.load(std::memory_order_acquire);

        const std::size_t used = h - t;
        const std::size_t can_write = Capacity - used;
        const std::size_t to_write = std::min(len, can_write);
        if (to_write == 0)
            return 0;

        const std::size_t write_pos = h & (Capacity - 1);
        const std::size_t first = std::min(to_write, Capacity - write_pos);
        const std::size_t second = to_write - first;

        std::memcpy(&data_[write_pos], src, first * sizeof(T));
        if (second > 0) {
            std::memcpy(&data_[0], src + first, second * sizeof(T));
        }

        head_.store(h + to_write, std::memory_order_release);
        return to_write;
    }

    std::size_t read(T* dst, std::size_t len)
    {
        const std::size_t h = head_.load(std::memory_order_acquire);
        const std::size_t t = tail_.load(std::memory_order_relaxed);

        const std::size_t can_read = h - t;
        const std::size_t to_read = std::min(can_read, len);
        if (to_read == 0)
            return 0;

        const std::size_t read_pos = t & (Capacity - 1);
        const std::size_t first = std::min(to_read, Capacity - read_pos);
        const std::size_t second = to_read - first;

        std::memcpy(dst, &data_[read_pos], first * sizeof(T));
        if (second > 0)
            std::memcpy(dst + first, &data_[0], second * sizeof(T));

        tail_.store(t + to_read, std::memory_order_release);
        return to_read;
    }
    
    
private:
    alignas(64) std::array<T, Capacity> data_{};
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
};

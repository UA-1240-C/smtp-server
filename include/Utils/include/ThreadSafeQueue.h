#pragma once

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <algorithm>
#include <optional>
#include <cds/container/fcdeque.h>
#include <cds/init.h>

using cds::container::FCDeque;

namespace ISXThreadPool {

template <typename T> class ThreadSafeQueue
{
public:
    ThreadSafeQueue() : m_data()
    {
    }

    void PushBack(T &&value)
    {
        m_data.push_back(std::forward<T>(value));
    }

    void PushFront(T &&value)
    {
        m_data.push_front(std::forward<T>(value));
    }

    std::optional<T> PopFront() {
        T value;
        if (m_data.pop_front(value)) {
            return std::move(value);
        }
        return std::nullopt;
    }

    std::optional<T> PopBack() {
        T value;
        if (m_data.pop_back(value)) {
            return std::move(value);
        }
        return std::nullopt;
    }

    std::optional<T> CopyFrontAndRotateToBack()
    {
        std::optional<T> frontItem = PopFront();
        if (frontItem) {
            PushBack(std::move(*frontItem));
        }
        return frontItem;
    }

private:
    FCDeque<T> m_data;
};

} // namespace ISXThreadPool
#endif // THREADSAFEQUEUE_H

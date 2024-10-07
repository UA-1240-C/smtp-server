#pragma once

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <algorithm>
#include <deque>
#include <optional>

namespace ISXThreadPool {
/**
 * @brief Concept to check if a type provides basic lock functionality.
 *
 * This concept checks if a type can be locked and unlocked, and if it supports
 * try-lock functionality.
 *
 * @tparam Lock The type to be checked for lockable functionality.
 */
template <typename Lock>
concept IsLocable = requires(Lock &&lock) {
    lock.lock();
    lock.unlock();
    { lock.try_lock() } -> std::convertible_to<bool>;
};

/**
 * @class ThreadSafeQueue
 * @brief A thread-safe queue implementation.
 *
 * This class provides a thread-safe implementation of a double-ended queue
 * (deque) that supports adding and removing elements from both the front and
 * the back in synchronized manner.It also includes methods for rotating
 * elements and copying the front element to the back of the queue.
 *
 * @tparam T The type of elements stored in the queue.
*/
template <typename T> class ThreadSafeQueue {
public:
    /**
    * @brief Constructs an empty ThreadSafeQueue.
    */
    ThreadSafeQueue() : m_data(std::make_shared<std::deque<T>>()) {}

    /**
    * @brief Adds an element by the actual
    * value type to the back of the queue.
    * @param value The element to be added.
    */
    void PushBack(T &&value) { m_data.load()->push_back(std::forward<T>(value)); }

    /**
    * @brief Adds an element to the front of the queue.
    * @param value The element to be added.
    */
    void PushFront(T &&value) {
        m_data.load()->push_front(std::forward<T>(value));
    }

    /**
    * @brief Removes and returns the front element of the queue.
    * @return An std::optional containing the front element if the queue is not
    * empty, or std::nullopt otherwise.
    */
    std::optional<T> PopFront() {
    if (m_data.load()->empty())
        return std::nullopt;

    auto front = std::move(m_data.load()->front());
        m_data.load()->pop_front();
        return front;
    }

    /**
    * @brief Removes and returns the back element of the queue.
    * @return An std::optional containing the back element if the queue is not
    * empty, or std::nullopt if the queue is empty.
    */
    std::optional<T> PopBack()
    {
        if (m_data.load()->empty())
            return std::nullopt;

        auto back = std::move(m_data.load()->back());
        m_data.load()->pop_back();
        return back;
    }

    /**
    * @brief Moves an element to the front of the queue.
    *
    * If the item is found in the queue, it will be removed from its current
    * position and added to the front of the queue.
    *
    * @param item The item to be moved to the front.
    */
    void RotateToFront(const T &item)
    {
        auto iter = std::find(m_data.load()->begin(), m_data.load()->end(), item);

        if (iter != m_data.load()->end())
        {
            m_data.load()->erase(iter);
            m_data.load()->push_front(item);
        }
    }

    /**
    * @brief Copies the front element and moves it to the back of the queue and
    * returns it.
    *
    * @return An std::optional containing the copied front element if the queue
    * is not empty, or std::nullopt if the queue is empty.
    */
    std::optional<T> CopyFrontAndRotateToBack()
    {
        if (m_data.load()->empty())
            return std::nullopt;

        auto front = std::move(m_data.load()->front());
        m_data.load()->pop_front();

        m_data.load()->push_back(front);

        return front;
    }

private:
    std::atomic<std::shared_ptr<std::deque<T>>> m_data;
};

} // namespace ISXThreadPool
#endif // THREADSAFEQUEUE_H

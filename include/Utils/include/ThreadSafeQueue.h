#pragma once

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <algorithm>
#include <mutex>
#include <deque>
#include <optional>

template <typename Lock>
concept is_locable = requires(Lock&& lock)
{
  lock.lock();
  lock.unlock();
  { lock.try_lock() } -> std::convertible_to<bool>;
};

template <typename T, is_locable Lock = std::mutex>
class ThreadSafeQueue
{
public:
  ThreadSafeQueue() : mutex_(), data_() {}

  void push_back(T&& value)
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    data_.push_back(std::forward<T>(value));
  }

  void push_front(T&& value)
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    data_.push_front(std::forward<T>(value));
  }

  std::optional<T> pop_front()
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    if (data_.empty()) return std::nullopt;

    auto front = std::move(data_.front());
    data_.pop_front();
    return front;
  }

  std::optional<T> pop_back()
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    if (data_.empty()) return std::nullopt;

    auto back = std::move(data_.back());
    data_.pop_back();
    return back;
  }

  void rotate_to_front(const T& item) {
    std::scoped_lock lock(mutex_);
    auto iter = std::find(data_.begin(), data_.end(), item);

    if (iter != data_.end()) {
      data_.erase(iter);
    }

    data_.push_front(item);
  }

  std::optional<T> copy_front_and_rotate_to_back() {
    std::scoped_lock lock(mutex_);

    if (data_.empty()) return std::nullopt;

    auto front = data_.front();
    data_.pop_front();

    data_.push_back(front);

    return front;
  }
private:
  mutable Lock mutex_;
  std::deque<T> data_;      // Guarded by mutex_
};

#endif //THREADSAFEQUEUE_H

#pragma once

#include <atomic>
#include <concepts>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <semaphore>
#include <thread>
#include <type_traits>

#include "ThreadSafeQueue.h"

namespace ISXThreadPool
{
template <typename FunctionType = std::function<void()>, typename ThreadType = std::jthread>
    requires std::invocable<FunctionType> &&
             std::is_same_v<void, std::invoke_result_t<FunctionType>>
class ThreadPool {
public:
    template <typename InitFunction = std::function<void(std::size_t)>>
        requires std::invocable<InitFunction, std::size_t> &&
                 std::is_same_v<void, std::invoke_result_t<InitFunction, std::size_t>>
    explicit ThreadPool(
        const unsigned int &number_of_threads = std::thread::hardware_concurrency(),
        InitFunction init = [](std::size_t) {})
        : m_tasks(number_of_threads) {
        std::size_t current_id = 0;
        for (std::size_t i = 0; i < number_of_threads; ++i) {
            m_priority_queue.push_back(static_cast<size_t>(current_id));
            try {
                m_threads.emplace_back([&, id = current_id,
                                       init](const std::stop_token &stop_tok) {
                    // invoke the init function on the thread
                    try {
                        std::invoke(init, id);
                    } catch (...) { /* suppress exception*/ }

                    do {
                        // wait until signaled
                        m_tasks[id].m_signal.acquire();

                        do {
                            // invoke the task
                            while (auto task = m_tasks[id].m_tasks.pop_front()) {
                                // decrement the unassigned tasks as the task is now going
                                // to be executed
                                m_unassigned_tasks.fetch_sub(1, std::memory_order_release);
                                // invoke the task
                                std::invoke(std::move(task.value()));
                                // the above task can push more work onto the pool, so we
                                // only decrement the in flights once the task has been
                                // executed because now it's now longer "in flight"
                                m_in_flight_tasks.fetch_sub(1, std::memory_order_release);
                            }

                            // try to steal a task
                            for (std::size_t j = 1; j < m_tasks.size(); ++j) {
                                const std::size_t index = (id + j) % m_tasks.size();
                                if (auto task = m_tasks[index].m_tasks.pop_back()) {
                                    // steal a task
                                    m_unassigned_tasks.fetch_sub(1, std::memory_order_release);
                                    std::invoke(std::move(task.value()));
                                    m_in_flight_tasks.fetch_sub(1, std::memory_order_release);
                                    // stop stealing once we have invoked a stolen task
                                    break;
                                }
                            }
                            // check if there are any unassigned tasks before rotating to the
                            // front and waiting for more work
                        } while (m_unassigned_tasks.load(std::memory_order_acquire) > 0);

                        m_priority_queue.rotate_to_front(id);
                        // check if all tasks are completed and release the barrier (binary
                        // semaphore)
                        if (m_in_flight_tasks.load(std::memory_order_acquire) == 0) {
                            m_threads_complete_signal.store(true, std::memory_order_release);
                            m_threads_complete_signal.notify_one();
                        }

                    } while (!stop_tok.stop_requested());
                });
                // increment the thread id
                ++current_id;

            } catch (...) {
                // catch all

                // remove one item from the tasks
                m_tasks.pop_back();

                // remove our thread from the priority queue
                std::ignore = m_priority_queue.pop_back();
            }
        }
    }

    ~ThreadPool() {
        WaitForTasks();

        // stop all threads
        for (std::size_t i = 0; i < m_threads.size(); ++i) {
            m_threads[i].request_stop();
            m_tasks[i].m_signal.release();
            m_threads[i].join();
        }
    }

    /// thread pool is non-copyable
    ThreadPool(const ThreadPool &) = delete;
    //ThreadPool &operator=(const ThreadPool &) = delete;

    /**
     * @brief Enqueue a task into the thread pool that returns a result.
     * @details Note that task execution begins once the task is enqueued.
     * @tparam Function An invokable type.
     * @tparam Args Argument parameter pack
     * @tparam ReturnType The return type of the Function
     * @param f The callable function
     * @param args The parameters that will be passed (copied) to the function.
     * @return A std::future<ReturnType> that can be used to retrieve the returned value.
     */
    template <typename Function, typename... Args,
              typename ReturnType = std::invoke_result_t<Function &&, Args &&...>>
        requires std::invocable<Function, Args...>
    [[nodiscard]] std::future<ReturnType> Enqueue(Function f, Args... args) {
        auto shared_promise = std::make_shared<std::promise<ReturnType>>();
        auto task = [func = std::move(f), ... largs = std::move(args),
                     promise = shared_promise]() {
            try {
                if constexpr (std::is_same_v<ReturnType, void>) {
                    func(largs...);
                    promise->set_value();
                } else {
                    promise->set_value(func(largs...));
                }

            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        };

        // get the future before enqueuing the task
        auto future = shared_promise->get_future();
        // enqueue the task
        enqueue_task(std::move(task));
        return future;
    }

    /**
     * @brief Enqueue a task to be executed in the thread pool. Any return value of the function
     * will be ignored.
     * @tparam Function An invokable type.
     * @tparam Args Argument parameter pack for Function
     * @param func The callable to be executed
     * @param args Arguments that will be passed to the function.
     */
    template <typename Function, typename... Args>
        requires std::invocable<Function, Args...>
    void EnqueueDetach(Function &&func, Args &&...args) {
        EnqueueTask(std::move([f = std::forward<Function>(func),
                                ... largs =
                                    std::forward<Args>(args)]() mutable -> decltype(auto) {
            // suppress exceptions
            try {
                if constexpr (std::is_same_v<void,
                                             std::invoke_result_t<Function &&, Args &&...>>) {
                    std::invoke(f, largs...);
                } else {
                    // the function returns an argument, but can be ignored
                    std::ignore = std::invoke(f, largs...);
                }
            } catch (...) {
            }
        }));
    }

    /**
     * @brief Returns the number of threads in the pool.
     *
     * @return std::size_t The number of threads in the pool.
     */
    [[nodiscard]] auto Size() const { return m_threads.size(); }

    // Wait for all tasks to finish.

    void WaitForTasks() const {
        if (m_in_flight_tasks.load(std::memory_order_acquire) > 0) {
            // wait for all tasks to finish
            m_threads_complete_signal.wait(false);
        }
    }

private:
    template <typename Function>
    void EnqueueTask(Function &&f) {
        auto i_opt = m_priority_queue.copy_front_and_rotate_to_back();
        if (!i_opt.has_value()) return;

        // get the index
        auto i = *(i_opt);

        // increment the unassigned tasks and in flight tasks
        m_unassigned_tasks.fetch_add(1, std::memory_order_release);

        // reset the in flight signal if the list was previously empty
        if (const auto prev_in_flight =
            m_in_flight_tasks.fetch_add(1, std::memory_order_release) == 0) {
            m_threads_complete_signal.store(false, std::memory_order_release);
            }

        // assign work
        m_tasks[i].m_tasks.push_back(std::forward<Function>(f));
        m_tasks[i].m_signal.release();
    }

    /**
     * @struct TaskItem
     * @brief A structure representing a task item in a thread pool or task processing system.
     *
     * This structure holds a queue of tasks and a semaphore used for signaling. It is designed
     * to manage tasks that need to be processed by threads in a thread pool.
     */
    struct TaskItem {
        /**
         * @var tasks
         * @brief A thread-safe queue holding tasks to be processed.
         *
         * This queue contains tasks of type `FunctionType`. It is designed to be accessed safely
         * from multiple threads.
         */
        ThreadSafeQueue<FunctionType> m_tasks{};

        /**
         * @var signal
         * @brief A semaphore used for signaling task availability.
         *
         * The semaphore is initialized to zero and used to signal when tasks are available
         * for processing. It helps in synchronizing task processing among threads.
         */
        std::binary_semaphore m_signal{0};
    };
    /**
     * @var threads_
     * @brief A vector of threads used for task processing.
     *
     * This vector holds the threads that are responsible for processing tasks from the task items.
     */
    std::vector<ThreadType> m_threads;

    /**
     * @var tasks_
     * @brief A deque of task items.
     *
     * This deque contains task items, each of which includes a queue of tasks and a semaphore
     * for signaling. It helps in organizing and managing tasks to be processed.
     */
    std::deque<TaskItem> m_tasks;

    /**
     * @var priority_queue_
     * @brief A thread-safe queue for managing task priorities.
     *
     * This queue stores task priorities and is used to prioritize tasks in the processing system.
     */
    ThreadSafeQueue<std::size_t> m_priority_queue;

    /**
     * @var unassigned_tasks_
     * @brief An atomic counter for the number of unassigned tasks.
     *
     * This atomic variable keeps track of the number of tasks that have not yet been assigned
     * to any thread for processing. It is zero-initialized.
     */
    std::atomic_int_fast64_t m_unassigned_tasks{0};

    /**
     * @var in_flight_tasks_
     * @brief An atomic counter for the number of in-flight tasks.
     *
     * This atomic variable counts the number of tasks that are currently being processed by threads.
     * It is zero-initialized.
     */
    std::atomic_int_fast64_t m_in_flight_tasks{0};

    /**
     * @var threads_complete_signal_
     * @brief An atomic flag indicating if all threads have completed their work.
     *
     * This atomic boolean flag is used to signal when all threads in the pool have finished
     * processing their tasks. It is zero-initialized.
     */
    std::atomic_bool m_threads_complete_signal{false};
};
};
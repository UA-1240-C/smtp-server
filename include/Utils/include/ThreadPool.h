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
/**
 * @class ThreadPool
 * @brief A thread pool that manages a set of threads and allows for task execution.
 * @details Requires the FunctionType return value to be same as void.
 * @tparam FunctionType The type of the function to be executed by the threads. Default is std::function<void()>.
 * @tparam ThreadType The type of thread to be used in the pool. Default is std::jthread.
 */
template <typename FunctionType = std::function<void()>, typename ThreadType = std::jthread>
    requires std::invocable<FunctionType> &&
             std::same_as<void, std::invoke_result_t<FunctionType>>
class ThreadPool {
public:
    /**
     * @brief Constructs a thread pool with the specified number of threads.
     *
     * @tparam InitFunction An invokable type for initializing each thread.
     * @param number_of_threads The number of threads to create in the pool. Defaults to the number of hardware threads available.
     * @param init A function to initialize each thread. It takes the thread ID as an argument.
     *
     * @details This constructor initializes a thread pool with a given number of threads. Each thread is initialized with the provided function `init`.
     * The threads are stored in the `m_threads` vector, and tasks are distributed among them based on a priority queue.
     *
     * @par The constructor performs the following steps:
     * @li @c Allocates the required number of task containers.
     * @li @c Initializes and starts each thread.
     * @li @c Threads are assigned an ID and added to a priority queue.
     * @li @c If a thread fails to start, it is removed from the pool.
     *
     * @note The `init` function is invoked on each thread during its initialization, allowing for custom setup per thread.
     *
     * @exception Throws no exceptions; however, any exceptions thrown by `init` are suppressed.
     */
    template <typename InitFunction = std::function<void(std::size_t)>>
        requires std::invocable<InitFunction, std::size_t> &&
                 std::is_same_v<void, std::invoke_result_t<InitFunction, std::size_t>>
    explicit ThreadPool(
        const unsigned int &number_of_threads = std::thread::hardware_concurrency(),
        InitFunction init = [](std::size_t) {})
        : m_tasks(number_of_threads) {
        std::size_t current_id = 0;
        for (std::size_t i = 0; i < number_of_threads; ++i) {
            m_priority_queue.PushBack(static_cast<size_t>(current_id));
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
                            while (auto task = m_tasks[id].m_tasks.PopFront()) {
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
                                if (auto task = m_tasks[index].m_tasks.PopBack()) {
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

                        m_priority_queue.RotateToFront(id);
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
                std::ignore = m_priority_queue.PopBack();
            }
        }
    }

    /**
     * @brief Destroys the thread pool and stops all threads.
     *
     * @details The destructor waits for all tasks in the pool to complete before stopping and joining all threads.
     * It performs the following steps:
     * - Invokes `WaitForTasks` to ensure all tasks are completed.
     * - Requests each thread to stop by signaling them and releasing their task semaphores.
     * - Joins all threads to ensure they have finished executing.
     *
     * @note This destructor is non-throwing and ensures a graceful shutdown of the thread pool.
     */
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
    ThreadPool &operator=(const ThreadPool &) = delete;

    /**
     * @brief Enqueues a task into the thread pool that returns a result.
     *
     * @tparam Function An invokable type representing the task to be executed.
     * @tparam Args Argument parameter pack for the function.
     * @tparam ReturnType The return type of the function.
     * @param f The callable function to be enqueued.
     * @param args The parameters that will be passed (copied) to the function.
     * @return A std::future<ReturnType> that can be used to retrieve the returned value.
     *
     * @details This method enqueues a task that returns a value and immediately begins execution.
     * It creates a `std::promise` to manage the task's return value and a `std::future` to retrieve it.
     *
     * @par The method performs the following steps:
     * @li @c Creates a shared promise to manage the return value or exception.
     * @li @c Wraps the provided function and arguments into a task that sets the promise's value.
     * @li @c Enqueues the task for execution by one of the threads in the pool.
     * @li @c Returns the associated future, which can be used to retrieve the result or catch exceptions.
     *
     * @warning The caller must ensure that the provided function and arguments are valid.
     *
     * @throws std::future_error if the promise is broken.
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
     * @brief Enqueues a task to be executed in the thread pool without returning a result.
     *
     * @tparam Function An invokable type representing the task to be executed.
     * @tparam Args Argument parameter pack for the function.
     * @param func The callable function to be executed.
     * @param args Arguments that will be passed to the function.
     *
     * @details This method enqueues a task that does not return a result. The task is immediately
     * executed by one of the threads in the pool. Any exceptions thrown by the task are suppressed and not propagated.
     *
     * @par The method performs the following steps:
     * @li @c Wraps the provided function and arguments into a task that ignores its return value.
     * @li @c Enqueues the task for execution by one of the threads in the pool.
     *
     * @note This method is useful for fire-and-forget tasks where the result is not needed.
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

    /**
     * @brief Waits for all tasks in the pool to complete.
     *
     * @details This method blocks until all tasks in the pool are completed.
     * It waits for the `m_threads_complete_signal` semaphore to be released, indicating that no tasks are in flight.
     *
     * @par The method performs the following steps:
     * @li @c Blocks until all tasks have been processed by checking the `m_threads_complete_signal`.
     * @li @c If there are no in-flight tasks, the signal is released and the method returns.
     *
     * @note This method can be used to ensure that all pending tasks are completed before shutting down the pool or before proceeding to the next stage of processing.
     */
    void WaitForTasks() const {
        if (m_in_flight_tasks.load(std::memory_order_acquire) > 0) {
            // wait for all tasks to finish
            m_threads_complete_signal.wait(false);
        }
    }

private:
    /**
     * @brief Enqueues a task for execution by the thread pool.
     *
     * @tparam Function An invokable type representing the task to be executed.
     * @param f The task function to be enqueued.
     *
     * @details This method assigns a task to one of the threads in the pool.
     * The task is enqueued in the task queue of a specific thread, and the thread's
     * signal is released to start processing the task.
     *
     * @par The method does the following:
     * @li @c Selects a thread based on the internal priority queue.
     * @li @c Increments the counters for unassigned and in-flight tasks.
     * @li @c Resets the thread completion signal if the task queue was previously empty.
     * @li @c Assigns the task to the selected thread and signals the thread to start execution.
     *
     * If no threads are available, the task is not enqueued.
     *
     * @note This is a private method and is used internally by the `Enqueue` and `EnqueueDetach` methods.
     *
     * @warning Exceptions in the task function are not handled by this method; they must be managed within the task itself.
     *
     */
    template <typename Function>
    void EnqueueTask(Function &&f) {
        auto i_opt = m_priority_queue.CopyFrontAndRotateToBack();
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
        m_tasks[i].m_tasks.PushBack(std::forward<Function>(f));
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
         * @var m_tasks
         * @brief A thread-safe queue holding tasks to be processed.
         *
         * This queue contains tasks of type `FunctionType`. It is designed to be accessed safely
         * from multiple threads.
         */
        ThreadSafeQueue<FunctionType> m_tasks{};

        /**
         * @var m_signal
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
     * @var m_tasks
     * @brief A deque of task items.
     *
     * This deque contains task items, each of which includes a queue of tasks and a semaphore
     * for signaling. It helps in organizing and managing tasks to be processed.
     */
    std::deque<TaskItem> m_tasks;

    /**
     * @var m_priority_queue
     * @brief A thread-safe queue for managing task priorities.
     *
     * This queue stores task priorities and is used to prioritize tasks in the processing system.
     */
    ThreadSafeQueue<std::size_t> m_priority_queue;

    /**
     * @var m_unassigned_tasks
     * @brief An atomic counter for the number of unassigned tasks.
     *
     * This atomic variable keeps track of the number of tasks that have not yet been assigned
     * to any thread for processing. It is zero-initialized.
     */
    std::atomic_int_fast64_t m_unassigned_tasks{0};

    /**
     * @var m_in_flight_tasks
     * @brief An atomic counter for the number of in-flight tasks.
     *
     * This atomic variable counts the number of tasks that are currently being processed by threads.
     * It is zero-initialized.
     */
    std::atomic_int_fast64_t m_in_flight_tasks{0};

    /**
     * @var m_threads_complete_signal
     * @brief An atomic flag indicating if all threads have completed their work.
     *
     * This atomic boolean flag is used to signal when all threads in the pool have finished
     * processing their tasks. It is zero-initialized.
     */
    std::atomic_bool m_threads_complete_signal{false};
};
};
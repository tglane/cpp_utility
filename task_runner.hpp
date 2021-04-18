#ifndef CPP_UTILITY_TASK_RUNNER_HPP
#define CPP_UTILITY_TASK_RUNNER_HPP

#include <memory>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>

class task_runner
{
    struct condition_holder
    {
        bool condition = false;
        std::mutex mutex;
        std::condition_variable cv;
    };

public:

    task_runner() = default;
    task_runner(const task_runner&) = delete;
    task_runner& operator=(const task_runner&) = delete;
    task_runner(task_runner&&) = default;
    task_runner& operator=(task_runner&&) = default;

    ~task_runner()
    {
        stop();
    }

    void start(const std::function<void()>& func,
        const std::chrono::duration<int64_t, std::milli>& delay = std::chrono::milliseconds(100))
    {
        if(m_holder->condition && m_runner.joinable())
        {
            m_holder->condition = false;
            m_holder->cv.notify_one();
            m_runner.join();
        }

        m_holder->condition = true;

        m_runner = std::thread([ptr = m_holder.get(), func, delay]() {
            while(ptr->condition)
            {
                func();
                std::unique_lock<std::mutex> lock {ptr->mutex};
                ptr->cv.wait_for(lock, delay, [ptr]() { return !ptr->condition; });
            }

        });
    }

    void stop()
    {
        if(!m_holder)
            return;

        m_holder->condition = false;
        m_holder->cv.notify_one();

        if(m_runner.joinable())
            m_runner.join();
    }

private:

    std::unique_ptr<condition_holder> m_holder = std::make_unique<condition_holder>();

    std::thread m_runner;

};

#endif

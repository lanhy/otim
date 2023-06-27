#pragma once

#include <condition_variable>
#include <mutex>
#include <atomic>

#define SEMAPHORE_TIMEOUT 3

class Semaphore {
public:
    explicit Semaphore(unsigned int count) {
        m_count = count;
    }
    ~Semaphore() {

    }
public:
    void wait() {
        std::unique_lock<std::mutex> unique_lock(m_mutex);
        --m_count;
        while (m_count < 0) {
            m_condition_variable.wait(unique_lock);
        }
    }
    
    bool wait(int second) {

        std::unique_lock<std::mutex> unique_lock(m_mutex);
        --m_count;
        while (m_count < 0) {
            if(std::cv_status::timeout == m_condition_variable.wait_for(unique_lock, std::chrono::seconds(second))){
                //timeout need to restore count
                ++m_count;
                return false;
            }
        }
        return true;
    }

    void signal() {
        std::lock_guard<std::mutex> lg(m_mutex);
        if (++m_count < 1) {
            m_condition_variable.notify_one();
        }
    }

    int count() {
        return m_count;
    }

private:
    std::atomic<int> m_count;
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
};


class CSemaphoreSimple {
public:
    CSemaphoreSimple() {
    }
    ~CSemaphoreSimple() {
    }
public:
    void wait() {
        std::unique_lock<std::mutex> unique_lock(m_mutex);
        m_condition_variable.wait(unique_lock);
    }
    
    bool wait(int second) {
        std::unique_lock<std::mutex> unique_lock(m_mutex);
        return m_condition_variable.wait_for(unique_lock, std::chrono::seconds(second)) == std::cv_status::no_timeout;
    }

    void signal() {
        std::lock_guard<std::mutex> lg(m_mutex);
        m_condition_variable.notify_one();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
};

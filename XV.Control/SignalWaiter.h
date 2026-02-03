#ifndef SIGNALWAITER_H
#define SIGNALWAITER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

/**
 * @class SignalWaiter
 * @brief 简单的信号同步类，用于控制线程等待
 * 
 * 允许一个线程等待另一个线程发出的信号
 */
class SignalWaiter
{
public:
    /**
     * @brief 构造信号同步对象
     * @param signaled 初始状态 (true = 已发出信号, false = 未发出信号)
     */
    explicit SignalWaiter(bool signaled = false)
        : m_signaled(signaled)
    {}

    /**
     * @brief 等待信号 (无限期)
     * 
     * 当前线程会阻塞直到信号被发出
     */
    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 使用while循环防止虚假唤醒
        m_cv.wait(lock, [this] { return m_signaled.load(); });
    }

    /**
     * @brief 带超时的等待
     * @tparam Rep 时间单位的类型
     * @tparam Period 时间单位
     * @param duration 等待的最长时间
     * @return true 如果在超时前收到信号
     * @return false 如果超时
     */
    template<typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& duration)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, duration, [this] { return m_signaled.load(); });
    }
    bool wait_for(int milliseconds) { return wait_for(std::chrono::milliseconds(milliseconds)); }

    /**
     * @brief 发出信号
     * 
     * 唤醒所有等待的线程，并设置信号状态
     */
    void signal()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_signaled = true; // 等价于 store(true, std::memory_order_seq_cst)
        }
        // 通知所有等待线程
        m_cv.notify_all();
    }

    /**
     * @brief 重置信号状态
     * 
     * 将信号状态恢复为未发出信号
     */
    void reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_signaled = false;
    }

    /**
     * @brief 检查信号状态
     * @return true 如果信号已发出
     * @return false 如果信号未发出
     */
    bool is_signaled() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_signaled.load();
    }

private:
    mutable std::mutex m_mutex;   ///< 保护共享状态的互斥锁
    std::condition_variable m_cv; ///< 信号条件变量
    std::atomic<bool> m_signaled; ///< 信号状态标志
};

#endif // SIGNALWAITER_H

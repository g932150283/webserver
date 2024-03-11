#ifndef __WEBSERVER_SCHEDULER_H__
#define __WEBSERVER_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "fiber.h"
#include "thread.h"
#include "mutex.h"

namespace webserver {

/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    /**
     * @brief 析构函数
     */
    virtual ~Scheduler();

    /**
     * @brief 返回协程调度器名称
     */
    const std::string& getName() const { return m_name;}

    /**
     * @brief 返回当前协程调度器
     */
    static Scheduler* GetThis();

    /**
     * @brief 返回当前协程调度器的调度协程
     */
    static Fiber* GetMainFiber();

    /**
     * @brief 启动协程调度器
     */
    void start();

    /**
     * @brief 停止协程调度器
     */
    void stop();

    /**
    * @brief 调度协程或函数到指定的线程。
    * 这个模板函数允许调度器以统一的方式处理协程和函数对象。
    * 用户可以指定一个协程或函数对象以及一个线程ID，调度器根据这些信息来执行任务。如果线程ID为-1，表示任务可以在任意线程上执行。
    * @param[in] fc 协程或函数。这个参数是泛型的，可以是一个协程（Fiber::ptr）或者是一个可调用对象（比如函数指针、lambda表达式等）。
    * @param[in] thread 协程或函数执行的线程ID。默认为-1，表示不指定线程，由调度器决定在哪个线程上执行。
    *                   如果指定了线程ID，任务将尝试在指定的线程上执行。
    * @details
    * 函数首先锁定互斥锁以确保线程安全，然后调用`scheduleNoLock`来实际安排任务。
    * `scheduleNoLock`会根据当前任务队列的状态返回一个布尔值，指示是否需要通过`tickle`方法通知调度器有新的任务到来。
    * 如果`scheduleNoLock`返回true，表示任务队列之前为空，现在有了新任务，因此需要调用`tickle`来唤醒调度器处理新的任务。
    */

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false; // 标记是否需要唤醒调度器
        {
            MutexType::Lock lock(m_mutex); // 锁定互斥锁以确保线程安全
            need_tickle = scheduleNoLock(fc, thread); // 安排任务，检查是否需要唤醒调度器
        }

        if(need_tickle) {
            tickle(); // 如果需要，则唤醒调度器
        }
    }

    

    /**
     * @brief 批量调度协程
     * @param[in] begin 协程数组的开始
     * @param[in] end 协程数组的结束
     */
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }

    void switchTo(int thread = -1);
    std::ostream& dump(std::ostream& os);
protected:
    /**
     * @brief 通知协程调度器有任务了
     */

    virtual void tickle();
    /**
     * @brief 协程调度函数
     */
    void run();

    /**
     * @brief 返回是否可以停止
     */
    virtual bool stopping();

    /**
     * @brief 协程无任务可调度时执行idle协程
     */
    virtual void idle();

    /**
     * @brief 设置当前的协程调度器
     */
    void setThis();

    /**
     * @brief 是否有空闲线程
     */
    bool hasIdleThreads() { return m_idleThreadCount > 0;}
private:
    /**
    * @brief 将协程或函数安排进调度队列中，不需要外部加锁。
    * 
    * 这个函数在调度器的互斥锁已经被锁定的情况下调用，以安全地将任务添加到调度队列中。
    * 它检查任务队列是否为空，如果为空，表示调度器在等待状态，需要通过返回true来告知调用者发送唤醒信号。
    * 
    * @tparam FiberOrCb 协程或函数的类型。这使得函数能够以泛型的方式处理不同类型的任务，无论是协程对象还是可调用对象。
    * @param fc 要调度的协程或函数。它可以是一个协程对象（Fiber::ptr）或者是一个可调用对象，例如函数指针、lambda表达式等。
    * @param thread 指定任务应该在哪个线程上执行的线程ID。如果传入-1，则表示任务可以在任意线程上执行。
    * 
    * @return bool 如果调度队列在添加任务前是空的，则返回true，表示需要唤醒调度器。否则，返回false。
    * 
    * @details
    * 函数首先检查调度队列是否为空，这是决定是否需要唤醒调度器的关键。
    * 然后，它创建一个FiberAndThread结构体实例，封装了任务和目标线程ID。
    * 如果该任务是有效的（即封装的协程对象或函数对象不为空），则将它添加到调度队列的末尾。
    * 这种设计允许调度器以统一的方式处理协程和函数，同时保持高效和线程安全。
    */
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty(); // 检查调度队列是否为空
        FiberAndThread ft(fc, thread); // 创建封装了任务和线程ID的结构体

        // 如果任务是有效的，即有具体的协程或函数需要执行
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft); // 将任务添加到调度队列
        }

        return need_tickle; // 返回是否需要唤醒调度器
    }

private:
    /**
     * @brief 协程/函数/线程组
     */
    struct FiberAndThread {
        /// 协程
        Fiber::ptr fiber;
        /// 协程执行函数
        std::function<void()> cb;
        /// 线程id
        int thread;

        /**
         * @brief 构造函数
         * Fiber对象和线程ID构造函数:
         * 通过传入一个Fiber智能指针和一个线程ID来初始化。这种方式是当你已经有一个Fiber实例，想要将其与特定的线程ID关联起来时使用。
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * Fiber智能指针的引用和线程ID构造函数:
         * 这个构造函数与第一个相似，但它接收一个Fiber::ptr的指针。
         * 这允许在构造FiberAndThread实例时直接修改传入的Fiber::ptr指针所指向的对象。
         * 这样可以在不需要额外的临时变量的情况下直接交换Fiber实例。
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
            fiber.swap(*f);
        }

        /**
         * @brief 构造函数
         * 函数对象和线程ID构造函数:
         * 允许使用一个函数对象（可以是一个lambda表达式、函数指针或者任何可调用对象）和线程ID进行初始化。
         * 这是为了直接调度函数而不是协程时使用，增加了调度器的灵活性。
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * 函数对象指针和线程ID构造函数:
         * 这个构造函数接受一个指向函数对象的指针和线程ID。
         * 它允许在构造FiberAndThread时，从指针指向的函数对象中移动函数对象，这样做可以避免不必要的复制，提高效率。
         * @param[in] f 协程执行函数指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }

        /**
         * @brief 无参构造函数
         * 一个默认的构造函数，不接受任何参数，将线程ID初始化为-1，并且不关联任何协程或函数。
         * 这可以用于创建一个占位的FiberAndThread对象，可能在稍后某个时间点再填充实际的内容。
         */
        FiberAndThread()
            :thread(-1) {
        }

        /*
        通过提供这些不同的构造函数，代码能够更加灵活地创建FiberAndThread对象，
        以满足不同的需求，如直接使用协程、调度简单的函数或者在特定线程上执行任务等。
        这种设计使得调度器能以统一和灵活的方式处理不同类型的任务。
        */

        /**
         * @brief 重置数据
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    /// Mutex
    MutexType m_mutex;
    /// 线程池
    std::vector<Thread::ptr> m_threads;
    /// 待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    /// use_caller为true时有效, 调度协程
    Fiber::ptr m_rootFiber;
    /// 协程调度器名称
    std::string m_name;
protected:
    /// 协程下的线程id数组
    std::vector<int> m_threadIds;
    /// 线程数量
    size_t m_threadCount = 0;
    /// 工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    /// 空闲线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    /// 是否正在停止
    bool m_stopping = true;
    /// 是否自动停止
    bool m_autoStop = false;
    /// 主线程id(use_caller)
    int m_rootThread = 0;
};

// class SchedulerSwitcher : public Noncopyable {
class SchedulerSwitcher {
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();
private:
    Scheduler* m_caller;
};

}

#endif

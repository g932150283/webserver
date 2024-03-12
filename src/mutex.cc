#include "mutex.h"
#include "macro.h"
#include "scheduler.h"
#include <stdexcept>

namespace webserver {

/**
 * @brief 初始化信号量对象。
 * 
 * @param count 信号量的初始计数值。
 */
Semaphore::Semaphore(uint32_t count) {
    // 尝试初始化信号量，第二个参数0表示信号量是当前进程的局部信号量，count是初始值
    if(sem_init(&m_semaphore, 0, count)) {
        // 如果sem_init返回非0值，表示初始化失败
        throw std::logic_error("sem_init error"); // 抛出异常
    }
}


/**
 * @brief 销毁信号量对象。
 */
Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore); // 销毁信号量，释放资源
}


/**
 * @brief 等待信号量。
 * 
 * 减少信号量的计数值。如果信号量的计数值为0，则调用此方法的线程将被阻塞，直到信号量的计数值被增加。
 */
void Semaphore::wait() {
    // 尝试减少信号量的计数值
    if(sem_wait(&m_semaphore)) {
        // 如果sem_wait返回非0值，表示操作失败
        throw std::logic_error("sem_wait error"); // 抛出异常
    }
}


/**
 * @brief 通知信号量。
 * 
 * 增加信号量的计数值。如果有线程因等待此信号量而被阻塞，则它们中的一个将被唤醒。
 * 信号量是一种常见的同步机制，用于控制对共享资源的访问。
 * Semaphore类的实现封装了POSIX信号量的操作，提供了一个简单的接口来等待和通知信号量。
 * 通过调用wait方法，线程可以等待信号量变为非零；而通过调用notify方法，则可以释放等待信号量的线程。这在实现线程同步和协调时非常有用。
 */
void Semaphore::notify() {
    // 尝试增加信号量的计数值
    if(sem_post(&m_semaphore)) {
        // 如果sem_post返回非0值，表示操作失败
        throw std::logic_error("sem_post error"); // 抛出异常
    }
}


/*
FiberSemaphore类通过内部维护一个并发数和等待队列来控制协程的并发执行。协程在尝试获取信号量时，如果并发数已满，协程将挂起并加入等待队列；
当信号量被释放时，等待队列中的协程将被重新调度执行。这是一种非常有效的协程同步机制。
*/
/**
 * @brief 初始化FiberSemaphore对象。
 * 
 * @param initial_concurrency 初始的并发级别，即同时允许多少个协程访问共享资源。
 */
FiberSemaphore::FiberSemaphore(size_t initial_concurrency)
    :m_concurrency(initial_concurrency) { // 初始化允许的并发数
}


/**
 * @brief 销毁FiberSemaphore对象。
 */
FiberSemaphore::~FiberSemaphore() {
    WEBSERVER_ASSERT(m_waiters.empty()); // 确保没有等待者在析构时
}

/**
 * @brief 尝试获取信号量。
 * 
 * @return 如果成功减少信号量并立即返回true，如果信号量为0则不阻塞直接返回false。
 */
bool FiberSemaphore::tryWait() {
    WEBSERVER_ASSERT(Scheduler::GetThis()); // 确保当前协程已经关联到了调度器
    {
        MutexType::Lock lock(m_mutex); // 加锁保护并发访问
        if(m_concurrency > 0u) { // 如果还有可用的并发数
            --m_concurrency; // 减少并发数
            return true; // 返回成功
        }
        return false; // 如果并发数为0，返回失败
    }
}


/**
 * @brief 等待获取信号量。
 * 
 * 如果信号量为0，则当前协程会挂起，加入到等待队列中，直到信号量可用。
 */
void FiberSemaphore::wait() {
    WEBSERVER_ASSERT(Scheduler::GetThis()); // 确保当前协程已经关联到了调度器
    {
        MutexType::Lock lock(m_mutex); // 加锁保护并发访问
        if(m_concurrency > 0u) { // 如果还有可用的并发数
            --m_concurrency; // 减少并发数
            return; // 直接返回
        }
        // 将当前协程及其调度器加入到等待队列
        m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::GetThis()));
    }
    Fiber::YieldToHold(); // 挂起当前协程，让出执行权
}


/**
 * @brief 通知信号量，唤醒等待队列中的一个协程，或者增加可用并发数。
 */
void FiberSemaphore::notify() {
    MutexType::Lock lock(m_mutex); // 加锁保护并发访问
    if(!m_waiters.empty()) { // 如果等待队列不为空
        auto next = m_waiters.front(); // 获取等待队列的第一个元素
        m_waiters.pop_front(); // 从队列中移除
        next.first->schedule(next.second); // 将等待的协程重新调度执行
    } else {
        ++m_concurrency; // 如果没有等待的协程，增加可用并发数
    }
}


}

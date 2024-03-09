#ifndef __WEBSERVER_FIBER_H__
#define __WEBSERVER_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace webserver {

class Scheduler;

/**
 * @brief 协程类
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
// 使用 std::enable_shared_from_this<Fiber> 让 Fiber 类的对象能安全地生成自己的 std::shared_ptr 实例。
// std::enable_shared_from_this 在设计需要复杂生命周期管理和跨线程操作的协程库时，
// 提供了一种安全且高效的方式来处理对象的共享所有权和自引用问题，从而简化了协程的设计和实现。
/*
生命周期管理：
协程可能在不同的执行线程之间切换，通过 std::shared_ptr 管理协程的生命周期可以确保协程在使用期间始终有效，避免了悬挂指针或野指针的问题。

自引用：
协程在执行过程中可能需要自引用，例如，注册回调函数时传递自身给调度器（Scheduler）。
使用 enable_shared_from_this 可以安全地获取当前对象的 shared_ptr，而不必担心所有权问题。

方便的资源共享：
协程可能需要访问共享资源或在协程间共享状态。
通过 shared_ptr 可以方便地在不同协程间传递资源，同时自动管理资源的生命周期。

与调度器（Scheduler）的集成：
在协程模型中，调度器负责管理和调度协程的执行。
通过让协程自身能生成 shared_ptr，调度器可以更灵活地管理协程，
例如，根据协程的状态决定其生命周期，或者在协程完成后自动释放资源。
*/
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief 协程状态
     */
    enum State {
        /// 初始化状态
        INIT,
        /// 暂停状态
        HOLD,
        /// 执行中状态
        EXEC,
        /// 结束状态
        TERM,
        /// 可执行状态
        READY,
        /// 异常状态
        EXCEPT
    };
private:
    /**
     * @brief 无参构造函数
     * @attention 每个线程第一个协程的构造
     */
    Fiber();

public:
    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    /**
     * @brief 析构函数
     */
    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() = EXEC
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     */
    void swapOut();

    /**
     * @brief 将当前线程切换到执行状态
     * @pre 执行的为当前线程的主协程
     */
    void call();

    /**
     * @brief 将当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */
    void back();

    /**
     * @brief 返回协程id
     */
    uint64_t getId() const { return m_id;}

    /**
     * @brief 返回协程状态
     */
    State getState() const { return m_state;}
public:

    /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 运行协程
     */
    static void SetThis(Fiber* f);

    /**
     * @brief 返回当前所在的协程
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 将当前协程切换到后台,并设置为READY状态
     * @post getState() = READY
     */
    static void YieldToReady();

    /**
     * @brief 将当前协程切换到后台,并设置为HOLD状态
     * @post getState() = HOLD
     */
    static void YieldToHold();

    /**
     * @brief 返回当前协程的总数量
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程调度协程
     */
    static void CallerMainFunc();

    /**
     * @brief 获取当前协程的id
     */
    static uint64_t GetFiberId();
private:
    /// 协程id
    uint64_t m_id = 0;
    /// 协程运行栈大小
    uint32_t m_stacksize = 0;
    /// 协程状态
    State m_state = INIT;
    /// 协程上下文
    ucontext_t m_ctx;
    /// 协程运行栈指针
    void* m_stack = nullptr;
    /// 协程运行函数
    std::function<void()> m_cb;
};

}

#endif

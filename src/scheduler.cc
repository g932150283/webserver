#include "scheduler.h"
#include "log.h"
#include "macro.h"
// #include "hook.h"

namespace webserver {

// 业务代码系统代码分离
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

// 线程局部变量，用于存储当前线程关联的调度器实例，确保每个线程都有自己独立的调度器引用
static thread_local Scheduler* t_scheduler = nullptr;
// 线程局部变量，用于存储当前线程的主协程，主要用于调度器内部切换协程时使用
static thread_local Fiber* t_scheduler_fiber = nullptr;

/**
 * @brief Scheduler构造函数，初始化调度器实例。
 * 
 * @param threads 指定调度器将使用的线程数量。
 * @param use_caller 指定是否将调用此构造函数的线程作为工作线程之一。
 * @param name 调度器的名称，用于日志记录和调试。
 */
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) { // 初始化成员变量m_name为提供的名称
    WEBSERVER_ASSERT(threads > 0); // 确保传入的线程数量大于0

    // 如果use_caller为true，表示将调用者线程作为调度器的一部分
    if(use_caller) {
        webserver::Fiber::GetThis(); // 确保调用者线程的主协程已被创建，这是协程调度的前提
        --threads; // 调整线程计数，因为调用者线程已被包含在内

        WEBSERVER_ASSERT(GetThis() == nullptr); // 确保当前线程未关联其他调度器
        t_scheduler = this; // 将当前调度器实例设置为此线程的调度器

        // 创建一个绑定到Scheduler::run的根协程，第三个参数true表示根协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        webserver::Thread::SetName(m_name); // 为当前线程设置名称，便于识别和调试

        t_scheduler_fiber = m_rootFiber.get(); // 将根协程设置为当前线程的主协程
        m_rootThread = webserver::GetThreadId(); // 获取并设置当前线程ID为根线程ID
        m_threadIds.push_back(m_rootThread); // 将根线程ID添加到线程ID列表中
    } else {
        m_rootThread = -1; // 如果不使用调用者线程，则将根线程ID设置为-1
    }
    m_threadCount = threads; // 设置调度器管理的线程数量
}


Scheduler::~Scheduler() {
    // 确保调度器正在停止，防止在调度器还在运行时误删除
    WEBSERVER_ASSERT(m_stopping);

    // 如果当前线程的调度器实例正是这个调度器实例，将其置为nullptr
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}
// 返回当前协程调度器
Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}
// 返回当前协程调度器的调度协程
Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

// 启动协程调度器
/*
线程安全的启动逻辑：使用互斥锁来确保修改调度器状态时的线程安全。
避免重复启动：通过检查m_stopping标志防止调度器被重复启动。
动态创建工作线程：根据配置(m_threadCount)动态创建指定数量的工作线程，每个线程将执行Scheduler::run方法。
线程命名：为每个工作线程指定了一个唯一的名称，便于调试和管理。
线程ID管理：创建线程后，将其ID保存到m_threadIds中，方便后续对线程的引用和管理。
协程支持（注释掉的部分）：提供了一种机制（虽然在这个代码段中被注释掉了），
用于在调度器启动后立即执行根协程(m_rootFiber)，这可能是为了快速启动某些预定的任务。
*/
void Scheduler::start() {
    // 使用互斥锁确保对调度器状态的修改是线程安全的
    MutexType::Lock lock(m_mutex);
    
    // 如果调度器未被标记为停止，则不进行任何操作直接返回
    if(!m_stopping) {
        return;
    }
    
    // 将调度器标记为非停止状态，即启动状态
    m_stopping = false;
    
    // 断言以确保在启动调度器前线程容器为空，避免重复启动
    WEBSERVER_ASSERT(m_threads.empty());
    
    // 根据预设的线程数调整线程容器的大小，准备存放工作线程
    m_threads.resize(m_threadCount);
    
    // 循环创建指定数量的工作线程
    for(size_t i = 0; i < m_threadCount; ++i) {
        // 对每个工作线程，使用std::bind绑定Scheduler::run为线程执行的函数
        // 线程的名称由调度器的基本名称和线程索引组成
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        
        // 将新创建的线程的ID添加到线程ID列表中，方便后续管理
        m_threadIds.push_back(m_threads[i]->getId());
    }
    
    // 解锁互斥锁，释放资源
    lock.unlock();

    // 下面的代码块是被注释的，其目的是在所有工作线程启动后，如果存在根协程m_rootFiber，则执行它
    //if(m_rootFiber) {
    //    //m_rootFiber->swapIn();  // 激活根协程
    //    m_rootFiber->call();     // 调用根协程的call方法，开始执行协程任务
    //    // 记录日志，显示根协程的状态
    //    WEBSERVER_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    //}
}


// 停止调度器的执行
/*
这个Scheduler::stop函数是一个调度器类Scheduler的成员函数，其目的是停止调度器的运行。
它首先尝试平和地停止所有操作，然后等待所有线程完成它们的任务，最后加入所有线程，确保资源被正确回收。
这个函数通过多个步骤确保调度器能够安全、有效地停止所有活动，并回收所有相关资源。
它首先判断是否可以立即停止，然后设置标志位，发送停止信号给所有工作线件，最后等待所有工作线程完成。
*/
void Scheduler::stop() {
    // 设置自动停止标志为true，表示调度器应当开始停止流程
    m_autoStop = true;

    // 检查根纤程是否存在，线程数量是否为0，以及根纤程是否处于终止(TERM)或初始化(INIT)状态
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        // 如果上述条件满足，则记录日志，表示调度器已经停止
        WEBSERVER_LOG_INFO(g_logger) << this << " stopped";
        // 设置停止标志为true
        m_stopping = true;

        // 如果调度器已处于停止状态，则直接返回
        if(stopping()) {
            return;
        }
    }

    // 检查是否在根线程上，如果是，则确保当前调度器实例就是此线程的调度器实例
    if(m_rootThread != -1) {
        WEBSERVER_ASSERT(GetThis() == this);
    } else {
        // 如果不在根线程上，确保当前调度器实例不是此线程的调度器实例
        WEBSERVER_ASSERT(GetThis() != this);
    }

    // 再次设置停止标志为true，确保在任何情况下停止流程都会执行
    m_stopping = true;
    // 遍历所有工作线程，发送停止信号（通过`tickle`函数）
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    // 如果存在根纤程，则再次发送停止信号
    if(m_rootFiber) {
        tickle();
    }

    // 如果根纤程存在，并且调度器还未完全停止
    if(m_rootFiber) {
        if(!stopping()) {
            // 调用根纤程的call方法，可能是为了处理一些清理任务
            m_rootFiber->call();
        }
    }

    // 用于存储工作线程的vector
    std::vector<Thread::ptr> thrs;
    {
        // 锁定互斥锁，交换m_threads和thrs，这样可以在不持锁的情况下等待所有线程完成
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    // 等待所有被交换出来的线程完成
    for(auto& i : thrs) {
        i->join();
    }
    // 之前的版本可能考虑在当前纤程退出，但最终这段代码没有被启用
    //if(exit_on_this_fiber) {
    //}
}

// 设置当前的协程调度器
void Scheduler::setThis() {
    t_scheduler = this;
}

/*

Scheduler::run函数是调度器的主运行循环，它负责管理和执行纤程（Fiber）以及回调函数。
这个函数使得调度器能够在多个任务（纤程或回调）之间进行切换，实现非阻塞的并发执行。
Scheduler::run函数是调度系统的核心，旨在高效地管理和执行纤程（轻量级协作线程）。
该函数负责协调纤程的运行、调度，并管理回调函数。

首先，函数通过日志记录启动调度器的信息，便于调试和跟踪。
接下来，它设置当前调度器实例为线程局部变量，使得在整个线程的执行上下文中都可以访问当前调度器实例。
如果当前线程不是根线程，则将当前纤程设置为该线程的调度纤程。
创建了一个空闲纤程idle_fiber，当调度器没有活跃的纤程可运行时，会执行此空闲纤程。空闲纤程通过绑定到调度器的idle函数来创建。
为了执行回调函数，定义了一个cb_fiber指针，用于将回调函数包装成纤程以便执行。
定义了一个FiberAndThread结构体ft，用来持有纤程和其关联的线程ID信息。

主循环：

在无限循环中，调度器不断地重置ft，检查和调度待执行的纤程或回调函数。
使用互斥锁m_mutex保护纤程队列的访问，避免在多线程环境下的数据竞争。
遍历纤程队列，选择合适的纤程执行。如果当前线程不适合执行某个纤程（基于线程ID的检查），则继续检查下一个纤程。
如果找到可执行的纤程或回调，就从队列中移除，并执行它。
如果执行的是纤程，并且纤程结束后没有处于TERM（终止）或EXCEPT（异常）状态，则根据纤程的状态决定是重新调度还是设置为HOLD状态。
如果执行的是回调函数，则将回调包装成纤程并执行。执行后同样根据状态决定后续操作。
如果没有活跃的纤程或回调要执行，并且纤程队列为空，就执行空闲纤程。空闲纤程在没有其它任务可执行时保持调度器运行。

整个run函数通过细致地管理纤程和回调函数的执行，确保调度器能高效地运行。
通过空闲纤程和主循环的设计，调度器能够在等待新任务时保持响应，同时优化了资源的使用和任务的调度策略。

此函数通过细致地控制纤程和回调的执行，实现了一个高效和灵活的任务调度系统。
它通过持续检查和执行任务队列中的纤程或回调，确保系统能够充分利用CPU资源，同时也通过空闲纤程保证了调度器在无任务执行时不会占用过多资源。
*/
// Scheduler::run函数：调度器的核心运行循环，负责管理和执行纤程以及回调任务。
void Scheduler::run() {
    // 记录调度器启动的调试信息，包括调度器的名字。
    WEBSERVER_LOG_DEBUG(g_logger) << m_name << " run";

    // 设置当前线程的Scheduler实例，用于在当前线程内部获取调度器的上下文。
    setThis();

    // 检查当前线程是否为根线程，如果不是，则设置当前纤程为调度器纤程。
    if(webserver::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    // 创建一个空闲纤程，用于调度器在没有任务执行时切换至此纤程，执行idle函数。
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

    // 用于执行回调任务的纤程，初始状态为空。
    Fiber::ptr cb_fiber;

    // 创建一个FiberAndThread结构体实例，用于记录待执行的纤程或回调以及它们对应的线程ID。
    FiberAndThread ft;

    // 调度器的主循环。
    while(true) {
        // 每轮循环开始时重置ft，准备接收新的任务。
        ft.reset();

        // 用于记录是否需要唤醒其他线程来处理任务队列。
        bool tickle_me = false;
        // 标记当前是否有活跃的任务被处理。
        bool is_active = false;

        // 用互斥锁保护任务队列，防止多线程访问冲突。
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            // 遍历任务队列，寻找适合当前线程执行的任务。
            while(it != m_fibers.end()) {
                // 如果任务指定了执行线程，并且不是当前线程，则跳过。
                if(it->thread != -1 && it->thread != webserver::GetThreadId()) {
                    ++it;
                    tickle_me = true; // 需要唤醒其他线程来处理任务。
                    continue;
                }

                // 确保任务是有效的，即有纤程或回调需要执行。
                WEBSERVER_ASSERT(it->fiber || it->cb);

                // 如果任务是纤程，并且纤程已经在执行中，则跳过。
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                // 找到一个可执行的任务，从队列中移除并准备执行。
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount; // 活跃任务计数增加。
                is_active = true;
                break;
            }
            // 如果任务队列还有任务，则设置tickle_me为true。
            tickle_me |= it != m_fibers.end();
        }

        // 如果需要唤醒其他线程处理任务队列，则调用tickle函数。
        if(tickle_me) {
            tickle();
        }

        // 如果找到的任务是纤程，并且纤程状态不是终止或异常，则执行纤程。
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn(); // 切换到该纤程执行。
            --m_activeThreadCount; // 执行完成后，活跃任务计数减少。

            // 根据纤程执行后的状态，决定是否重新调度该纤程。
            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber); // 如果纤程准备好再次执行，则重新调度。
            } else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD; // 如果纤程未终止且无异常，则设置为保持状态。
            }
            ft.reset(); // 重置ft，准备下一轮循环。
        } else if(ft.cb) { // 如果找到的任务是回调函数，则执行回调。
            // 如果已有回调纤程，则重置其任务为当前回调；否则创建新的回调纤程。
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset(); // 重置ft，准备执行回调。
            cb_fiber->swapIn(); // 切换到回调纤程执行。
            --m_activeThreadCount; // 执行完成后，活跃任务计数减少。

            // 根据回调纤程执行后的状态，决定是否重新调度。
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber); // 如果回调纤程准备好再次执行，则重新调度。
                cb_fiber.reset(); // 重置cb_fiber，准备下一轮循环。
            } else if(cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr); // 如果回调纤程终止或异常，则清空cb_fiber。
            } else { // 如果回调纤程状态为其他，则设置为保持状态，并重置cb_fiber。
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else { // 如果没有找到可执行的任务。
            if(is_active) { // 如果本轮循环处理过活跃任务，则减少活跃任务计数并继续下一轮。
                --m_activeThreadCount;
                continue;
            }
            // 如果空闲纤程已终止，则退出主循环。
            if(idle_fiber->getState() == Fiber::TERM) {
                WEBSERVER_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            // 没有任务执行时，切换到空闲纤程，直到有新的任务到来。
            ++m_idleThreadCount; // 空闲线程计数增加。
            idle_fiber->swapIn(); // 切换到空闲纤程执行。
            --m_idleThreadCount; // 执行完成后，空闲线程计数减少。

            // 根据空闲纤程执行后的状态，决定是否设置为保持状态。
            if(idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}


// 唤醒调度器以处理更多任务或进行清理操作。
/*
tickle函数的作用是向调度器发送一个信号，告知有新的任务需要处理或者有任务状态发生了变化。
它通常在任务被添加到调度器或者某些特定条件触发时被调用。
*/
void Scheduler::tickle() {
    // 记录唤醒操作的日志信息。
    WEBSERVER_LOG_INFO(g_logger) << "tickle";
}

// 检查调度器是否处于停止状态。
/*
stopping函数用于判断调度器是否应该停止运行。
它检查是否启用了自动停止，调度器是否已被明确要求停止，以及是否所有任务都已处理完成。
*/
bool Scheduler::stopping() {
    // 通过互斥锁保护访问共享资源，以避免数据竞争。
    MutexType::Lock lock(m_mutex);
    // 如果满足以下所有条件，则返回true，表示调度器正在停止：
    // 1. 自动停止被启用；
    // 2. 明确要求停止；
    // 3. 没有待处理的纤程；
    // 4. 没有活跃的线程。
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}


// 调度器空闲时执行的函数。
/*
idle函数在调度器没有任务可以执行时被调用，它会不断检查调度器是否应该停止。
如果不应该停止，它将使当前纤程让出执行权，让其他纤程有机会执行，直到新的任务到来。
*/
void Scheduler::idle() {
    // 记录进入空闲状态的日志信息。
    WEBSERVER_LOG_INFO(g_logger) << "idle";
    // 当调度器没有停止时，循环执行。
    while(!stopping()) {
        // 让当前纤程让出执行权，进入等待或保持状态，直到有新的任务需要处理。
        webserver::Fiber::YieldToHold();
    }
}


// 切换执行线程。
/*
switchTo函数用于将当前执行的纤程切换到指定的线程上执行。
如果当前调度器与调用此函数的调度器相同，且指定的线程为-1（表示不关心线程）或当前线程ID，则不会进行切换。
否则，它会调度当前纤程到指定的线程上，并让当前纤程让出执行权。
*/
void Scheduler::switchTo(int thread) {
    // 确保当前存在调度器实例。
    WEBSERVER_ASSERT(Scheduler::GetThis() != nullptr);
    // 如果当前调度器就是调用此函数的调度器，并且指定的线程是-1（表示任意线程）或当前线程，则不进行切换。
    if(Scheduler::GetThis() == this) {
        if(thread == -1 || thread == webserver::GetThreadId()) {
            return;
        }
    }
    // 将当前纤程调度到指定线程上执行。
    schedule(Fiber::GetThis(), thread);
    // 让当前纤程让出执行权，等待被再次调度执行。
    Fiber::YieldToHold();
}


// 输出调度器的状态信息到输出流。
/*
dump函数用于将调度器的当前状态输出到一个std::ostream对象中，
包括调度器的名称、线程数、活跃线程数、空闲线程数、是否正在停止等信息，以及所有线程的ID。
这个函数可以用于调试和日志记录，以便了解调度器的当前状态。
*/
std::ostream& Scheduler::dump(std::ostream& os) {
    // 将调度器的基本信息输出到提供的输出流中。
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";

    // 遍历所有线程ID，并将它们输出到流中，线程ID之间用逗号分隔。
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    // 返回输出流，以便可以进行链式调用。
    return os;
}

/*
SchedulerSwitcher是一个辅助类，其构造函数用于在创建对象时自动切换到目标调度器，而析构函数则在对象销毁时自动切换回原来的调度器。
这种设计利用了C++的RAII（资源获取即初始化）原则，
可以方便地在一个作用域内临时切换调度器，而在作用域结束时自动恢复原状态，从而简化资源管理和错误处理。
需要注意的是，switchTo方法的调用在这里缺少了必要的参数，这可能是一个示例代码的简化表示或错误。
在实际使用中，switchTo方法可能需要指定一个目标线程ID或其他参数来完成切换操作。
*/
// 构造函数，用于切换到目标调度器。
SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
    // 获取当前调度器，并保存为m_caller。
    m_caller = Scheduler::GetThis();
    // 如果提供了目标调度器，则切换到目标调度器。
    if(target) {
        target->switchTo();
    }
}

// 析构函数，用于在对象生命周期结束时切换回原来的调度器。
SchedulerSwitcher::~SchedulerSwitcher() {
    // 如果保存有原调度器，则切换回该调度器。
    if(m_caller) {
        m_caller->switchTo();
    }
}


}

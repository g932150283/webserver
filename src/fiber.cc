#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace webserver {

static Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

// 定义一个原子变量 s_fiber_id，用于生成唯一的协程 ID，初始值为 0
static std::atomic<uint64_t> s_fiber_id {0};

// 定义一个原子变量 s_fiber_count，用于记录当前创建的协程数量，初始值为 0
static std::atomic<uint64_t> s_fiber_count {0};

// 定义一个线程局部变量 t_fiber，指向当前线程正在执行的协程
// 这意味着每个线程都有自己的 t_fiber 变量副本
static thread_local Fiber* t_fiber = nullptr;

// 定义一个线程局部的智能指针变量 t_threadFiber，管理一个 Fiber 实例
// 这通常用于表示当前线程的主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

// 定义一个配置变量 g_fiber_stack_size，用于配置协程栈的大小
// 这个配置项的名称为 "fiber.stack_size"，默认值为 128 * 1024（即128KB）
// "fiber stack size" 是这个配置项的描述文字
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");


// 定义一个内存分配器类，使用 malloc 和 free 来分配和释放内存
/*
此类是一个简单的内存分配器实现，利用 C 语言标准库中的 malloc 和 free 函数来分配和释放内存。
这种分配器通常用在需要直接管理内存的场合，例如性能敏感的应用或者自定义的数据结构管理等。
然而，在实际使用中，可能需要考虑更多的内存管理策略和错误处理机制（比如 malloc 分配失败时的处理）。
*/
class MallocStackAllocator {
public:
    // 分配指定大小的内存块
    // @param size 要分配的内存大小，单位为字节
    // @return 返回指向分配的内存块的指针
    /*
    Alloc 函数：
    这个函数的目的是分配一块指定大小的内存。
    它接受一个 size 参数，该参数指定了希望分配的内存大小（以字节为单位）。
    函数内部通过调用 C 标准库函数 malloc 来实现内存分配，并返回指向分配的内存块的指针。
    */
    static void* Alloc(size_t size) {
        // 调用 malloc 函数分配内存，并返回分配的内存指针
        return malloc(size);
    }

    // 释放之前分配的内存块
    // @param vp 指向要释放的内存块的指针
    // @param size 要释放的内存块的大小，这个参数在这个函数中实际上没有被使用，
    //             因为 free 函数本身不需要知道释放的内存块的大小
    /*
    Dealloc 函数：
    此函数用于释放之前通过 Alloc 函数分配的内存块。
    它接受两个参数：vp 和 size。vp 是一个指针，指向需要被释放的内存块。
    而 size 参数在这个实现中实际上是多余的，因为 C 的 free 函数不需要知道释放内存块的具体大小。
    这里的 size 参数可能是为了匹配某个接口设计的预留，或是为了兼容其他需要知道释放大小的自定义释放函数。
    */
    static void Dealloc(void* vp, size_t size) {
        // 调用 free 函数释放内存
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

/*
这段代码是 Fiber 类的无参构造函数的实现，用于初始化一个协程对象。
它主要做了以下几件事情：
设置协程状态：
    将协程的状态 (m_state) 设置为 EXEC，表示这个协程正在执行或准备执行。这通常用于标记主协程或新创建的协程。
设置当前协程：
    调用 SetThis(this) 将当前协程设置为正在执行的协程。
    这个操作可能涉及到将 this 指针赋值给一个线程局部变量，以便在当前线程中跟踪哪个协程是当前协程。
初始化协程上下文：
    通过 getcontext(&m_ctx) 初始化协程的上下文 (m_ctx)。
    getcontext 是 POSIX 标准定义的，用于获取当前的执行上下文并保存到指定的 ucontext_t 结构体中。
    如果 getcontext 调用失败，使用 WEBSERVER_ASSERT2 断言输出错误信息。
    这里的 WEBSERVER_ASSERT2(false, "getcontext") 表明如果 getcontext 失败，则断言并可能输出错误信息 "getcontext"。
增加协程计数：
    通过 ++s_fiber_count; 增加协程的计数器，这个计数器跟踪了总共创建了多少个协程。这对于调试和监视协程的创建和销毁非常有用。
日志记录：
    使用 WEBSERVER_LOG_DEBUG 宏和全局日志变量 g_logger 输出调试信息，信息内容是 "Fiber::Fiber main"。
    这条日志表明这个无参构造函数通常用于创建主协程或一些特殊的协程。

总的来说，这个无参构造函数为协程的创建和初始化提供了基础设施，包括设置初始状态、初始化执行上下文、更新协程计数器，
并且通过日志记录协程的创建。
注意，WEBSERVER_ASSERT2 和 WEBSERVER_LOG_DEBUG 很可能是框架或应用程序特定的宏或函数，用于断言和日志记录。

m_state = EXEC; 设置协程的状态，表示协程处于执行状态。
SetThis(this); 将当前的协程实例设置为此线程的当前协程。
getcontext(&m_ctx) 获取当前的执行上下文，并保存到 m_ctx 中。这是设置协程上下文的重要步骤，以便将来可以切换回这个协程。
++s_fiber_count; 增加全局协程计数器，用于跟踪系统中创建的协程总数。
WEBSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber main"; 输出一条调试日志信息，表明主协程已创建。
*/
Fiber::Fiber() {
    // 设置协程状态为 EXEC，代表此协程正在执行或即将执行
    m_state = EXEC;

    // 设置当前协程为此协程实例。这通常涉及将 this 指针存储在某个线程局部变量中
    SetThis(this);

    // 初始化协程的上下文。如果 getcontext 调用失败，则断言
    if(getcontext(&m_ctx)) {
        // 如果获取上下文失败，则断言并输出错误信息。这里使用 WEBSERVER_ASSERT2 宏，可能是自定义的断言宏
        WEBSERVER_ASSERT2(false, "getcontext");
    }

    // 协程计数增加。这记录了创建的协程数量，用于监控和调试
    ++s_fiber_count;

    // 使用日志记录这个操作。WEBSERVER_LOG_DEBUG 是一个宏，可能被定义用来输出调试级别的日志信息
    // g_logger 是一个日志记录器实例，用于输出日志。这行日志表明主协程被创建
    WEBSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

/*
这个构造函数用于创建一个新的协程实例，为其分配一个唯一的 ID 和协程栈，并设置协程的执行上下文。
协程的回调函数 (cb) 被存储，以便在协程执行时调用。
协程栈的大小如果未指定 (stacksize 为 0)，则使用一个全局配置值 g_fiber_stack_size，该值可以通过配置系统设置。
协程的执行上下文 (m_ctx) 被初始化，并设置为执行 MainFunc 或 CallerMainFunc，取决于 use_caller 标志。
这两个函数通常是协程的入口点，其中包含了调用存储的回调函数的逻辑。
最后，输出一条日志信息，记录协程的创建，包括其唯一 ID。
通过提供 cb, stacksize, 和 use_caller 参数，这个构造函数允许更灵活地创建和配置协程，包括协程的栈大小和执行行为。
*/
// Fiber 类构造函数：接收一个回调函数，协程栈大小，以及是否使用调用者标志
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id)  // 为当前协程分配唯一 ID，并自增全局协程 ID 计数
    , m_cb(cb) { // 初始化协程的回调函数
    ++s_fiber_count; // 增加全局协程计数
    // 如果指定了协程栈大小，则使用该值；否则，使用配置的默认协程栈大小
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    // 使用 StackAllocator 分配协程栈内存
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) { // 获取当前的上下文，保存到 m_ctx
        WEBSERVER_ASSERT2(false, "getcontext"); // 如果获取上下文失败，触发断言
    }
    m_ctx.uc_link = nullptr; // 设置上下文链接为 null，表示协程结束时不自动启动另一个上下文
    m_ctx.uc_stack.ss_sp = m_stack; // 设置协程栈指针
    m_ctx.uc_stack.ss_size = m_stacksize; // 设置协程栈大小

    // 根据 use_caller 标志，为协程上下文设置适当的入口函数
    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0); // 设置协程入口为 MainFunc
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0); // 设置协程入口为 CallerMainFunc
    }

    // 记录创建协程的日志，包括协程的 ID
    WEBSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

/*
析构函数首先递减全局的协程计数器 s_fiber_count。
如果协程有分配栈内存 (m_stack 不为 nullptr)，则在释放栈内存之前，通过断言检查协程的状态。
这确保只有在协程处于适当的终止状态时，才会释放其资源。
如果协程没有分配栈内存（即为“主协程”或特定情况下的协程），则断言确保没有设置协程的回调函数 (m_cb 为 nullptr)，
并且协程处于执行状态 (EXEC)。此外，如果当前协程是正在执行的协程，就将线程局部的当前协程指针 t_fiber 设置为 nullptr。
无论哪种情况，最后都会输出一条日志信息，记录协程的销毁，包括协程的唯一 ID 和当前剩余的协程总数。
通过这种方式，Fiber 的析构函数确保了协程在销毁前，其资源被正确释放，且状态符合逻辑要求，从而维护了协程系统的健壮性。
*/
// Fiber 类的析构函数
Fiber::~Fiber() {
    --s_fiber_count; // 减少全局协程计数器

    if(m_stack) {
        // 如果协程有分配栈内存，确保协程状态是 TERM（正常终止）、EXCEPT（异常状态）或 INIT（初始状态）
        WEBSERVER_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

        // 释放分配的栈内存
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // 如果没有分配栈内存，确保没有设置回调函数
        WEBSERVER_ASSERT(!m_cb);
        // 并且协程状态必须是 EXEC（执行中）
        WEBSERVER_ASSERT(m_state == EXEC);

        // 获取当前线程正在执行的协程
        Fiber* cur = t_fiber;
        // 如果当前协程就是这个协程实例，将线程局部变量中的当前协程设置为 nullptr
        if(cur == this) {
            SetThis(nullptr);
        }
    }

    // 记录协程销毁的日志，包括协程的 ID 和剩余协程总数
    WEBSERVER_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id << " total=" << s_fiber_count;
}


/*
reset 函数允许在协程完成其任务后，用新的任务重用协程对象。这可以提高资源利用率，避免频繁创建和销毁协程对象的开销。
函数首先通过断言确保协程处于可重置的状态，即正常终止、异常状态或初始状态，同时也确保协程拥有分配的栈内存。
然后，更新协程的回调函数为新提供的 cb 函数，重新初始化协程的上下文，并设置新的入口函数为 MainFunc。
最后，协程状态被设置为 INIT，表示协程已经被重置并准备好执行新的任务。
这种设计允许协程在完成其生命周期后被重用，降低了资源消耗，并且提高了应用程序的效率。
*/
// 重置协程函数，并重置状态为 INIT，允许协程被重新使用
void Fiber::reset(std::function<void()> cb) {
    // 确保协程有分配栈内存
    WEBSERVER_ASSERT(m_stack);
    // 断言协程当前状态必须是 TERM（正常终止）、EXCEPT（异常状态）或 INIT（初始状态）
    WEBSERVER_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

    // 更新协程的回调函数
    m_cb = cb;
    // 重新获取协程的上下文
    if(getcontext(&m_ctx)) {
        // 如果获取上下文失败，则断言
        WEBSERVER_ASSERT2(false, "getcontext");
    }

    // 设置协程上下文的链接为 nullptr，表示协程结束时不自动启动另一个上下文
    m_ctx.uc_link = nullptr;
    // 设置协程上下文的栈指针
    m_ctx.uc_stack.ss_sp = m_stack;
    // 设置协程上下文的栈大小
    m_ctx.uc_stack.ss_size = m_stacksize;

    // 为协程上下文设置新的入口函数，这里固定使用 MainFunc 作为入口
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    // 重置协程状态为 INIT，表示协程已准备好被再次执行
    m_state = INIT;
}


void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        WEBSERVER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        WEBSERVER_ASSERT2(false, "swapcontext");
    }
}

/*
swapIn 方法首先通过调用 SetThis 将当前协程设置为线程的当前运行协程。
方法通过断言确保当前协程的状态不是 EXEC，因为即将切换执行到这个协程，然后将协程的状态设置为 EXEC。
使用 swapcontext 函数切换当前执行上下文到这个协程的上下文。
swapcontext 保存当前的上下文（通常是主协程或调度协程的上下文）到第一个参数指向的结构体中，并切换到第二个参数指向的上下文继续执行。
如果 swapcontext 调用失败（返回非0值），则通过断言输出错误信息，这是一种错误处理方式，确保开发者注意到上下文切换失败的情况。
这种机制允许应用程序在不同的协程之间切换执行流，实现了协程的并发执行，是实现轻量级并发编程的基础。
*/
// 切换到当前协程执行
void Fiber::swapIn() {
    // 设置当前线程的正在运行的协程为 this 协程
    SetThis(this);
    
    // 断言当前协程的状态不是 EXEC（执行中），因为我们即将切换到这个协程执行
    WEBSERVER_ASSERT(m_state != EXEC);
    
    // 将协程状态设置为 EXEC，表示协程正在执行中
    m_state = EXEC;
    
    // 使用 swapcontext 函数切换当前执行上下文到当前协程的上下文
    // Scheduler::GetMainFiber()->m_ctx 是当前线程的主协程上下文，m_ctx 是当前协程的上下文
    // 这里实际上保存了当前执行流的上下文到主协程，然后切换到当前协程的上下文执行
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        // 如果上下文切换失败，则断言
        WEBSERVER_ASSERT2(false, "swapcontext");
    }
}

/*
swapOut 方法首先将线程的当前运行协程设置为主协程，这是通过调用 SetThis 方法实现的，参数是通过调用 Scheduler::GetMainFiber() 获取的主协程。
接着，使用 swapcontext 函数将当前协程的执行上下文切换回主协程。
这个过程中，当前协程的上下文会被保存到 m_ctx 中，以便之后可以再次切换回来继续执行。
如果 swapcontext 调用失败，即返回非0值，则使用 WEBSERVER_ASSERT2 宏进行断言，提示开发者注意到上下文切换失败的情况。
此方法的主要用途是在需要暂停当前协程执行，并切换回到主协程执行流时使用。
这在协程调度中是常见的操作，允许应用程序实现更复杂的并发逻辑，如协程间的相互切换、事件等待、IO操作等待等。
*/
// 切换到后台执行，通常意味着返回到主协程或调度协程
void Fiber::swapOut() {
    // 设置当前线程的正在运行的协程为主协程
    // Scheduler::GetMainFiber() 返回主协程的智能指针
    SetThis(Scheduler::GetMainFiber());
    
    // 使用 swapcontext 函数切换当前执行上下文回到主协程的上下文
    // 第一个参数 &m_ctx 是当前协程的上下文，保存当前协程的状态
    // 第二个参数是主协程的上下文，切换执行流到主协程
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        // 如果上下文切换失败，则断言，表示有严重的运行时错误
        WEBSERVER_ASSERT2(false, "swapcontext");
    }
}


// 设置当前协程
// 这个静态成员函数用于更新指向当前正在执行的Fiber对象的指针。
// @param f 指向一个Fiber实例的指针，表示将要成为当前协程的Fiber。
/*
这个函数SetThis是Fiber类的一部分，用于设置当前正在执行或应该被视为“当前活动”的协程。
在协程编程模型中，协程（在这里是通过Fiber类表示）是执行流的一个单位，可以独立调度。
*/
void Fiber::SetThis(Fiber* f) {
    // 将全局或类范围内的静态变量t_fiber更新为指向传入的Fiber对象。
    t_fiber = f; 
}


// 返回当前协程的智能指针
// 如果没有当前协程，则创建一个新的主协程，并返回其智能指针
/*
这个函数的目的是提供对当前激活协程的访问，实现了以下逻辑：

检查当前协程是否存在：通过检查if(t_fiber)，来确定是否已有一个当前协程存在。如果是，使用shared_from_this()返回当前协程的智能指针。

创建并设置主协程：如果没有当前协程，函数会创建一个新的Fiber实例，并尝试将其设置为当前协程。这里存在一些逻辑上的问题和潜在的错误：

WEBSERVER_ASSERT(t_fiber == main_fiber.get());
这行代码会在main_fiber被创建后立即断言t_fiber（当前应该为空）等于main_fiber.get()，这显然是一个错误，因为t_fiber此时应该还没有被设置，所以这个断言可能总是失败。
t_threadFiber = main_fiber;
这行意图是将新创建的main_fiber设置为当前线程的协程，但它实际上应该是要设置t_fiber而不是t_threadFiber。
可能这里的意图是使用不同的变量来存储当前线程的主协程引用，但上下文不足以明确这一点。
在返回新创建的主协程之前，它应该将t_fiber设置为新的main_fiber，但代码中遗漏了这一步骤。
*/
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        // 如果当前协程已存在，返回其智能指针
        return t_fiber->shared_from_this();
    }

    // 如果没有当前协程，创建一个新的Fiber实例作为主协程
    Fiber::ptr main_fiber(new Fiber);

    // 断言确保当前没有其他协程被设置为主协程
    WEBSERVER_ASSERT(t_fiber == main_fiber.get());

    // 更新静态成员变量t_threadFiber为新创建的主协程
    t_threadFiber = main_fiber;

    // 返回新创建的主协程的智能指针
    return t_fiber->shared_from_this();
}


// 协程切换到后台，并且设置为Ready状态
/*
获取当前协程：通过调用GetThis函数，获取代表当前正在执行的协程的智能指针。
这一步是必须的，因为我们需要对当前协程进行操作。

断言检查：
使用WEBSERVER_ASSERT（很可能是自定义的断言宏）来确保当前协程的状态是EXEC（执行中）。
这是一个安全检查，确保只有当协程确实在执行中时，才能将其切换到READY状态。

更新状态：
将当前协程的状态从EXEC更改为READY。这表示协程已经准备好被再次调度执行。

切换协程：
通过调用swapOut方法，实现将当前协程挂起，让出执行权。
具体来说，这通常涉及保存当前协程的上下文（例如，寄存器状态、堆栈信息等），然后切换到另一个协程或主程序执行流。
swapOut的具体实现取决于协程库的设计，但它的目的是允许协程之间的切换。

这个函数的设计意图是允许当前执行的协程主动让出CPU使用权，转而执行其他已经准备就绪的协程。这是协程编程中常见的一个操作，允许更灵活的任务调度和并发控制。
*/
void Fiber::YieldToReady() {
    // 获取当前协程的智能指针
    Fiber::ptr cur = GetThis();

    // 断言当前协程的状态必须是EXEC（执行中）
    WEBSERVER_ASSERT(cur->m_state == EXEC);

    // 将当前协程的状态设置为READY（准备就绪）
    cur->m_state = READY;

    // 调用swapOut方法，将当前协程切换到后台
    cur->swapOut();
}


// 协程切换到后台，并且原意是设置为Hold状态，但该行为被注释
/*
获取当前协程：
首先，通过GetThis函数获取代表当前正在执行的协程的智能指针。这一步骤是必需的，因为我们要对当前协程进行操作。

断言检查：
使用WEBSERVER_ASSERT（可能是一个自定义的断言宏）来确保当前协程的状态是EXEC。这是一个安全检查，确保只有当协程确实处于执行状态时，才能进行后续操作。

状态设置为HOLD（被注释）：
本来应该将当前协程的状态更改为HOLD，表示协程被暂停或挂起，不过这个操作在代码中被注释掉了。
如果未来需要修改协程的状态为HOLD，可能需要取消这一行的注释或通过其他方式来设置状态。

切换协程：
通过调用swapOut方法，实现将当前协程挂起，让出执行权。这通常涉及保存当前协程的上下文（如寄存器状态、堆栈信息等），然后切换到另一个协程或主程序执行流。

这个函数与YieldToReady非常相似，主要区别在于它原本打算将协程的状态设置为HOLD而不是READY。
这样的设计允许协程在不同的状态之间灵活切换，从而实现复杂的调度逻辑和资源管理。
*/
void Fiber::YieldToHold() {
    // 获取当前协程的智能指针
    Fiber::ptr cur = GetThis();

    // 断言当前协程的状态必须是EXEC（执行中）
    WEBSERVER_ASSERT(cur->m_state == EXEC);

    // 将当前协程的状态设置为HOLD（被暂停）-- 这一行被注释掉了
    // cur->m_state = HOLD;

    // 调用swapOut方法，将当前协程切换到后台
    cur->swapOut();
}


//总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

/*
执行协程任务：
MainFunc首先尝试执行协程的回调函数m_cb。这是协程被创建时指定的任务。

异常处理：
如果任务执行过程中抛出异常，MainFunc会捕获这些异常。根据异常的类型，它会将协程状态设置为EXCEPT并记录相关的错误信息。这包括标准异常和其他类型的异常。

状态更新：
任务正常完成后，MainFunc会将协程状态设置为TERM，表示协程已经终止。如果发生异常，则设置为EXCEPT。

协程切换：
执行完毕后（无论是正常结束还是异常终止），MainFunc会释放当前协程的智能指针并调用swapOut，这样做是为了将控制权交回给协程调度器或主线程，允许其他协程运行。

逻辑断言：
最后，MainFunc使用WEBSERVER_ASSERT2进行断言，确保执行流不会到达swapOut调用之后的代码。
到达那里通常意味着有逻辑错误，因为协程的执行应该在swapOut之后在某个时刻被再次恢复，而不是继续执行MainFunc之后的代码。

这个函数是协程执行流程中的核心部分，负责执行协程的主要逻辑，并在执行完毕后正确地管理协程状态和执行流的转移。
*/
void Fiber::MainFunc() {
    // 获取当前协程的智能指针
    Fiber::ptr cur = GetThis();
    // 确保当前协程存在
    WEBSERVER_ASSERT(cur);

    try {
        // 尝试执行协程的回调函数
        cur->m_cb();
        // 回调执行完成后，清除回调以防止重复执行
        cur->m_cb = nullptr;
        // 设置协程的状态为TERM，表示任务正常完成
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        // 如果执行过程中抛出了标准异常，设置协程状态为EXCEPT
        cur->m_state = EXCEPT;
        // 记录异常信息到日志
        WEBSERVER_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << webserver::BacktraceToString();
    } catch (...) {
        // 捕获非标准异常
        cur->m_state = EXCEPT;
        // 记录异常信息到日志
        WEBSERVER_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << webserver::BacktraceToString();
    }

    // 获取当前协程的原始指针，并重置智能指针
    auto raw_ptr = cur.get();
    cur.reset();
    // 切换出当前协程
    raw_ptr->swapOut();

    // 断言以确保不应该到达这里，如果到达，意味着逻辑错误
    WEBSERVER_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}


void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    WEBSERVER_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        WEBSERVER_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << webserver::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        WEBSERVER_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << webserver::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    WEBSERVER_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

}

}

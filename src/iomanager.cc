#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace webserver {

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

enum EpollCtlOp {
};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op) {
    switch((int)op) {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

/**
 * 功能描述: 根据指定的事件类型，获取对应的事件上下文对象引用。
 *
 * 参数:
 *   - event: IOManager::Event类型，指定的事件类型，例如IOManager::READ或IOManager::WRITE。
 *
 * 返回值: IOManager::FdContext::EventContext的引用。
 *   - 根据输入的事件类型，返回对应的read或write事件上下文对象的引用。
 *   - 如果输入的事件类型不是READ或WRITE，则抛出std::invalid_argument异常。
 *
 * 异常:
 *   - std::invalid_argument: 如果提供的事件类型无效或不支持，函数将抛出此异常。
 */
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    // 使用switch语句判断事件类型
    switch(event) {
        // 如果事件类型为READ，返回read事件上下文对象的引用
        case IOManager::READ:
            return read;
        // 如果事件类型为WRITE，返回write事件上下文对象的引用
        case IOManager::WRITE:
            return write;
        // 如果事件类型不是READ或WRITE，执行默认操作
        default:
            // 断言失败，提示“getContext”错误，这通常用于开发阶段的错误检查
            WEBSERVER_ASSERT2(false, "getContext");
    }
    // 如果switch语句中没有找到匹配的case并正常返回，抛出异常
    throw std::invalid_argument("getContext invalid event");
}


/**
 * 功能描述: 重置指定的事件上下文对象，清除其中的调度器、协程和回调函数。
 *
 * 参数:
 *   - ctx: EventContext的引用，指定要重置的事件上下文对象。
 *
 * 返回值: 无返回值。
 */
void IOManager::FdContext::resetContext(EventContext& ctx) {
    // 将事件上下文对象中的调度器指针设置为nullptr，表示没有调度器
    ctx.scheduler = nullptr;
    // 重置事件上下文对象中的协程，使用std::shared_ptr的reset方法释放原有协程并置为空
    ctx.fiber.reset();
    // 将事件上下文对象中的回调函数指针设置为nullptr，表示没有回调函数
    ctx.cb = nullptr;
}


/**
 * 功能描述: 触发指定的事件，并根据事件上下文安排任务的执行。
 *
 * 参数:
 *   - event: Event类型，表示需要触发的事件类型。
 *
 * 返回值: 无返回值。
 *
 * 详细说明:
 *   1. 首先，断言当前事件上下文中是否含有指定的事件。如果没有，这意味着程序存在逻辑错误。
 *   2. 然后，从当前的事件掩码中移除指定的事件，更新事件状态。
 *   3. 获取与指定事件对应的事件上下文对象。
 *   4. 检查事件上下文中是否设置了回调函数：
 *      - 如果设置了回调函数，就通过调度器安排执行该回调函数。
 *      - 如果没有设置回调函数，那么假定已经设置了协程，通过调度器安排执行该协程。
 *   5. 触发事件后，将事件上下文中的调度器指针清空，表示该事件上下文的任务已经被安排执行。
 */
void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    // 断言确保要触发的事件已经被注册
    WEBSERVER_ASSERT(events & event);

    // 更新事件状态，从当前的事件掩码中移除被触发的事件
    events = (Event)(events & ~event);

    // 根据事件类型获取对应的事件上下文对象
    EventContext& ctx = getContext(event);

    // 检查事件上下文中是否有设置回调函数cb
    if(ctx.cb) {
        // 如果有回调函数，则通过调度器安排回调函数的执行
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        // 如果没有设置回调函数，则假定设置了协程，通过调度器安排协程的执行
        ctx.scheduler->schedule(&ctx.fiber);
    }

    // 事件处理完毕，清空事件上下文中的调度器指针
    ctx.scheduler = nullptr;
}



/**
 * IOManager构造函数，用于初始化异步IO管理器。
 * @param threads 数量，指定创建的线程数，用于处理异步IO事件。
 * @param use_caller 布尔值，指定是否将调用者线程也用作事件处理线程之一。
 * @param name 字符串，为当前IO管理器实例指定一个名称，便于调试和管理。
 * 构造函数，初始化IOManager，继承自Scheduler类。参数包括线程数、是否使用调用者线程和名称。
 * 这段代码主要完成以下功能：

初始化IOManager实例，同时也初始化它的基类Scheduler。
创建一个epoll实例来管理异步IO事件，其最大句柄数设置为5000。
创建一个管道用于唤醒阻塞在epoll_wait调用中的线程。
设置epoll事件监听管道的读端，确保在有数据（比如唤醒信号）到来时，能够及时处理。
将管道读端设置为非阻塞模式，并添加到epoll实例中监听事件。
调整内部资源，以便能够管理至少32个上下文。
启动Scheduler，开始执行调度任务。
这是一个针对异步IO和任务调度设计的高效构造函数，通过使用epoll和管道机制来实现高效的任务唤醒和管理。
 */
IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    :Scheduler(threads, use_caller, name) { // 调用基类Scheduler的构造函数进行初始化
    m_epfd = epoll_create(5000); // 创建epoll实例，指定最大句柄数为5000
    WEBSERVER_ASSERT(m_epfd > 0); // 断言epoll实例创建成功

    int rt = pipe(m_tickleFds); // 创建一个管道，用于唤醒epoll_wait
    WEBSERVER_ASSERT(!rt); // 断言管道创建成功

    epoll_event event; // 定义一个epoll事件
    memset(&event, 0, sizeof(epoll_event)); // 初始化epoll事件结构体
    event.events = EPOLLIN | EPOLLET; // 设置事件类型为输入事件和边缘触发
    event.data.fd = m_tickleFds[0]; // 将管道读端设置为epoll事件的文件描述符

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK); // 设置管道读端为非阻塞模式
    WEBSERVER_ASSERT(!rt); // 断言设置成功

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event); // 向epoll实例中添加管道读端的事件监听
    WEBSERVER_ASSERT(!rt); // 断言事件添加成功

    contextResize(32); // 预分配或调整某些内部资源的大小，以适应至少32个上下文

    start(); // 启动Scheduler，开始调度执行
}


/**
 * IOManager析构函数
 * 用于在IOManager实例生命周期结束时清理资源。
 * 
 * 
析构函数的作用主要包括：

停止IOManager的所有操作，包括任何正在进行的异步IO操作和事件循环。
关闭epoll实例的文件描述符，确保相关资源被正确释放。
关闭用于唤醒epoll等待状态的管道的读端和写端，释放管道资源。
遍历所有的文件描述符上下文（如果有），并释放它们所占用的内存。
这是必要的，因为每个文件描述符上下文可能关联着一些资源，如回调函数或缓冲区，它们需要在对象销毁时被清理，以防止内存泄漏。
 */
IOManager::~IOManager() {
    stop(); // 停止所有调度和事件处理，确保没有活动的事件或任务。

    close(m_epfd); // 关闭epoll实例的文件描述符，释放epoll相关资源。

    close(m_tickleFds[0]); // 关闭用于通知事件循环的管道读端。
    close(m_tickleFds[1]); // 关闭用于通知事件循环的管道写端。

    // 遍历并清理所有的文件描述符上下文对象
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) { // 如果上下文对象存在
            delete m_fdContexts[i]; // 删除对象，释放内存
            m_fdContexts[i] = nullptr; // 避免野指针，确保指针被正确地置空
        }
    }
}



/**
 * 调整文件描述符上下文数组的大小。
 * 该方法确保m_fdContexts数组有足够的容量来管理所有的文件描述符上下文。
 * 如果数组大小增加，新的位置将被初始化为新的FdContext对象。
 * 
 * @param size 新的数组大小。如果该值大于当前数组的大小，数组将扩展。
 * 
 * 这个方法是IOManager用于管理文件描述符上下文(FdContext)的重要组成部分，尤其在需要处理大量文件描述符时。
 * 通过动态调整m_fdContexts数组的大小，IOManager可以确保有足够的空间来存储每个文件描述符的上下文信息。
 * 此外，通过为新扩展的部分初始化FdContext对象，保证了即使是新加入的文件描述符也能够立即被正确管理。
 * 这种设计使得IOManager在处理IO事件时更加灵活和高效。
 * 
 * IOManager::contextResize函数的工作流程是专门设计来调整IO管理器中管理文件描述符（FD）上下文数组的大小，
 * 并确保每个文件描述符都有一个对应的上下文对象。

调整数组大小：
首先，根据传入的size参数，调整m_fdContexts数组的大小。
这一步是通过resize方法实现的，它可以根据需要增大或减小数组的大小。
如果size大于当前数组的大小，则数组将被扩展；
如果size小于当前大小，该方法也能确保数组被正确缩减。

遍历数组并初始化新元素：
接下来，函数遍历m_fdContexts数组的每个元素。这一步是为了确认哪些新添加的位置是空的（即nullptr），需要初始化。
对于数组中的每个位置，如果发现位置上的指针是空的（即之前没有分配FdContext对象），那么就在该位置上创建一个新的FdContext对象。

设置文件描述符上下文：
新创建的FdContext对象需要被正确初始化。这包括将FdContext对象的fd成员变量设置为其在数组中的索引。
这一步骤是非常重要的，因为它确保了每个FdContext对象都能够明确地知道它是为哪个文件描述符服务的。

通过这个流程，IOManager::contextResize方法不仅能够根据需要动态地调整文件描述符上下文数组的大小，
而且还确保了数组中的每个位置都被正确地初始化并关联到一个具体的文件描述符。
这样，无论何时添加新的文件描述符到IO管理器中，都可以保证有一个对应的上下文对象来管理相关的事件和状态，从而提高了IO管理的效率和可靠性。
 */
void IOManager::contextResize(size_t size) {
    // 调整m_fdContexts数组的大小至指定的size
    m_fdContexts.resize(size);

    // 遍历m_fdContexts数组
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        // 如果当前位置未初始化（即为nullptr）
        if(!m_fdContexts[i]) {
            // 为当前位置创建一个新的FdContext对象
            m_fdContexts[i] = new FdContext;
            // 设置新创建的FdContext对象的文件描述符为其在数组中的索引
            m_fdContexts[i]->fd = i;
        }
    }
}


/**
 * 向指定的文件描述符添加事件监听。
 * 
 * @param fd 文件描述符，标识一个特定的IO资源。
 * @param event 需要监听的事件类型，例如读、写事件等。
 * @param cb 当事件触发时，将调用这个回调函数。
 * 
 * @return 成功时返回0，失败时返回-1。
 * 
 * 这段代码定义了IOManager::addEvent方法，其目的是向IO管理器添加一个事件监听。
 * 这个函数处理了一系列复杂的逻辑，包括处理多线程环境下的资源访问、更新事件监听上下文、并将新的事件添加到epoll实例中
 *  
 * IOManager::addEvent函数的工作流程非常关键于异步事件处理的实现。这个流程可以分解为以下几个主要步骤：
 * 
 * 获取文件描述符上下文：
 * 首先，尝试以读锁的方式锁定资源，以减少对性能的影响。检查是否已经为该文件描述符（fd）创建了上下文（FdContext）。
 * 如果上下文已经存在，则直接使用该上下文。如果不存在，则释放读锁，加上写锁，扩展上下文数组的大小以包含该文件描述符，并为它创建新的上下文。
 * 
 * 设置事件和回调：
 * 在确定了文件描述符的上下文之后，方法会锁定该上下文，防止并发修改。
 * 检查要添加的事件是否已经被监听。如果是，则记录错误并断言失败，因为同一事件不应被重复添加。
 * 确定是修改（EPOLL_CTL_MOD）还是添加（EPOLL_CTL_ADD）事件到epoll实例。如果文件描述符上下文已经有事件被监听，则使用修改操作；否则，使用添加操作。
 * 
 * 调用epoll_ctl：
 * 使用epoll_ctl函数向epoll实例添加或修改事件监听。设置事件类型为边缘触发（EPOLLET）并合并新的事件类型，然后将文件描述符上下文作为回调数据。
 * 如果epoll_ctl调用失败，则记录错误信息并返回-1。
 * 
 * 更新上下文和计数器：
 * 成功调用epoll_ctl后，增加等待处理的事件计数器，并更新文件描述符上下文中的事件掩码。
 * 获取或初始化对应事件的上下文，并确保该上下文在之前未被设置。
 * 
 * 设置调度器和回调：
 * 设置事件上下文的调度器为当前调度器实例。
 * 如果提供了回调函数，则设置该回调；如果未提供，则使用当前的协程作为回调执行的环境，并确保当前协程的状态为执行状态。
 * 
 * 完成并返回：
 * 在成功完成上述步骤后，函数返回0，表示事件添加成功。
 * 整个流程体现了异步事件处理的核心：高效地监听和响应IO事件，同时确保线程安全和资源的正确管理。
 * 通过细致地管理每个文件描述符的事件监听和回调，IOManager能够高效地处理大量并发的IO操作。
 */
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr; // 定义文件描述符上下文指针。
    
    // 首先尝试以读锁的方式获取锁，避免写锁的高开销。
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) { // 检查fd是否已经有对应的上下文。
        fd_ctx = m_fdContexts[fd]; // 获取文件描述符上下文。
        lock.unlock(); // 解锁读锁。
    } else { // 如果没有对应的上下文，则需要扩展上下文数组并加写锁。
        lock.unlock(); // 先解锁读锁。
        RWMutexType::WriteLock lock2(m_mutex); // 加写锁。
        contextResize(fd * 1.5); // 调整上下文数组的大小。
        fd_ctx = m_fdContexts[fd]; // 获取新的文件描述符上下文。
    }

    // 对文件描述符上下文加锁，避免并发修改。
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 检查是否已经监听了该事件，如果是，则记录错误并断言。
    if(WEBSERVER_UNLIKELY(fd_ctx->events & event)) {
        WEBSERVER_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        WEBSERVER_ASSERT(!(fd_ctx->events & event));
    }

    // 判断是修改现有事件监听（EPOLL_CTL_MOD）还是添加新的事件监听（EPOLL_CTL_ADD）。
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent; // 定义epoll事件。
    epevent.events = EPOLLET | fd_ctx->events | event; // 设置事件类型，包括边缘触发和新事件。
    epevent.data.ptr = fd_ctx; // 将文件描述符上下文作为回调数据。

    // 调用epoll_ctl添加或修改事件监听。
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) { // 如果调用失败，记录错误并返回-1。
        WEBSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    // 事件添加成功，增加待处理事件计数器。
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event); // 更新上下文中的事件掩码。
    
    // 获取或初始化事件上下文。
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    // 确保事件上下文在此之前没有被设置。
    WEBSERVER_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    // 设置事件上下文的调度器为当前调度器实例。
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) { // 如果提供了回调函数，则设置回调函数。
        event_ctx.cb.swap(cb);
    } else { // 否则，设置当前协程为事件处理协程。
        event_ctx.fiber = Fiber::GetThis();
        // 断言当前协程状态为EXEC。
        WEBSERVER_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                      ,"state=" << event_ctx.fiber->getState());
    }
    return 0; // 成功添加事件，返回0。
}


/**
 * 删除指定文件描述符上的一个事件监听。
 * 
 * @param fd 文件描述符，表示需要删除事件的IO资源。
 * @param event 要删除的事件类型（如EPOLLIN、EPOLLOUT等）。
 * @return 如果成功删除事件，则返回true；如果指定的事件不存在或删除失败，则返回false。
 * 
 * 功能描述: 从IO管理器中删除指定文件描述符(fd)上的某个事件(event)监听。
 *
 * 参数:
 *   - fd: 整型，表示需要删除事件的文件描述符。
 *   - event: Event类型，表示需要删除的事件类型（如读、写等）。
 *
 * 返回值: 布尔类型。
 *   - 返回true表示事件成功从指定文件描述符上删除。
 *   - 返回false表示删除事件失败，可能是由于文件描述符超出范围或指定的事件未在文件描述符上注册。
 */
/**
 * IOManager::delEvent方法的工作流程是为了从IO管理器中删除一个特定的事件监听。下面是这个函数的主要步骤：

检查文件描述符有效性：
使用读锁锁定资源，以检查提供的文件描述符（fd）是否有效且已有上下文。如果文件描述符超出了当前上下文数组的范围，则直接返回false。

获取文件描述符上下文：
获取对应文件描述符的上下文（FdContext），然后释放读锁。

锁定文件描述符上下文：
对文件描述符上下文加锁，以保证在修改事件时的线程安全。

检查事件是否已被监听：
如果尝试删除的事件并未被监听（即，该事件不在文件描述符的事件掩码中），则直接返回false。

计算新的事件掩码并决定操作：
通过移除指定的事件，计算新的事件掩码。如果新的掩码非空，则需要修改（EPOLL_CTL_MOD）现有的事件监听；
如果新的掩码为空，则意味着没有更多事件需要监听，此时将使用删除（EPOLL_CTL_DEL）操作。

调用epoll_ctl更新epoll监听：
根据新的事件掩码和决定的操作（修改或删除），更新epoll的监听设置。
如果epoll_ctl调用失败（比如，由于文件描述符无效），则记录错误并返回false。

更新上下文和计数器：
成功更新epoll后，减少等待处理的事件计数器，并更新文件描述符上下文中的事件掩码以反映已删除的事件。
获取事件的上下文，并调用resetContext方法重置事件上下文，清除任何关联的调度器、协程或回调函数。

完成操作：
返回true表示成功删除了事件监听。

这个流程紧密跟随异步事件管理的核心原则，确保了事件可以被安全、有效地添加和删除。
通过仔细管理每个文件描述符的事件监听和上下文状态，IOManager提供了强大的基础设施来支持复杂的异步IO操作。
*/
bool IOManager::delEvent(int fd, Event event) {
    // 加读锁以安全地访问文件描述符上下文数组
    RWMutexType::ReadLock lock(m_mutex);
    // 如果文件描述符超出当前管理的范围，则无法删除事件，返回false
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    // 获取指定文件描述符的上下文
    FdContext* fd_ctx = m_fdContexts[fd];
    // 释放读锁，因为后续操作将针对特定的文件描述符上下文
    lock.unlock();

    // 对文件描述符上下文加锁，保证线程安全
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 如果要删除的事件并未被监听，直接返回false
    if(WEBSERVER_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 计算删除事件后的新事件掩码
    Event new_events = (Event)(fd_ctx->events & ~event);
    // 确定epoll的操作类型：如果还有其他事件被监听，则修改；否则，删除
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    // 设置epoll事件类型为边缘触发，并应用新的事件掩码
    epevent.events = EPOLLET | new_events;
    // 将文件描述符上下文关联到epoll事件数据中
    epevent.data.ptr = fd_ctx;

    // 调用epoll_ctl来更新epoll监听状态
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    // 如果更新失败，记录错误信息并返回false
    if(rt) {
        WEBSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 成功更新epoll监听状态后，减少待处理的事件计数
    --m_pendingEventCount;
    // 更新文件描述符上下文中的事件掩码
    fd_ctx->events = new_events;
    // 获取并重置指定事件的上下文
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    // 成功删除事件，返回true
    return true;
}




/**
 * 取消对指定文件描述符上的事件监听，并触发该事件的处理。
 * 
 * @param fd 文件描述符，表示需要取消事件的IO资源。
 * @param event 要取消的事件类型（如EPOLLIN、EPOLLOUT等）。
 * @return 如果成功取消事件并触发处理，则返回true；如果指定的事件不存在或操作失败，则返回false。
 * 
 * @details IOManager::cancelEvent方法的目的是取消对指定文件描述符上某个事件的监听，并且触发该事件的处理程序。
 * 这个方法对于在某些操作需要被中断或不再需要继续监听某个事件时非常有用。
 * 
 * 通过这个方法，可以灵活地控制事件的监听状态，并且确保即使在取消事件监听的同时，也能够及时处理相关的逻辑，比如执行清理或者资源释放等操作。
 * 这对于实现复杂的IO管理逻辑和保证系统资源正确使用非常关键。
 * 
 * IOManager::cancelEvent函数的工作流程主要涉及取消对指定文件描述符（fd）上注册的特定事件（event）的监听，并立即触发该事件的处理程序。
 * 这个过程确保了即使某个事件不再被监听，其相关的处理逻辑仍然能够被执行。
 * 
 * 下面是该函数的具体工作流程：

安全检查和锁定：
使用读写锁（RWMutexType::ReadLock）安全地访问全局的文件描述符上下文数组（m_fdContexts）。这一步保证了在多线程环境下对数据的安全访问。
检查传入的文件描述符（fd）是否在管理的范围内。如果超出范围，则函数返回false，表示取消事件失败。

获取文件描述符上下文：
根据文件描述符（fd）获取对应的上下文对象（FdContext）。每个文件描述符都有一个与之对应的上下文对象，用于管理该文件描述符上的事件监听和处理。

检查事件是否被监听：
在文件描述符上下文的锁定范围内（FdContext::MutexType::Lock），检查要取消的事件是否实际上被监听。如果该事件未被监听，函数返回false。

更新epoll监听：
根据是否还有其他事件被监听来决定是修改（EPOLL_CTL_MOD）还是完全删除（EPOLL_CTL_DEL）对该文件描述符的epoll监听。
准备epoll_event结构，设置新的事件掩码（去除了要取消的事件），并通过epoll_ctl函数更新epoll的监听设置。

处理epoll_ctl调用失败：
如果epoll_ctl调用失败（例如，因为文件描述符无效），则记录错误并返回false，表示事件取消失败。

触发事件处理程序：
如果epoll_ctl成功更新了监听设置，那么使用FdContext的triggerEvent方法立即触发对应事件的处理程序。
这是因为取消事件的目的不仅是停止监听，还要确保相关的清理或逻辑得到执行。

更新计数并返回成功：

成功完成以上步骤后，减少待处理事件的计数（m_pendingEventCount）并返回true，表示事件成功取消并触发了处理程序。
这个流程提供了一种灵活且高效的方式来管理文件描述符上的事件监听和处理，特别是在需要取消特定事件监听并立即执行相关处理时非常有用。
这对于实现高性能和响应快速的IO管理至关重要。
 */
bool IOManager::cancelEvent(int fd, Event event) {
    // 加读锁以安全地访问文件描述符上下文数组
    RWMutexType::ReadLock lock(m_mutex);
    // 如果文件描述符超出当前管理的范围，直接返回false
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    // 获取指定文件描述符的上下文
    FdContext* fd_ctx = m_fdContexts[fd];
    // 释放读锁
    lock.unlock();

    // 对文件描述符上下文加锁，保证线程安全
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 如果要取消的事件并未被监听，直接返回false
    if(WEBSERVER_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 计算取消指定事件后的新事件掩码
    Event new_events = (Event)(fd_ctx->events & ~event);
    // 确定epoll的操作类型：如果还有其他事件被监听，则修改；否则，删除
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    // 设置epoll事件类型为边缘触发，并应用新的事件掩码
    epevent.events = EPOLLET | new_events;
    // 将文件描述符上下文关联到epoll事件数据中
    epevent.data.ptr = fd_ctx;

    // 调用epoll_ctl来更新epoll监听状态
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    // 如果更新失败，记录错误信息并返回false
    if(rt) {
        WEBSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 成功更新epoll监听状态后，触发指定事件的处理
    fd_ctx->triggerEvent(event);
    // 减少待处理的事件计数
    --m_pendingEventCount;
    // 成功取消事件并触发处理，返回true
    return true;
}


/**
 * 取消指定文件描述符上所有事件的监听，并触发对应的事件处理程序。
 * 
 * @param fd 文件描述符，表示需要取消所有事件监听的IO资源。
 * @return 如果成功取消所有事件并触发处理程序，则返回true；否则返回false。
 * 
 * 加读锁：
 * 首先，通过读写锁（RWMutexType）加读锁来安全地访问文件描述符上下文数组（m_fdContexts）。
检查文件描述符有效性：
检查传入的文件描述符fd是否有效。如果fd超出了m_fdContexts的大小，表示没有对应的文件描述符上下文，函数返回false。

获取文件描述符上下文并释放读锁：
通过索引访问m_fdContexts数组获取对应的FdContext对象，随后释放之前加的读锁。

加上下文锁：
对获取到的文件描述符上下文对象加锁（FdContext::MutexType::Lock），以保护对该上下文的操作。

检查注册事件：
检查该文件描述符是否有注册的事件（通过检查fd_ctx->events）。如果没有注册任何事件，函数返回false。

准备从epoll中删除事件：
设置epoll_event结构体，准备从epoll监听中删除该文件描述符上的所有事件。

执行epoll删除操作：
通过调用epoll_ctl函数，使用EPOLL_CTL_DEL操作从epoll实例中移除文件描述符。

错误处理：
如果epoll_ctl操作失败（即返回值不为0），记录错误信息并返回false。

触发事件处理程序：
如果文件描述符上注册了读或写事件，分别触发这些事件的处理程序，并递减待处理事件的计数。

断言事件已全部处理：
最后，断言该文件描述符上没有任何剩余的事件监听（即fd_ctx->events == 0）。

操作成功完成：
函数最终返回true，表示成功取消了该文件描述符上的所有事件监听并触发了相应的处理程序。

这个流程体现了对文件描述符上下文的安全访问和操作，包括使用锁来保护数据结构、检查和修改文件描述符的事件状态，
以及通过epoll_ctl与epoll交互来更新I/O事件的监听状态。
 */
bool IOManager::cancelAll(int fd) {
    // 使用读锁安全地访问文件描述符上下文数组
    RWMutexType::ReadLock lock(m_mutex);
    // 如果文件描述符超出当前管理的范围，则直接返回false
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    // 获取指定文件描述符的上下文
    FdContext* fd_ctx = m_fdContexts[fd];
    // 释放读锁
    lock.unlock();

    // 对文件描述符上下文加锁，保证线程安全
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 如果该文件描述符上没有注册任何事件，则返回false
    if(!fd_ctx->events) {
        return false;
    }

    // 准备删除操作，将该文件描述符上的所有事件从epoll监听中移除
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0; // 事件类型设置为0，因为是删除操作
    epevent.data.ptr = fd_ctx; // 关联文件描述符上下文

    // 调用epoll_ctl执行删除操作
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    // 如果操作失败，记录错误信息并返回false
    if(rt) {
        WEBSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 如果之前监听了读事件，触发读事件的处理程序
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount; // 减少待处理的事件计数
    }
    // 如果之前监听了写事件，触发写事件的处理程序
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount; // 减少待处理的事件计数
    }

    // 断言：在触发所有事件的处理程序后，该文件描述符上不应再有任何事件被监听
    WEBSERVER_ASSERT(fd_ctx->events == 0);
    return true; // 成功取消所有事件监听并触发处理程序，返回true
}


// 获取当前线程的 IOManager 实例。
/*
这个函数 IOManager::GetThis() 属于 IOManager 类，它的目的是获取当前线程的 IOManager 实例。
这是通过从当前线程的 Scheduler 基类实例进行向下转型（dynamic_cast）来实现的。

简单来说，这个函数尝试将当前线程的 Scheduler 实例转换为 IOManager 实例。
如果当前线程的 Scheduler 实际上是一个 IOManager 实例，这个转换就会成功，函数将返回一个指向该实例的指针。
如果不是，函数将返回 nullptr。这种设计允许在不完全了解当前线程 Scheduler 具体类型的情况下，安全地尝试获取 IOManager 实例。

这个函数的工作流程如下：

函数调用：
当代码中某处调用 IOManager::GetThis() 时，工作流程开始。这通常发生在需要获取当前线程的 IOManager 实例的场景中。

获取当前 Scheduler：
函数内部首先调用 Scheduler::GetThis()。这个静态成员函数的作用是返回与当前执行线程关联的 Scheduler 实例。
Scheduler 类是一个基类，用于调度和管理线程或任务的执行。

类型转换尝试：
得到 Scheduler 实例后，使用 dynamic_cast 尝试将这个实例向下转换为 IOManager 类型的指针。
dynamic_cast 是 C++ 中用于运行时类型检查的一个操作符，它确保了类型转换的安全性。
如果当前 Scheduler 实例实际上是一个 IOManager 实例（或其派生类的实例），这个转换就会成功。

转换结果：
成功：
如果转换成功，dynamic_cast 将返回指向 IOManager 实例的指针。
这意味着当前线程的 Scheduler 实际上是一个 IOManager 实例，函数随后返回这个 IOManager 指针。
失败：
如果转换失败，这通常意味着当前线程的 Scheduler 实例不是 IOManager 类型（可能是 Scheduler 类的另一个派生类）。
在这种情况下，dynamic_cast 将返回 nullptr，表示转换不成功，函数也将返回 nullptr。

函数返回：
函数返回 IOManager 类型的指针或者 nullptr，取决于类型转换尝试的结果。
调用者可以根据返回值判断当前线程是否有关联的 IOManager 实例，并据此采取相应的行动。

这个工作流程提供了一种类型安全的方式来尝试获取当前线程的 IOManager 实例，
使得在不确定当前 Scheduler 具体类型的情况下，也能尝试访问 IOManager 的功能。

*/
IOManager* IOManager::GetThis() {
    // 将当前线程的 Scheduler 实例转型为 IOManager 实例并返回。
    // dynamic_cast 在运行时执行类型安全的向下转换，用于确定某个基类指针或引用是否可以安全地转换为派生类指针或引用。
    // Scheduler::GetThis() 用于获取当前线程关联的 Scheduler 实例。
    // 如果当前 Scheduler 实际上是一个 IOManager 实例，这个转换就会成功，并返回该 IOManager 指针。
    // 如果转换失败（即当前 Scheduler 不是 IOManager 实例），将返回 nullptr。
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


// tickle函数：用于唤醒IO管理器进行任务处理
/*
这个函数首先检查是否有空闲线程可用来处理新的任务。
如果有空闲线程，它会往一个预先设置的管道的写端（m_tickleFds[1]）写入字符 "T"。
这个写操作通常会触发等待在管道读端的线程开始执行，从而处理新的事件或任务。
函数中的断言 WEBSERVER_ASSERT(rt == 1); 确保写操作正常完成，确切地写入了一个字节。
如果写入的字节数不是预期的一个字节，断言将触发，可能会导致程序中断或输出错误信息。
*/
void IOManager::tickle() {
    // 检查是否存在空闲线程可以处理新任务
    if(!hasIdleThreads()) {
        // 如果没有空闲线程，直接返回不执行后续操作
        return;
    }

    // 往管道的写端写入字符"T"，以触发事件处理
    // m_tickleFds[1] 是管道写端的文件描述符，"T"是要写入的数据，1是写入的字节数
    int rt = write(m_tickleFds[1], "T", 1);

    // 确认写入操作是否成功：要求实际写入的字节数(rt)必须等于1
    // WEBSERVER_ASSERT 是一个断言宏，用于在条件不满足时提示错误或中断程序
    WEBSERVER_ASSERT(rt == 1);
}

/**
 * 检查IOManager是否应该停止运行。
 * 此函数主要用于在事件循环中判断是否所有的事件都已经处理完毕，
 * 并且没有其他等待中的定时器事件，以决定是否可以安全地停止IOManager的运行。
 *
 * 参数：
 * - timeout 用于接收下一个定时器的超时时间。如果没有定时器等待，将被设置为最大的无符号64位整数。
 * 
 * 返回值：
 *  - 返回一个布尔值，如果没有等待的定时器、没有待处理的事件，并且调度器已经停止，则返回true，表示IOManager应该停止；否则返回false。
 * 
 * 
 * 工作流程如下：
 * 
 * 1. 获取下一个定时器的超时时间： 
 * - 调用getNextTimer()函数获取距离当前时间最近的下一个定时器事件的超时时间，并将该时间赋值给传入的timeout参数。
 * - 这一步是为了确定是否有即将到来的定时器事件需要处理。
 * 
 * 2. 检查停止条件： 
 *  - 接下来，函数检查三个条件是否同时满足：
 *      2.1 timeout == ~0ull：
 *          - 检查timeout是否等于~0ull，即无符号长整型的最大值。这表示没有任何定时器事件等待处理。
 *      2.2 m_pendingEventCount == 0：
 *          - 检查成员变量m_pendingEventCount（表示待处理事件的数量）是否为0。这表示当前没有任何非定时器的事件等待处理。
 *      2.3 Scheduler::stopping()：
 *          - 调用基类Scheduler的stopping方法，检查调度器是否处于停止状态。这一步是为了确保整个调度系统是否准备好停止。
 * 3. 返回停止状态：
 *  - 如果上述三个条件都满足，则函数返回true，表示IO管理器应该停止。否则，返回false，表示IO管理器不应该停止，事件循环应该继续运行。
 * 
 * 总的来说，这个函数的作用是判断IO管理器是否应该停止运行，它通过检查是否有待处理的定时器事件、非定时器事件以及调度器的停止状态来做出决定。
 */
bool IOManager::stopping(uint64_t& timeout) {
    // 获取下一个定时器的超时时间，并将其赋值给timeout参数。
    timeout = getNextTimer();

    // 如果没有定时器需要处理（timeout == ~0ull）、没有待处理的事件（m_pendingEventCount == 0）
    // 且调度器也在停止状态（Scheduler::stopping()为true），则返回true，表示IO管理器应该停止。
    // 否则，返回false，表示IO管理器不应该停止。
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

/**
 * 检查IO管理器是否应该停止。
 * 
 * 函数名: stopping
 * 
 * 类名: IOManager
 * 
 * 功能描述: 
 *    - 检查IO管理器是否应该停止。该函数通过调用另一个重载的stopping函数，并传递一个预设的timeout值（0），
 *    - 来确定IO管理器的停止状态。这提供了一个方便的方式，用默认的超时设置来判断停止状态，无需调用者指定具体的超时时间。
 * 
 * 参数列表: 无
 * 
 * 返回值: 
 *    bool - 如果根据当前的条件和预设的超时时间（0）判断IO管理器应该停止，则返回true；否则返回false。
 * 
 * 详细介绍：
 * - 这个IOManager::stopping()函数是一个重载版本，它不接受任何参数，并且内部调用了另一个重载版本的stopping(uint64_t& timeout)函数。
 * - 以下是这个函数的工作流程：
 * 
 * 1. 初始化超时时间： 
 * - 在函数内部，首先声明一个uint64_t类型的变量timeout，并将其初始化为0。
 * - 这个变量用来接收stopping(uint64_t& timeout)函数返回的下一个定时器的超时时间。
 * 
 * 2. 调用另一个重载版本的stopping函数： 
 * - 接着，函数调用stopping(uint64_t& timeout)这个重载版本，并将刚刚声明的timeout变量作为参数传入。
 * - 这个调用将检查IO管理器是否应该停止，同时更新timeout变量的值。
 * 
 * 3. 返回停止状态： 
 * - 最后，函数返回stopping(uint64_t& timeout)的返回值，这个值表示IO管理器是否应该停止。
 * - 由于在这个重载版本中，调用者可能不关心具体的超时时间，所以timeout变量在这里主要是作为一个内部变量使用。
 * 
 * 
 * 总的来说，这个重载版本的stopping()函数提供了一种更简洁的方式来检查IO管理器是否应该停止，而不需要关心具体的超时时间。
 */
bool IOManager::stopping() {
    // 初始化超时时间为0。
    uint64_t timeout = 0;

    // 调用重载的stopping函数，检查IO管理器是否应该停止，并更新timeout变量的值。
    // 这里的timeout变量主要用于接收下一个定时器的超时时间，但在这个函数中不会被使用。
    return stopping(timeout);
}

// IOManager的idle函数，负责监听和处理IO事件和定时任务。
/**
 * 功能描述: 当IO管理器没有其他任务时，执行idle函数来等待和处理epoll事件。
 *
 * 参数: 无
 *
 * 返回值: 无
 *
 * 详细说明:
 *   - 函数首先记录进入idle状态的日志。
 *   - 定义了一个最大事件数量的常量，用于epoll_wait调用。
 *   - 使用智能指针管理epoll_event数组的生命周期，以自动处理资源回收。
 *   - 进入一个无限循环，循环体内首先检查IO管理器是否应当停止，如果是，则退出循环。
 *   - 调用epoll_wait等待事件的发生，等待时间由next_timeout确定。
 *   - 如果有事件发生，或者超时，或者因为信号中断而返回，则处理这些事件。
 *   - 检查是否有到期的回调函数，如果有，则调度它们执行。
 *   - 遍历所有发生的事件，对每个事件根据其类型和对应的文件描述符上下文进行处理。
 *     - 特别地，如果事件是tickler（用于唤醒）的读事件，则读取所有数据并忽略。
 *     - 对于其他事件，根据事件类型更新文件描述符上下文中的事件状态，并触发相应的事件处理。
 *   - 在处理完所有事件后，当前的协程让出执行权。
 * 
 * 工作流程:
 *    - 这段代码是一个用于IO管理的C++函数，它在某个服务或应用程序（如Web服务器）中管理异步IO事件。
 *    - 函数通过epoll来高效地处理大量的并发连接和IO事件。
 *    - epoll是Linux特有的IO事件通知机制，它比传统的select或poll更为高效，特别是在处理大量并发连接时。
 *    - 这段代码展示了在一个基于epoll事件循环的服务中如何高效处理IO事件和定时任务。
 *    - 通过动态调整epoll监听的事件集、处理定时器回调和利用智能指针管理资源，这个函数为高性能网络服务提供了一个稳固的基础。
 *    - 这个idle函数是一个高效处理IO事件和定时任务的复杂函数，它利用了Linux特有的epoll机制。以下是该函数的工作流程概述，按步骤解析其核心逻辑：
 * 1. 记录空闲状态日志：
 *    - 函数开始时，首先记录一条表示当前系统空闲状态的调试日志。
 * 2. 准备事件数组：
 *     - 通过动态分配一个epoll_event数组，准备用于接收epoll_wait返回的事件。利用智能指针管理这个数组，确保函数退出时自动释放资源，防止内存泄露。
 * 3. 进入主循环：
 *     - 函数通过一个无限循环开始监听IO事件和处理定时任务。这是事件驱动模型的核心，持续监听直到显式退出。
 * 4. 检查服务停止条件：
 *     - 每次循环开始时，检查是否有停止服务的条件满足。如果满足，则记录日志并退出循环。
 * 5. 调整和设置超时：
 *     - 根据当前的任务和状态，调整epoll_wait调用的超时时间，以高效地等待IO事件。
 * 6. 等待IO事件：
 *     - 使用epoll_wait等待注册的文件描述符上的IO事件。如果调用因为中断被打断，则重新进行调用。
 * 7. 处理定时任务：
 *     - 检查并执行到期的定时任务。这些任务通常是以回调函数的形式存在，它们被添加到一个向量中，然后统一执行和清除。
 * 8. 处理IO事件：
 *     - 遍历epoll_wait返回的事件。对于每个事件，根据事件类型（如读、写）和关联的文件描述符（通过epoll_event结构的data.ptr获取）进行相应的处理。
 * 9. 特殊处理tickle事件：
 *     - 如果事件源自内部通信机制（tickle），则读取并清空相关数据，不进行进一步处理。
 * 10. 错误和挂起处理：
 *     - 如果事件报告错误或挂起状态，则相应地修改事件类型以触发后续处理。
 * 11. 事件分发：
 *     - 根据事件类型，调用关联文件描述符的处理函数，执行具体的读或写操作。
 * 12. 更新epoll监听事件：
 *     - 根据处理结果，更新epoll对该文件描述符的监听事件，可能是修改监听的事件类型或是从监听列表中移除文件描述符。
 * 13. 协程调度：
 *     - 在处理完所有事件后，如果当前执行环境支持协程，切换到另一个协程，让出CPU，以允许其他任务执行。
 * 14. 循环继续：
 *     - 除非显式退出（如检测到停止条件），否则返回步骤4继续循环，等待和处理新的事件。
 * 
 * 通过以上步骤，idle函数实现了一个持续运行的事件循环，有效地处理IO事件和定时任务。
 * 该函数的设计充分利用了epoll的高效性，适用于需要处理大量并发连接和事件的网络服务。
 */
void IOManager::idle() {
    // 记录调试级别日志，表示当前进入空闲状态。
    WEBSERVER_LOG_DEBUG(g_logger) << "idle";

    // 最大事件数量，用于epoll_wait调用。
    const uint64_t MAX_EVNETS = 256;

    // 动态分配一个足够大的epoll_event数组来接收epoll_wait的输出。
    epoll_event* events = new epoll_event[MAX_EVNETS]();

    // 使用智能指针管理epoll_event数组，确保异常安全，自动释放资源。
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr; // 自定义删除器，用于数组释放。
    });

    // 无限循环，直到显式退出，处理IO事件和定时任务。
    while(true) {
        // 初始化下次epoll_wait的超时时间。
        uint64_t next_timeout = 0;

        // 检查是否满足停止条件，如果是，则记录信息并退出循环。
        if(WEBSERVER_UNLIKELY(stopping(next_timeout))) {
            WEBSERVER_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
            break;
        }

        int rt = 0; // epoll_wait的返回值。
        do {
            static const int MAX_TIMEOUT = 3000; // epoll_wait调用的最大超时时间（毫秒）。
            // 确保next_timeout不会超过MAX_TIMEOUT，除非特别指定为无限等待(~0ull)。
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            // 调用epoll_wait等待事件发生，或超时。
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            // 处理epoll_wait被信号打断的情况。
            if(rt < 0 && errno == EINTR) {
                // 信号中断，重试epoll_wait。
            } else {
                // 正常退出或其他错误，跳出循环。
                break;
            }
        } while(true);

        // 处理所有到期的定时任务。
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs); // 收集到期的定时器回调。
        if(!cbs.empty()) {
            // 如果有到期的定时任务，则依次执行它们。
            schedule(cbs.begin(), cbs.end());
            cbs.clear(); // 清空回调列表以备下次使用。
        }

        // 遍历epoll_wait报告的所有事件。
        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            // 检查是否为内部唤醒操作（tickle）的事件。
            if(event.data.fd == m_tickleFds[0]) {
                // 清空tickle fd的读缓冲区。
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue; // 处理下一个事件。
            }

            // 从事件数据中获取文件描述符上下文。
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            // 为fd_ctx上的操作加锁。
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            // 如果事件报告错误或挂断，自动将其视为读写事件。
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE; // 实际处理的事件类型。
            // 判断事件类型并设置相应的标志。
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            // 如果没有与fd_ctx的事件匹配，继续下一个事件。
            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            // 根据实际处理的事件更新epoll的监听事件。
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            // 更新epoll监听设置。
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                // 如果epoll_ctl失败，记录错误日志。
                WEBSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue; // 处理下一个事件。
            }

            // 触发对应的事件处理函数。
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount; // 减少待处理事件计数。
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        // 如果支持协程，切换出当前协程，让出CPU控制权。
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get(); // 获取原始指针。
        cur.reset(); // 释放智能指针的控制权。

        raw_ptr->swapOut(); // 切换协程。
    }
}




// IOManager类中的一个成员方法，用于响应计时器插入到队列或列表最前端的情况
/*
*/
/**
 * 函数名: onTimerInsertedAtFront
 * 
 * 类名: IOManager
 * 
 * 功能描述: 
 *    - 当一个定时器事件被插入到事件处理队列的最前端时调用。这通常意味着需要立即对该事件做出响应。
 *    - 该函数通过调用`tickle`方法来实现这一功能，`tickle`方法的具体实现依赖于IOManager类的详细设计，
 *    - 但它可能用于通知事件循环系统立即处理新加入的定时器事件。
 * 
 * 参数列表: 无
 * 
 * 返回值: 无 (void)
 * 
 * 工作流程：
 * 
 * 这个函数onTimerInsertedAtFront是IOManager类的一个成员方法。
 * 它的目的是在某个计时器被插入到管理的计时器队列或列表的最前端时进行响应。这通常意味着需要对事件处理或者计时管理逻辑进行即时的更新或通知。
 * 在这种情况下，该方法通过调用tickle方法来实现这一目的。
 * tickle方法的具体功能没有在这段代码中展示，但是通常它可能用于唤醒或者通知事件循环系统有新的事件或计时器需要处理。
 * onTimerInsertedAtFront函数在IOManager类中可能的工作流程。这个流程将围绕计时器的插入和事件处理展开。
 * 
 * onTimerInsertedAtFront工作流程
 * 1. 触发条件：
 * 这个函数被设计为在一个新的计时器对象被插入到IOManager管理的计时器队列或列表的最前端时被调用。
 * 通常，这意味着新的计时器有一个在当前所有计时器中最早的触发时间。
 * 
 * 2. 函数调用：
 * 当新的计时器插入到最前端，onTimerInsertedAtFront方法被自动触发。
 * 这通常是由于某种事件或操作，比如用户动作、程序逻辑判断或其他计时器事件的完成，导致新计时器的添加。
 * 
 * 3. 执行tickle方法：
 * onTimerInsertedAtFront方法的主要任务是调用IOManager类中的另一个方法tickle。
 * 这个调用是这个函数唯一的操作，表明onTimerInsertedAtFront的目的主要是触发另一层级的处理逻辑。
 * 
 * 4. tickle方法的作用：
 * 尽管onTimerInsertedAtFront方法本身并未展示tickle方法的实现，我们可以合理推测，
 * tickle方法的责任可能包括唤醒或通知事件处理循环（如果它当前处于等待状态），以便重新评估和调整待处理事件和计时器的优先级或触发时间。
 * 这是因为新的最早计时器可能需要更快的响应。
 * 
 * 5. 后续操作：
 * tickle方法执行后，IOManager可能会进行一系列的内部调整，比如更新内部数据结构来反映新的计时器队列，
 * 调整内部等待和通知机制，以确保新加入的计时器能够及时且准确地被处理。
 * 
 * 6. 结束：
 * 一旦tickle方法完成其任务，onTimerInsertedAtFront方法的执行也随之完成，等待下一个计时器事件或其他相关操作的触发。
 * 
 * - 总结
 * 在没有更多上下文信息的情况下，上述流程是基于一般性假设的。
 * onTimerInsertedAtFront方法的设计初衷是作为一个响应机制，用于处理新计时器插入到队列最前端的情况，
 * 并通过调用tickle方法来确保IOManager能够及时适应这一变化。此过程确保了IOManager可以有效管理其计时器队列，保证计时器事件的准确性和及时性。
 * 
 * 
 */
void IOManager::onTimerInsertedAtFront() {
    // 调用tickle方法来处理或响应新计时器的插入
    tickle();
}

}

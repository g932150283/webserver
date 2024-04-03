#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"

webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");
namespace webserver {

// 静态变量，用于管理 TCP 连接超时的配置项
static webserver::ConfigVar<int>::ptr g_tcp_connect_timeout =
    // 查找名为 "tcp.connect.timeout" 的配置项，如果找不到，则默认值为 5000 毫秒，描述为 "tcp connect timeout"
    webserver::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

// 线程局部变量，用于标识当前线程是否启用了 hook 功能，默认为 false
// 定义线程局部变量，来控制是否开启hook
static thread_local bool t_hook_enable = false;

// 获取接口原始地址
// 使用宏来封装对每个原始接口地址的获取。
// 将hook_init()封装到一个结构体的构造函数中，并创建静态对象，能够在main函数运行之前就能将地址保存到函数指针变量当中。
// 定义了一个宏 HOOK_FUN，用于列举需要 hook 的函数名
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)
// hook 初始化函数
void hook_init() {
    // 静态变量，用于标识是否已经进行了初始化
    static bool is_inited = false;
    // 如果已经初始化过，则直接返回，避免重复初始化
    if(is_inited) {
        return;
    }
    // 使用宏定义展开一系列的函数指针初始化操作
    // 这里使用了 dlsym 函数来动态获取标准库中的函数指针
    // dlsym - 从一个动态链接库或者可执行文件中获取到符号地址。成功返回跟name关联的地址
    // RTLD_NEXT 返回第一个匹配到的 "name" 的函数地址
    // 取出原函数，赋值给新函数
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    // 调用宏 HOOK_FUN，为每个函数名生成相应的初始化代码
    HOOK_FUN(XX);
#undef XX
}

/*
宏展开如下
void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }
    
	sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
    usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep");
    ...
    setsocketopt_f = (setsocketopt_fun)dlsym(RTLD_NEXT, "setsocketopt");
}
*/

// 静态变量，用于保存连接超时时间，默认值为 -1
static uint64_t s_connect_timeout = -1;

// 结构体 _HookIniter 的定义
struct _HookIniter {
    // 构造函数，在程序初始化时被调用
    _HookIniter() {
        // 调用 hook_init() 函数，用于初始化 hook 相关的操作
        hook_init();
        
        // 将 g_tcp_connect_timeout 的值赋给 s_connect_timeout
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        // 为 g_tcp_connect_timeout 注册一个监听器，用于在配置项变化时更新 s_connect_timeout
        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                // 使用日志记录器输出配置项变化的信息
                WEBSERVER_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                // 更新 s_connect_timeout 的值为新值
                s_connect_timeout = new_value;
        });
    }
};

// 定义静态变量 s_hook_initer，类型为 _HookIniter，该变量在程序初始化时会被构造
static _HookIniter s_hook_initer;

// 获取是否hook
bool is_hook_enable() {
    return t_hook_enable;
}
// 设置是否hook
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}


// 定时器超时条件
struct timer_info {
    int cancelled = 0;
};


/*
这段代码实现了一个模板函数 do_io，用于执行 I/O 操作。具体功能如下：

如果 hook 没有启用，则直接调用原始函数执行 I/O 操作。
如果启用了 hook，则先获取文件描述符上下文信息。
如果文件描述符上下文不存在，则直接调用原始函数执行 I/O 操作。
如果文件描述符已关闭，则设置错误码并返回 -1。
如果文件描述符不是套接字或用户设置了非阻塞标志，则直接调用原始函数执行 I/O 操作。
如果需要阻塞等待，并且设置了超时时间，则创建定时器以实现超时机制。
执行原始函数进行 I/O 操作，处理返回值和错误码。
如果发生 EAGAIN 错误（表示暂时无法完成 I/O 操作），则使用 IO 管理器添加事件并挂起当前协程，等待事件完成或超时。
如果超时或被取消，则设置错误码并返回 -1。

在这个函数中，我们首先检查是否启用了钩子功能，如果未启用，则直接调用原始的I/O函数。
接下来，函数尝试获取文件描述符的上下文信息。
    如果文件描述符已经关闭，或者它不是一个套接字，或者用户已经设置了非阻塞模式，那么函数也会直接调用原始的I/O函数。
    否则，函数会根据指定的超时设置等待I/O操作完成，如果需要，会通过协程挂起和恢复来异步等待操作完成，或者在遇到超时情况下取消操作。

这段代码使用了模板和可变参数，可以适用于不同类型的IO操作，能够以写同步的方式实现异步的效果。
该函数的主要思想如下：

1. 先进行一系列判断，是否按原函数执行。
2. 执行原始函数进行操作，若errno = EINTR，则为系统中断，应该不断重新尝试操作。
3. 若errno = EAGIN，系统已经隐式的将socket设置为非阻塞模式，此时资源暂不可用。
4. 若设置了超时时间，则设置一个执行周期为超时时间的条件定时器，它保证若在超时之前数据就已经来了，
然后操作完do_io执行完毕，智能指针tinfo已经销毁了，但是定时器还在，此时弱指针拿到的就是个空指针，将不会执行定时器的回调函数。
在条件定时器的回调函数中设置错误为ETIMEDOUT超时，并且使用cancelEvent强制执行该任务，继续回到该协程执行。
5. 通过addEvent添加事件，若添加事件失败，则将条件定时器删除并返回错误。成功则让出协程执行权。
6. 只有两种情况协程会被拉起： a. 超时了，通过定时器回调函数 cancelEvent ---> triggerEvent会唤醒回来 b. addEvent数据回来了会唤醒回来
7. 将定时器取消，若为超时则返回-1并设置errno = ETIMEDOUT，并返回-1。
8. 若为数据来了则retry，重新操作。

*/
// do_io: 执行I/O操作的通用函数模板，可以处理钩子、非阻塞和超时。
// 参数:
// fd - 文件描述符
// fun - 原始的I/O函数，例如read或write
// hook_fun_name - 钩子函数的名称，用于日志或调试
// event - 事件类型，用于非阻塞操作的事件监听
// timeout_so - 超时设置的选项
// args - 原始I/O函数需要的参数， 可变参数
// 返回值:
// ssize_t - 成功时返回操作的字节数，失败时返回-1，并设置errno
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
    // 如果钩子未启用，直接调用原始函数
    if (!webserver::t_hook_enable) {
        /* 可以将传入的可变参数args以原始类型的方式传递给函数fun。
         * 这样做的好处是可以避免不必要的类型转换和拷贝，提高代码的效率和性能。*/
        return fun(fd, std::forward<Args>(args)...);
    }

    // 尝试从文件描述符管理器获取文件描述符的上下文
    // 获取fd对应的FdCtx
    webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(fd);
    // 如果上下文不存在，直接调用原始函数
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 如果文件描述符已关闭，设置错误码并返回
    // 看看句柄是否关闭
    if (ctx->isClose()) {
        // 坏文件描述符
        errno = EBADF;
        return -1;
    }

    // 如果文件描述符不是套接字或设置为非阻塞模式，直接调用原始函数
    // 不是socket 或 用户设置了非阻塞
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // ------ hook要做了 ------异步IO
    // 获得超时时间
    // 获取根据timeout_so指定的超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    // 设置超时条件
    // 创建一个新的timer_info结构，用于超时处理
    std::shared_ptr<timer_info> tinfo(new timer_info);

    // 重试标签，用于在某些情况下重复尝试I/O操作
retry:
    // 先执行fun 读数据或写数据 若函数返回值有效就直接返回
    // 调用原始的I/O函数，传入所有参数
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    // 如果遇到中断错误，重新尝试调用
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 如果遇到EAGAIN错误，表示操作需要阻塞等待
    // 若为阻塞状态
    if (n == -1 && errno == EAGAIN) {
        // 获取当前IO管理器实例
        webserver::IOManager* iom = webserver::IOManager::GetThis();
        // 初始化定时器指针为空
        webserver::Timer::ptr timer;
        // 创建一个弱引用指向timer_info，用于定时器回调
        // tinfo的弱指针，可以判断tinfo是否已经销毁
        std::weak_ptr<timer_info> winfo(tinfo);

        // 如果设置了超时，创建一个定时器
        if (to != (uint64_t)-1) {
            /*	添加条件定时器
             *	to时间消息还没来就触发callback */ 
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                /* tinfo失效 || 设了错误
                 * 定时器失效了 */
                if (!t || t->cancelled) {
                    return;
                }
                // 没错误的话设置为超时而失败
                t->cancelled = ETIMEDOUT;
                // 取消事件强制唤醒
                iom->cancelEvent(fd, (webserver::IOManager::Event)(event));
            }, winfo);
        }
        /* addEvent error:-1 acc:0  
         * cb为空， 任务为执行当前协程 */
        // 尝试添加事件，并使当前协程等待或超时
        int rt = iom->addEvent(fd, (webserver::IOManager::Event)(event));
        // addEvent失败， 取消上面加的定时器
        if (rt) {
            // 如果添加事件失败，记录错误并取消定时器（如果已设置）
            WEBSERVER_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {
            /*	addEvent成功，把执行时间让出来
             *	只有两种情况会从这回来：
             * 	1) 超时了， timer cancelEvent triggerEvent会唤醒回来
             * 	2) addEvent数据回来了会唤醒回来 */

            // 挂起当前协程，等待I/O事件完成或超时
            webserver::Fiber::YieldToHold();
            // 回来了还有定时器就取消掉
            if (timer) {
                // 如果存在定时器，取消它
                timer->cancel();
            }
            // 从定时任务唤醒，超时失败
            // 如果因为超时而被取消，设置错误码并返回
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            // 数据来了就直接重新去操作
            // 如果未因超时被取消，重试I/O操作
            goto retry;
        }
    }
    
    // 返回原始函数的调用结果
    return n;
}




// 使用 extern "C" 声明以下代码块中的函数为 C 函数
extern "C" {
    // 定义宏 HOOK_FUN，用于列举需要 hook 的函数名，并初始化对应的函数指针为 nullptr
#define XX(name) name ## _fun name ## _f = nullptr;
    // 调用宏 HOOK_FUN，为每个函数名生成对应的函数指针声明和初始化代码
    HOOK_FUN(XX);
#undef XX  // 取消宏定义
/*
宏展开如下
extern "C" {
	sleep_fun sleep_f = nullptr;
    usleep_fun usleep_f = nullptr;
	.....
	setsocketopt_fun setsocket_f = nullptr;
}
*/

// 睡眠系列
/*
设置一个定时器然后让出执行权，超时后继续执行该协程。

回调函数使用std::bind函数将webserver::IOManager::schedule函数绑定到iom对象上，并传入fiber和-1两个参数。
由于schedule是个模板类，如果直接与函数绑定，就无法确定函数的类型，从而无法使用std::bind函数。
因此，需要先声明函数指针，将函数的类型确定下来，然后再将函数指针与std::bind函数进行绑定。
*/
// 重定义标准库中的 sleep 函数，以实现 hook
unsigned int sleep(unsigned int seconds) {
    // 如果未启用 hook，则直接调用原始的 sleep 函数
    if (!webserver::t_hook_enable) {
        return sleep_f(seconds);
    }

    // 获取当前协程
    webserver::Fiber::ptr fiber = webserver::Fiber::GetThis();
    // 获取当前 IO 管理器
    webserver::IOManager* iom = webserver::IOManager::GetThis();
    // 添加一个定时器任务，延迟执行当前协程（以毫秒为单位）
    /**
     * 	@details
     *
     *	(void(webserver::Scheduler::*)(webserver::Fiber::ptr, int thread)) 是一个函数指针类型，
     *	它定义了一个指向 webserver::Scheduler 类中一个参数为 webserver::Fiber::ptr 和 int 类型的成员函数的指针类型。
     *	具体来说，它的含义如下：
     *	void 表示该成员函数的返回值类型，这里是 void 类型。
     *	(webserver::Scheduler::*) 表示这是一个 webserver::Scheduler 类的成员函数指针类型。
     *	(webserver::Fiber::ptr, int thread) 表示该成员函数的参数列表
     *       ，其中第一个参数为 webserver::Fiber::ptr 类型，第二个参数为 int 类型。
     *	
     *	使用 std::bind 绑定了 webserver::IOManager::schedule 函数，
     * 	并将 iom 实例作为第一个参数传递给了 std::bind 函数，将webserver::IOManager::schedule函数绑定到iom对象上。
     * 	在这里，第二个参数使用了函数指针类型 (void(webserver::Scheduler::*)(webserver::Fiber::ptr, int thread))
     * 	，表示要绑定的函数类型是 webserver::Scheduler 类中一个参数为 webserver::Fiber::ptr 和 int 类型的成员函数
     * 	，这样 std::bind 就可以根据这个函数类型来实例化出一个特定的函数对象，并将 fiber 和 -1 作为参数传递给它。
     */
    iom->addTimer(seconds * 1000, std::bind(
        (void (webserver::Scheduler::*)
        (webserver::Fiber::ptr, int thread)) &webserver::IOManager::schedule,
        iom, fiber, -1));
    // 让出当前协程的执行权，切换到 IO 管理器中运行的其他协程
    webserver::Fiber::YieldToHold();
    // 返回 0，表示 sleep 函数执行完成
    return 0;
}


// 重定义标准库中的 usleep 函数，以实现 hook
int usleep(useconds_t usec) {
    // 如果未启用 hook，则直接调用原始的 usleep 函数
    if (!webserver::t_hook_enable) {
        return usleep_f(usec);
    }

    // 获取当前协程
    webserver::Fiber::ptr fiber = webserver::Fiber::GetThis();
    // 获取当前 IO 管理器
    webserver::IOManager* iom = webserver::IOManager::GetThis();
    // 添加一个定时器任务，延迟执行当前协程（以毫秒为单位）
    iom->addTimer(usec / 1000, std::bind(
        (void (webserver::Scheduler::*)
        (webserver::Fiber::ptr, int thread)) &webserver::IOManager::schedule,
        iom, fiber, -1));
    // 让出当前协程的执行权，切换到 IO 管理器中运行的其他协程
    webserver::Fiber::YieldToHold();
    // 返回 0，表示 usleep 函数执行完成
    return 0;
}


// 重定义标准库中的 nanosleep 函数，以实现 hook
int nanosleep(const struct timespec *req, struct timespec *rem) {
    // 如果未启用 hook，则直接调用原始的 nanosleep 函数
    if (!webserver::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    // 计算超时时间（毫秒为单位）
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    // 获取当前协程
    webserver::Fiber::ptr fiber = webserver::Fiber::GetThis();
    // 获取当前 IO 管理器
    webserver::IOManager* iom = webserver::IOManager::GetThis();
    // 添加一个定时器任务，延迟执行当前协程
    iom->addTimer(timeout_ms, std::bind(
        (void (webserver::Scheduler::*)
        (webserver::Fiber::ptr, int thread)) &webserver::IOManager::schedule,
        iom, fiber, -1));
    // 让出当前协程的执行权，切换到 IO 管理器中运行的其他协程
    webserver::Fiber::YieldToHold();
    // 返回 0，表示 nanosleep 函数执行完成
    return 0;
}

// 创建socket
// 重定义标准库中的 socket 函数，以实现 hook
int socket(int domain, int type, int protocol) {
    // 如果未启用 hook，则直接调用原始的 socket 函数
    if (!webserver::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }

    // 调用原始的 socket 函数创建套接字
    int fd = socket_f(domain, type, protocol);
    // 如果创建套接字失败，则直接返回错误码
    if (fd == -1) {
        return fd;
    }
    
    // 将文件描述符注册到文件描述符管理器中
    webserver::FdMgr::GetInstance()->get(fd, true);
    // 返回创建的文件描述符
    return fd;
}

// socket连接
// 带有超时功能的连接函数
int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    // 如果未启用 hook，则直接调用原始的 connect 函数
    if (!webserver::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }

    // 获取文件描述符上下文对象
    webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(fd);
    // 如果文件描述符上下文对象不存在或已关闭，则返回错误
    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    // 如果文件描述符不是套接字，则直接调用原始的 connect 函数
    if (!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    // 如果文件描述符设置了非阻塞模式，则直接调用原始的 connect 函数
    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    // ----- 异步开始 -----
    // 先尝试连接
    // 调用原始的 connect 函数进行连接
    int n = connect_f(fd, addr, addrlen);
    // 如果连接成功或者连接已建立，则返回 0
    if (n == 0) {
        return 0;
    }
    // 其他错误，EINPROGRESS表示连接操作正在进行中
    // 如果连接失败，并且错误码不是 EINPROGRESS，则直接返回错误码
    else if (n != -1 || errno != EINPROGRESS) {
        return n;
    }

    // 获取当前 IO 管理器
    webserver::IOManager* iom = webserver::IOManager::GetThis();
    // 创建一个定时器对象
    webserver::Timer::ptr timer;
    // 创建一个共享指针用于存储定时器信息
    std::shared_ptr<timer_info> tinfo(new timer_info);
    // 使用弱引用保存定时器信息的共享指针
    std::weak_ptr<timer_info> winfo(tinfo);

    // 如果设置了连接超时时间，则创建一个条件定时器
    if (timeout_ms != (uint64_t)-1) {
        // 加条件定时器
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            // 取消事件监听
            iom->cancelEvent(fd, webserver::IOManager::WRITE);
        }, winfo);
    }

    // 将文件描述符添加到 IO 管理器的事件监听中
    // 添加一个写事件
    int rt = iom->addEvent(fd, webserver::IOManager::WRITE);
    // 如果添加事件失败，则打印错误日志
    if (rt == 0) {
        /* 	只有两种情况唤醒：
         * 	1. 超时，从定时器唤醒
         *	2. 连接成功，从epoll_wait拿到事件 */
        // 让出当前协程的执行权，切换到 IO 管理器中运行的其他协程
        webserver::Fiber::YieldToHold();
        // 如果定时器存在，则取消定时器
        if (timer) {
            timer->cancel();
        }
        // 从定时器唤醒，超时失败
        // 如果定时器被取消，则返回相应的错误码
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        // 如果添加事件失败，则取消定时器，并打印错误日志
        if (timer) {
            timer->cancel();
        }
        WEBSERVER_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    // 获取套接字的错误状态
    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    // 如果获取到的错误状态为 0，则表示连接成功，返回 0
    if (!error) {
        return 0;
    } else {
        // 如果获取到的错误状态不为 0，则将错误状态设置为 errno，并返回 -1
        errno = error;
        return -1;
    }
}


// 重新定义标准库中的 connect 函数
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // 调用带有超时功能的连接函数 connect_with_timeout，并传递默认的连接超时时间
    return connect_with_timeout(sockfd, addr, addrlen, webserver::s_connect_timeout);
}


// 接收请求
// 封装了 accept 函数，实现了非阻塞 accept 操作
int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    // 调用 do_io 函数进行非阻塞 accept 操作
    // 参数解释：
    // s：套接字描述符
    // accept_f：accept 函数指针，用于执行 accept 操作
    // "accept"：操作名称，用于日志输出
    // webserver::IOManager::READ：事件类型，表示读事件
    // SO_RCVTIMEO：套接字选项，用于设置接收超时时间
    // addr：用于保存客户端地址信息的结构体指针
    // addrlen：地址信息结构体的长度指针
    int fd = do_io(s, accept_f, "accept", webserver::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    // 将新创建的连接放到文件管理中
    // 如果 accept 操作成功，则将新建的文件描述符添加到文件描述符管理器中
    if (fd >= 0) {
        webserver::FdMgr::GetInstance()->get(fd, true);
    }
    
    // 返回 accept 操作结果（文件描述符）
    return fd;
}


// 封装了 read 函数，实现了非阻塞的读操作
ssize_t read(int fd, void *buf, size_t count) {
    // 调用 do_io 函数进行非阻塞的读操作
    // 参数解释：
    // fd：文件描述符
    // read_f：read 函数指针，用于执行读操作
    // "read"：操作名称，用于日志输出
    // webserver::IOManager::READ：事件类型，表示读事件
    // SO_RCVTIMEO：套接字选项，用于设置接收超时时间
    // buf：用于存储读取数据的缓冲区指针
    // count：读取数据的字节数
    return do_io(fd, read_f, "read", webserver::IOManager::READ, SO_RCVTIMEO, buf, count);
}

// 封装了 readv 函数，实现了非阻塞的读操作
ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    // 调用 do_io 函数进行非阻塞的读操作
    // 参数解释：
    // fd：文件描述符
    // readv_f：readv 函数指针，用于执行读操作
    // "readv"：操作名称，用于日志输出
    // webserver::IOManager::READ：事件类型，表示读事件
    // SO_RCVTIMEO：套接字选项，用于设置接收超时时间
    // iov：用于存储读取数据的缓冲区结构体指针
    // iovcnt：缓冲区结构体的数量
    return do_io(fd, readv_f, "readv", webserver::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}


// 封装了 recv 函数，实现了非阻塞的接收操作
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    // 调用 do_io 函数进行非阻塞的接收操作
    // 参数解释：
    // sockfd：套接字描述符
    // recv_f：recv 函数指针，用于执行接收操作
    // "recv"：操作名称，用于日志输出
    // webserver::IOManager::READ：事件类型，表示读事件
    // SO_RCVTIMEO：套接字选项，用于设置接收超时时间
    // buf：用于存储接收数据的缓冲区指针
    // len：接收数据的最大长度
    // flags：接收标志
    return do_io(sockfd, recv_f, "recv", webserver::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

// 封装了 recvfrom 函数，实现了非阻塞的接收操作
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    // 调用 do_io 函数进行非阻塞的接收操作
    // 参数解释：
    // sockfd：套接字描述符
    // recvfrom_f：recvfrom 函数指针，用于执行接收操作
    // "recvfrom"：操作名称，用于日志输出
    // webserver::IOManager::READ：事件类型，表示读事件
    // SO_RCVTIMEO：套接字选项，用于设置接收超时时间
    // buf：用于存储接收数据的缓冲区指针
    // len：接收数据的最大长度
    // flags：接收标志
    // src_addr：用于保存发送端地址信息的结构体指针
    // addrlen：地址信息结构体的长度指针
    return do_io(sockfd, recvfrom_f, "recvfrom", webserver::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

// 封装了 recvmsg 函数，实现了非阻塞的接收操作
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    // 调用 do_io 函数进行非阻塞的接收操作
    // 参数解释：
    // sockfd：套接字描述符
    // recvmsg_f：recvmsg 函数指针，用于执行接收操作
    // "recvmsg"：操作名称，用于日志输出
    // webserver::IOManager::READ：事件类型，表示读事件
    // SO_RCVTIMEO：套接字选项，用于设置接收超时时间
    // msg：用于存储接收数据的消息头结构体指针
    // flags：接收标志
    return do_io(sockfd, recvmsg_f, "recvmsg", webserver::IOManager::READ, SO_RCVTIMEO, msg, flags);
}


// 封装了 write 函数，实现了非阻塞的写操作
ssize_t write(int fd, const void *buf, size_t count) {
    // 调用 do_io 函数进行非阻塞的写操作
    // 参数解释：
    // fd：文件描述符
    // write_f：write 函数指针，用于执行写操作
    // "write"：操作名称，用于日志输出
    // webserver::IOManager::WRITE：事件类型，表示写事件
    // SO_SNDTIMEO：套接字选项，用于设置发送超时时间
    // buf：待发送数据的缓冲区指针
    // count：待发送数据的字节数
    return do_io(fd, write_f, "write", webserver::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

// 封装了 writev 函数，实现了非阻塞的写操作
ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    // 调用 do_io 函数进行非阻塞的写操作
    // 参数解释：
    // fd：文件描述符
    // writev_f：writev 函数指针，用于执行写操作
    // "writev"：操作名称，用于日志输出
    // webserver::IOManager::WRITE：事件类型，表示写事件
    // SO_SNDTIMEO：套接字选项，用于设置发送超时时间
    // iov：待发送数据的缓冲区结构体指针
    // iovcnt：缓冲区结构体的数量
    return do_io(fd, writev_f, "writev", webserver::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

// 封装了 send 函数，实现了非阻塞的发送操作
ssize_t send(int s, const void *msg, size_t len, int flags) {
    // 调用 do_io 函数进行非阻塞的发送操作
    // 参数解释：
    // s：套接字描述符
    // send_f：send 函数指针，用于执行发送操作
    // "send"：操作名称，用于日志输出
    // webserver::IOManager::WRITE：事件类型，表示写事件
    // SO_SNDTIMEO：套接字选项，用于设置发送超时时间
    // msg：待发送数据的缓冲区指针
    // len：待发送数据的字节数
    // flags：发送标志
    return do_io(s, send_f, "send", webserver::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

// 封装了 sendto 函数，实现了非阻塞的发送操作
ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    // 调用 do_io 函数进行非阻塞的发送操作
    // 参数解释：
    // s：套接字描述符
    // sendto_f：sendto 函数指针，用于执行发送操作
    // "sendto"：操作名称，用于日志输出
    // webserver::IOManager::WRITE：事件类型，表示写事件
    // SO_SNDTIMEO：套接字选项，用于设置发送超时时间
    // msg：待发送数据的缓冲区指针
    // len：待发送数据的字节数
    // flags：发送标志
    // to：目标地址信息的结构体指针
    // tolen：目标地址信息结构体的长度
    return do_io(s, sendto_f, "sendto", webserver::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

// 封装了 sendmsg 函数，实现了非阻塞的发送操作
ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    // 调用 do_io 函数进行非阻塞的发送操作
    // 参数解释：
    // s：套接字描述符
    // sendmsg_f：sendmsg 函数指针，用于执行发送操作
    // "sendmsg"：操作名称，用于日志输出
    // webserver::IOManager::WRITE：事件类型，表示写事件
    // SO_SNDTIMEO：套接字选项，用于设置发送超时时间
    // msg：待发送数据的消息头结构体指针
    // flags：发送标志
    return do_io(s, sendmsg_f, "sendmsg", webserver::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

// 关闭socket
// 封装了 close 函数，根据是否启用 Hook 进行不同的处理
int close(int fd) {
    // 如果未启用 Hook，则直接调用原始的 close 函数
    if (!webserver::t_hook_enable) {
        return close_f(fd);
    }

    // 获取文件描述符的上下文信息
    webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(fd);

    // 如果文件描述符上下文信息存在
    if (ctx) {
        // 获取当前的 IO 管理器实例
        auto iom = webserver::IOManager::GetThis();

        // 如果 IO 管理器实例存在
        if (iom) {
            // 取消所有与该文件描述符相关的事件
            iom->cancelAll(fd);
        }

        // 从文件描述符管理器中删除该文件描述符的上下文信息
        webserver::FdMgr::GetInstance()->del(fd);
    }

    // 调用原始的 close 函数关闭文件描述符
    return close_f(fd);
}

// 修改文件状态, 对用户反馈是否是用户设置了非阻塞模式
// 封装了 fcntl 函数，用于对文件描述符进行属性控制
int fcntl(int fd, int cmd, ... /* arg */ ) {
    // 定义可变参数列表
    va_list va;
    // 开始可变参数列表
    va_start(va, cmd);
    // 根据命令类型进行不同的处理
    switch(cmd) {
        // 设置文件状态标志
        case F_SETFL:
            {
                // 获取参数
                int arg = va_arg(va, int);
                // 结束可变参数列表
                va_end(va);
                // 获取文件描述符的上下文信息
                webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(fd);
                // 如果文件描述符上下文信息不存在、已关闭或不是套接字，则调用原始的 fcntl 函数
                if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                // 设置用户态的非阻塞标志
                ctx->setUserNonblock(arg & O_NONBLOCK);
                // 如果系统态的非阻塞标志已设置，则将参数中的 O_NONBLOCK 加入标志中，否则将其从标志中移除
                if (ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                // 调用原始的 fcntl 函数进行设置并返回结果
                return fcntl_f(fd, cmd, arg);
            }
            break;
        // 获取文件状态标志
        case F_GETFL:
            {
                // 结束可变参数列表
                va_end(va);
                // 调用原始的 fcntl 函数获取文件状态标志
                int arg = fcntl_f(fd, cmd);
                // 获取文件描述符的上下文信息
                webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(fd);
                // 如果文件描述符上下文信息不存在、已关闭或不是套接字，则直接返回获取的结果
                if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                // 如果用户态的非阻塞标志已设置，则在结果中加入 O_NONBLOCK，否则从结果中移除 O_NONBLOCK
                if (ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        // 其他命令，直接调用原始的 fcntl 函数进行处理
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                // 获取参数
                int arg = va_arg(va, int);
                // 结束可变参数列表
                va_end(va);
                // 调用原始的 fcntl 函数进行处理并返回结果
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                // 结束可变参数列表
                va_end(va);
                // 调用原始的 fcntl 函数进行处理并返回结果
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                // 获取参数
                struct flock* arg = va_arg(va, struct flock*);
                // 结束可变参数列表
                va_end(va);
                // 调用原始的 fcntl 函数进行处理并返回结果
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                // 获取参数
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                // 结束可变参数列表
                va_end(va);
                // 调用原始的 fcntl 函数进行处理并返回结果
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            // 结束可变参数列表
            va_end(va);
            // 其他命令，直接调用原始的 fcntl 函数进行处理并返回结果
            return fcntl_f(fd, cmd);
    }
}


// 封装了 ioctl 函数，用于控制设备
// 对设备进行控制操作
/*	value为指向int类型的指针，如果该指针指向的值为0，则表示关闭非阻塞模式；如果该指针指向的值为非0，则表示打开非阻塞模式。
 *	int value = 1;
 *	ioctl(fd, FIONBIO, &value);
 * 
 */
int ioctl(int d, unsigned long int request, ...) {
    // 定义可变参数列表
    va_list va;
    // 开始可变参数列表
    va_start(va, request);
    // 获取参数
    void* arg = va_arg(va, void*);
    // 结束可变参数列表
    va_end(va);

    // 如果请求是 FIONBIO
    //	FIONBIO用于设置文件描述符的非阻塞模式
    if (FIONBIO == request) {
        // 将参数转换为布尔值，表示是否设置非阻塞
        bool user_nonblock = !!*(int*)arg;
        // 获取文件描述符的上下文信息
        webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(d);
        // 如果文件描述符上下文信息不存在、已关闭或不是套接字，则调用原始的 ioctl 函数
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        // 设置用户态的非阻塞标志
        ctx->setUserNonblock(user_nonblock);
    }

    // 调用原始的 ioctl 函数并返回结果
    return ioctl_f(d, request, arg);
}


// 封装了 getsockopt 函数，用于获取套接字选项
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    // 直接调用原始的 getsockopt 函数并返回结果
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

// 封装了 setsockopt 函数，用于设置套接字选项
// 设置socket
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    // 如果未启用 Hook，则直接调用原始的 setsockopt 函数
    if (!webserver::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    // 如果设置的选项是 SOL_SOCKET
    if (level == SOL_SOCKET) {
        // 如果选项是 SO_RCVTIMEO 或 SO_SNDTIMEO
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            // 获取文件描述符的上下文信息
            webserver::FdCtx::ptr ctx = webserver::FdMgr::GetInstance()->get(sockfd);
            // 如果文件描述符上下文信息存在，则将超时时间设置到套接字上下文中
            if (ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    // 调用原始的 setsockopt 函数并返回结果
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

/*
有了hook模块的加持，在使用IO协程调度器时，如果不想该操作导致整个线程的阻塞，
我们可以使用scheduler将该任务加入到任务队列中，这样当任务阻塞时，只会使执行该任务的协程挂起，去执行别的任务，
在消息来后或者达到超时时间继续执行该协程任务，这样就实现了异步操作。
*/

}

#include "thread.h"
#include "log.h"
#include "util.h"

namespace webserver {

/*
这里使用了thread_local关键字声明两个静态变量t_thread和t_thread_name，它们的值对每个线程都是唯一的。
这意味着每个线程都有自己的一份副本，互不干扰。
*/
// 定义线程局部变量t_thread，用于存储指向当前Thread对象的指针，默认为nullptr。
static thread_local Thread* t_thread = nullptr;

// 定义线程局部变量t_thread_name，用于存储当前线程的名称，默认为"UNKNOW"。
static thread_local std::string t_thread_name = "UNKNOW";

/*
g_logger是一个全局变量，用于记录系统级别的日志信息。这里假设WEBSERVER_LOG_NAME是一个宏或函数，用于创建或获取名为"system"的日志器实例。
*/
// 定义一个全局日志器指针g_logger，初始化为指向"system"日志器的指针。
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");


// 获取当前线程的Thread对象指针。
Thread* Thread::GetThis() {
    return t_thread;
}

// 获取当前线程的名称。
const std::string& Thread::GetName() {
    return t_thread_name;
}

// 设置当前线程的名称。
/*
SetName允许设置当前线程的名称。如果传入的名称不为空，并且当前线程对应的Thread对象存在，则同时更新该对象的名称和线程局部变量的值。
*/
void Thread::SetName(const std::string& name) {
    // 如果提供的名称为空，不做任何操作。
    if(name.empty()) {
        return;
    }
    // 如果t_thread指针非空，即存在Thread对象，更新其m_name成员变量。
    if(t_thread) {
        t_thread->m_name = name;
    }
    // 更新线程局部变量t_thread_name的值。
    t_thread_name = name;
}


/**
 * @brief 构造一个新的Thread对象。
 * 
 * @param cb 将要在新线程中执行的函数对象。
 * @param name 新线程的名称。
 * 这个构造函数通过pthread_create创建了一个新线程，并将类实例的this指针作为参数传递给Thread::run静态函数，
 * 以便在新线程中执行成员函数。
 * 构造函数中使用了信号量m_semaphore来同步，确保线程创建并初始化完成后，构造函数才继续执行。
 */
Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb) // 初始化成员变量m_cb为传入的函数对象
    ,m_name(name) { // 初始化成员变量m_name为传入的线程名称
    if(name.empty()) { // 如果传入的线程名称为空
        m_name = "UNKNOW"; // 设置默认线程名称为"UNKNOW"
    }
    // 创建一个新线程，线程开始时执行Thread::run静态函数
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) { // 如果pthread_create返回非0值，表示线程创建失败
        // 记录错误日志
        WEBSERVER_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error"); // 抛出异常
    }
    m_semaphore.wait(); // 等待新创建的线程启动并设置好其它相关属性
}


/**
 * @brief Thread对象的析构函数。
 * 析构函数负责将已创建的线程设置为分离状态（通过pthread_detach），这样一来，线程结束时将自动回收资源。
 * 这是因为在分离状态的线程结束时，其占用的资源会被操作系统自动回收，无需其他线程来回收。
 * 通过在析构函数中分离线程，可以避免在主线程退出时出现悬挂线程，从而防止内存泄漏和其他问题。
 */
Thread::~Thread() {
    if(m_thread) { // 如果成员变量m_thread不为空，即线程已被创建
        pthread_detach(m_thread); // 将线程标记为分离状态
    }
}


/**
 * @brief 等待线程完成执行。
 * 
 * 如果线程已经启动，则调用此方法会阻塞当前线程，直到被等待的线程完成执行。
 * pthread_join函数是一种阻塞机制，用于等待指定线程的终止并获取其返回值，同时负责回收线程所使用的资源。
 */
void Thread::join() {
    if(m_thread) { // 检查线程是否存在
        int rt = pthread_join(m_thread, nullptr); // 调用pthread_join等待线程结束
        if(rt) { // 如果pthread_join返回非0值，表示等待失败
            // 记录错误日志
            WEBSERVER_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error"); // 抛出异常
        }
        m_thread = 0; // 清除线程标识符，表示线程已经结束并且已经被join
    }
}


/**
 * @brief 线程的入口函数。
 * 
 * @param arg 传递给线程的参数，期望是Thread类的实例指针。
 * @return 返回值没有实际意义，总是返回0。
 * 
 * 这里的run方法是作为pthread_create的回调函数，它实际上是线程的入口点。
 * 该方法首先将传入的void*参数转换为Thread*类型，这样就可以访问Thread类的成员变量和方法。
 * 然后，它设置了一些线程局部存储变量（如线程实例指针和线程名称），
 * 并且调用pthread_setname_np为线程设置一个名称（注意，名称长度限制为15个字符）。
 * 接着，它通过信号量通知构造函数线程已经准备就绪，可以继续执行。
 * 最后，它执行了线程任务（通过std::function<void()> cb），任务完成后线程将结束。
 * 
 * 通过信号量，能够确保构造函数在创建线程之后会一直阻塞，直到run方法运行并通知信号量，构造函数才会返回。
 * 
 * 在构造函数中完成线程的启动和初始化操作，可能会导致线程还没有完全启动就被调用，从而导致一些未知的问题。
 * 因此，在出构造函数之前，确保线程先跑起来，保证能够初始化id，可以避免这种情况的发生。同时，这也可以保证线程的安全性和稳定性。
 */
void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg; // 从void*转换为Thread*，获取Thread实例
    t_thread = thread; // 设置当前线程的Thread实例指针，用于线程局部存储
    t_thread_name = thread->m_name; // 设置当前线程的名称，用于线程局部存储
    thread->m_id = webserver::GetThreadId(); // 获取当前线程的ID并保存 // 只有进了run方法才是新线程在执行，创建时是由主线程完成的，threadId为主线程的
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str()); // 设置线程名称（取最多前15个字符）

    std::function<void()> cb;
    cb.swap(thread->m_cb); // 交换线程任务，准备执行

    thread->m_semaphore.notify(); // 通知构造函数线程已经准备好，可以继续执行 // 在出构造函数之前，确保线程先跑起来，保证能够初始化id

    cb(); // 执行线程任务
    return 0; // 返回值没有实际意义，仅符合pthread_create的签名
}


}

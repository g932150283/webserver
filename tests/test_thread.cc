#include "src/webserver.h"
#include <unistd.h>
// 定义一个全局的日志记录器，用于输出日志信息
webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();
// 定义一个全局变量count，用于在多个线程中共享和操作
int count = 0;
// 注释掉的代码：定义一个读写互斥锁
// webserver::RWMutex s_mutex;
// 使用webserver库中的互斥锁
webserver::Mutex s_mutex;

// 定义函数fun1，用于线程执行的任务
void fun1() {
    // 输出当前线程的信息，包括名字和ID
    WEBSERVER_LOG_INFO(g_logger) << "name: " << webserver::Thread::GetName()
                             << " this.name: " << webserver::Thread::GetThis()->getName()
                             << " id: " << webserver::GetThreadId()
                             << " this.id: " << webserver::Thread::GetThis()->getId();
    // 注释掉的代码：线程休眠20秒
    // sleep(20);
    // 在循环中增加count变量的值
    for(int i = 0; i < 100000000; ++i) {
        // 注释掉的代码：获取写锁
        //webserver::RWMutex::WriteLock lock(s_mutex);
        // 获取互斥锁
        // webserver::Mutex::Lock lock(s_mutex);
        ++count;
    }
}
// 定义函数fun2，无限循环输出日志信息
void fun2() {
    while(true) {
        WEBSERVER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}
// 定义函数fun3，无限循环输出日志信息
void fun3() {
    while(true) {
        WEBSERVER_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    WEBSERVER_LOG_INFO(g_logger) << "thread test begin";
    // YAML::Node root = YAML::LoadFile("/home/user/wsl-code/webserver/bin/conf/log2.yml");
    // webserver::Config::LoadFromYaml(root);

    // 定义一个线程指针的向量，用于存储创建的线程
    std::vector<webserver::Thread::ptr> thrs;
    // 创建5个线程，每个线程执行fun1函数
    for(int i = 0; i < 10; ++i) {
        webserver::Thread::ptr thr(new webserver::Thread(&fun1, "name_" + std::to_string(i * 2)));
        // webserver::Thread::ptr thr2(new webserver::Thread(&fun2, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        // thrs.push_back(thr2);
    }

    // 等待所有线程执行完毕
    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    WEBSERVER_LOG_INFO(g_logger) << "thread test end";
    WEBSERVER_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}

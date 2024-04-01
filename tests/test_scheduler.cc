#include "src/webserver.h"

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    WEBSERVER_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;
    // WEBSERVER_LOG_INFO(g_logger) << "test in fiber";
    sleep(1);
    if(--s_count >= 0) {
        webserver::Scheduler::GetThis()->schedule(&test_fiber, webserver::GetThreadId());
    }
}

int main(int argc, char** argv) {
    WEBSERVER_LOG_INFO(g_logger) << "main";
    webserver::Scheduler sc(3, false, "test");
    // webserver::Scheduler sc;
    sc.start();
    sleep(2);
    WEBSERVER_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    WEBSERVER_LOG_INFO(g_logger) << "over";
    return 0;
}
/*
在这个上下文中，use_caller 是一个布尔值参数，用于指示调度器是否使用调用者线程作为主线程。
当 use_caller 为 true 时，调度器会使用调用者线程作为主线程，并且该调用者线程将成为调度器的主线程。
而当 use_caller 为 false 时，调度器将创建一个新的线程作为主线程。

这个参数的作用是为了控制调度器的初始化方式，当我们想要将调度器嵌入到已有的线程中时，可以将 use_caller 设置为 true，这样就不需要额外创建线程了。
*/
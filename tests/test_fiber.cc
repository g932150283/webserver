#include"src/webserver.h"
#include<vector>
webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

void run_in_fiber() {
    WEBSERVER_LOG_INFO(g_logger) << "run_in_fiber begin";
    // webserver::Fiber::back();
    webserver::Fiber::YieldToHold();
    WEBSERVER_LOG_INFO(g_logger) << "run_in_fiber end";
    webserver::Fiber::YieldToHold();
}

void test_fiber() {
    WEBSERVER_LOG_INFO(g_logger) << "main begin -1";
    {
        webserver::Fiber::GetThis();
        WEBSERVER_LOG_INFO(g_logger) << "main begin";
        webserver::Fiber::ptr fiber(new webserver::Fiber(run_in_fiber));
        fiber->swapIn();
        WEBSERVER_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        WEBSERVER_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    WEBSERVER_LOG_INFO(g_logger) << "main after end2";
}

int main(){
    // webserver::Fiber::GetThis();
    // WEBSERVER_LOG_INFO(g_logger) << "main begin";
    // webserver::Fiber::ptr fiber(new webserver::Fiber(run_in_fiber));
    // fiber->swapIn();
    // WEBSERVER_LOG_INFO(g_logger) << "main after swapIn";
    // fiber->swapIn();
    // WEBSERVER_LOG_INFO(g_logger) << "main after end";
    // return 0;
    webserver::Thread::SetName("main");

    std::vector<webserver::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(webserver::Thread::ptr(
                    new webserver::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}
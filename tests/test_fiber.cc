#include"src/webserver.h"

webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

void run_in_fiber() {
    WEBSERVER_LOG_INFO(g_logger) << "run_in_fiber begin";
    webserver::Fiber::GetThis()->swapOut();
    // webserver::Fiber::YieldToHold();
    WEBSERVER_LOG_INFO(g_logger) << "run_in_fiber end";
    // webserver::Fiber::YieldToHold();
}

int main(){
    WEBSERVER_LOG_INFO(g_logger) << "main begin";
    webserver::Fiber::ptr fiber(new webserver::Fiber(run_in_fiber));
    fiber->swapIn();
    WEBSERVER_LOG_INFO(g_logger) << "main after swapIn";
    fiber->swapIn();
    WEBSERVER_LOG_INFO(g_logger) << "main after end";
    return 0;
}
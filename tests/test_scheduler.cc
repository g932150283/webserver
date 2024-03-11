#include"src/webserver.h"

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    WEBSERVER_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        webserver::Scheduler::GetThis()->schedule(&test_fiber, webserver::GetThreadId());
    }
}

int main(int argc, char** argv) {
    WEBSERVER_LOG_INFO(g_logger) << "main";
    webserver::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    WEBSERVER_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    WEBSERVER_LOG_INFO(g_logger) << "over";
    return 0;
}

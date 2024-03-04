#include "src/webserver.h"
#include <unistd.h>

webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

int count = 0;
//sylar::RWMutex s_mutex;
// webserver::Mutex s_mutex;

void fun1() {
    WEBSERVER_LOG_INFO(g_logger) << "name: " << webserver::Thread::GetName()
                             << " this.name: " << webserver::Thread::GetThis()->getName()
                             << " id: " << webserver::GetThreadId()
                             << " this.id: " << webserver::Thread::GetThis()->getId();
    sleep(20);
    // for(int i = 0; i < 100000; ++i) {
    //     //sylar::RWMutex::WriteLock lock(s_mutex);
    //     // webserver::Mutex::Lock lock(s_mutex);
    //     ++count;
    // }
}

void fun2() {
    // while(true) {
    //     WEBSERVER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    // }
}

// void fun3() {
//     while(true) {
//         WEBSERVER_LOG_INFO(g_logger) << "========================================";
//     }
// }

int main(int argc, char** argv) {
    WEBSERVER_LOG_INFO(g_logger) << "thread test begin";
    // YAML::Node root = YAML::LoadFile("/home/sylar/test/sylar/bin/conf/log2.yml");
    // webserver::Config::LoadFromYaml(root);

    std::vector<webserver::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i) {
        webserver::Thread::ptr thr(new webserver::Thread(&fun1, "name_" + std::to_string(i)));
        //sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        //thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    WEBSERVER_LOG_INFO(g_logger) << "thread test end";
    // WEBSERVER_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}

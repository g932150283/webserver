// #include "src/tcp_server.h"
// #include "src/iomanager.h"
// #include "src/log.h"

// webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// void run() {
//     auto addr = webserver::Address::LookupAny("0.0.0.0:8033");
//     //auto addr2 = webserver::UnixAddress::ptr(new webserver::UnixAddress("/tmp/unix_addr"));
//     std::vector<webserver::Address::ptr> addrs;
//     addrs.push_back(addr);
//     //addrs.push_back(addr2);

//     webserver::TcpServer::ptr tcp_server(new webserver::TcpServer);
//     std::vector<webserver::Address::ptr> fails;
//     while(!tcp_server->bind(addrs, fails)) {
//         sleep(2);
//     }
//     tcp_server->start();
    
// }
// int main(int argc, char** argv) {
//     webserver::IOManager iom(2);
//     iom.schedule(run);
//     return 0;
// }
#include "src/tcp_server.h" // 包含TCP服务器的头文件
#include "src/iomanager.h" // 包含IO管理器的头文件
#include "src/log.h" // 包含日志系统的头文件

webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT(); // 全局日志对象

/**
 * @brief 运行函数，用于创建并启动TCP服务器
 * 
 */
void run() {
    auto addr = webserver::Address::LookupAny("0.0.0.0:8033"); // 解析地址
    //auto addr2 = webserver::UnixAddress::ptr(new webserver::UnixAddress("/tmp/unix_addr")); // Unix域套接字地址
    std::vector<webserver::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    webserver::TcpServer::ptr tcp_server(new webserver::TcpServer); // 创建TCP服务器对象
    std::vector<webserver::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) { // 尝试绑定地址，直到绑定成功为止
        sleep(2); // 等待2秒后重试
    }
    tcp_server->start(); // 启动服务器
}

/**
 * @brief 主函数，用于创建IO管理器并调度运行函数
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码
 */
int main(int argc, char** argv) {
    webserver::IOManager iom(2); // 创建IO管理器，线程数为2
    iom.schedule(run); // 调度运行函数
    return 0; // 返回程序退出码
}

// #include "src/tcp_server.h"
// #include "src/log.h"
// #include "src/iomanager.h"
// #include "src/bytearray.h"
// #include "src/address.h"

// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// class EchoServer : public webserver::TcpServer {
// public:
//     EchoServer(int type);
//     void handleClient(webserver::Socket::ptr client);

// private:
//     int m_type = 0;
// };

// EchoServer::EchoServer(int type)
//     :m_type(type) {
// }

// void EchoServer::handleClient(webserver::Socket::ptr client) {
//     WEBSERVER_LOG_INFO(g_logger) << "handleClient " << *client;   
//     webserver::ByteArray::ptr ba(new webserver::ByteArray);
//     while(true) {
//         ba->clear();
//         std::vector<iovec> iovs;
//         ba->getWriteBuffers(iovs, 1024);

//         int rt = client->recv(&iovs[0], iovs.size());
//         if(rt == 0) {
//             WEBSERVER_LOG_INFO(g_logger) << "client close: " << *client;
//             break;
//         } else if(rt < 0) {
//             WEBSERVER_LOG_INFO(g_logger) << "client error rt=" << rt
//                 << " errno=" << errno << " errstr=" << strerror(errno);
//             break;
//         }
//         ba->setPosition(ba->getPosition() + rt);
//         ba->setPosition(0);
//         //WEBSERVER_LOG_INFO(g_logger) << "recv rt=" << rt << " data=" << std::string((char*)iovs[0].iov_base, rt);
//         if(m_type == 1) {//text 
//             std::cout << ba->toString();// << std::endl;
//         } else {
//             std::cout << ba->toHexString();// << std::endl;
//         }
//         std::cout.flush();
//     }
// }

// int type = 1;

// void run() {
//     WEBSERVER_LOG_INFO(g_logger) << "server type=" << type;
//     EchoServer::ptr es(new EchoServer(type));
//     auto addr = webserver::Address::LookupAny("0.0.0.0:8020");
//     while(!es->bind(addr)) {
//         sleep(2);
//     }
//     es->start();
// }

// int main(int argc, char** argv) {
//     if(argc < 2) {
//         WEBSERVER_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
//         return 0;
//     }

//     if(!strcmp(argv[1], "-b")) {
//         type = 2;
//     }

//     webserver::IOManager iom(2);
//     iom.schedule(run);
//     return 0;
// }
#include "src/tcp_server.h" // 包含TCP服务器的头文件
#include "src/log.h" // 包含日志系统的头文件
#include "src/iomanager.h" // 包含IO管理器的头文件
#include "src/bytearray.h" // 包含字节数组的头文件
#include "src/address.h" // 包含地址解析的头文件

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT(); // 全局日志对象

/**
 * @brief 回显服务器类，继承自TCP服务器类
 * 
 */
class EchoServer : public webserver::TcpServer {
public:
    /**
     * @brief 构造函数，初始化回显服务器
     * 
     * @param type 服务器类型，1为文本模式，2为十六进制模式
     */
    EchoServer(int type);
    
    /**
     * @brief 处理客户端连接函数，接收客户端数据并回显
     * 
     * @param client 客户端套接字
     */
    void handleClient(webserver::Socket::ptr client);

private:
    int m_type = 0; // 服务器类型
};

/**
 * @brief 回显服务器构造函数
 * 
 * @param type 服务器类型，1为文本模式，2为十六进制模式
 */
EchoServer::EchoServer(int type)
    :m_type(type) {
}

/**
 * @brief 处理客户端连接函数，接收客户端数据并回显
 * 
 * @param client 客户端套接字
 */
void EchoServer::handleClient(webserver::Socket::ptr client) {
    WEBSERVER_LOG_INFO(g_logger) << "handleClient " << *client;   
    webserver::ByteArray::ptr ba(new webserver::ByteArray); // 创建字节数组指针
    while(true) {
        ba->clear(); // 清空字节数组内容
        std::vector<iovec> iovs; // 创建iovec向量
        ba->getWriteBuffers(iovs, 1024); // 获取可写入的iovec向量

        int rt = client->recv(&iovs[0], iovs.size()); // 接收客户端数据
        if(rt == 0) {
            WEBSERVER_LOG_INFO(g_logger) << "client close: " << *client; // 客户端关闭连接，输出日志
            break;
        } else if(rt < 0) {
            WEBSERVER_LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno); // 接收错误，输出日志
            break;
        }
        ba->setPosition(ba->getPosition() + rt); // 移动字节数组指针
        ba->setPosition(0); // 将指针位置设置为起始位置
        if(m_type == 1) {//text 
            std::cout << ba->toString(); // 输出文本模式的字节数组内容
        } else {
            std::cout << ba->toHexString(); // 输出十六进制模式的字节数组内容
        }
        std::cout.flush(); // 刷新输出缓冲区
    }
}

int type = 1;

/**
 * @brief 运行函数，用于创建并启动回显服务器
 * 
 */
void run() {
    WEBSERVER_LOG_INFO(g_logger) << "server type=" << type; // 输出服务器类型日志
    EchoServer::ptr es(new EchoServer(type)); // 创建回显服务器对象指针
    auto addr = webserver::Address::LookupAny("0.0.0.0:8020"); // 解析地址
    while(!es->bind(addr)) { // 绑定地址
        sleep(2); // 等待2秒后重试
    }
    es->start(); // 启动服务器
}

/**
 * @brief 主函数，用于解析命令行参数并创建IO管理器调度运行函数
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码
 */
int main(int argc, char** argv) {
    if(argc < 2) {
        WEBSERVER_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]"; // 输出用法日志
        return 0;
    }

    if(!strcmp(argv[1], "-b")) {
        type = 2; // 设置服务器类型为十六进制模式
    }

    webserver::IOManager iom(2); // 创建IO管理器，线程数为2
    iom.schedule(run); // 调度运行函数
    return 0; // 返回程序退出码
}

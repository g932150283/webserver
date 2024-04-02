#include "src/webserver.h"
#include "src/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

int sock = 0;

void test_fiber() {
    WEBSERVER_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    //sleep(3);

    //close(sock);
    //webserver::IOManager::GetThis()->cancelAll(sock);
    // 创建socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // 地址转换
    inet_pton(AF_INET, "220.181.38.149", &addr.sin_addr.s_addr);
    // 发起连接
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    // EINPROGRESS在处理中
    } else if(errno == EINPROGRESS) {
        WEBSERVER_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        webserver::IOManager::GetThis()->addEvent(sock, webserver::IOManager::READ, [](){
            WEBSERVER_LOG_INFO(g_logger) << "read callback";
        });
        webserver::IOManager::GetThis()->addEvent(sock, webserver::IOManager::WRITE, [](){
            WEBSERVER_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            webserver::IOManager::GetThis()->cancelEvent(sock, webserver::IOManager::READ);
            // 关闭连接
            close(sock);
        });
    } else {
        WEBSERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    webserver::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

webserver::Timer::ptr s_timer;
void test_timer() {
    webserver::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        WEBSERVER_LOG_INFO(g_logger) << "hello timer i=" << i;
        // if(++i == 3) {
        //     s_timer->reset(500, true);
        //     // s_timer->cancel();
        //     // std::cout << "aaaaaaaaaa" << std::endl;
        // }
        if (++i == 5) {
            s_timer->reset(2000, true);
        }
        if (i == 10) {
            s_timer->cancel();
        }

    }, true);
}

int main(int argc, char** argv) {
    // test1();
    g_logger->setLevel(webserver::LogLevel::INFO);
    test_timer();
    return 0;
}


// int sock = 0;

void test_fiber1() {
    WEBSERVER_LOG_INFO(g_logger) << "test_fiber1";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "220.181.38.149", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        WEBSERVER_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);

        webserver::IOManager::GetThis()->addEvent(sock, webserver::IOManager::READ, [](){
            WEBSERVER_LOG_INFO(g_logger) << "read callback";
            char temp[1000];
            int rt = read(sock, temp, 1000);
            if (rt >= 0) {
                std::string ans(temp, rt);
                WEBSERVER_LOG_INFO(g_logger) << "read:["<< ans << "]";
            } else {
                WEBSERVER_LOG_INFO(g_logger) << "read rt = " << rt;
            }
            });
        webserver::IOManager::GetThis()->addEvent(sock, webserver::IOManager::WRITE, [](){
            WEBSERVER_LOG_INFO(g_logger) << "write callback";
            int rt = write(sock, "GET / HTTP/1.1\r\ncontent-length: 0\r\n\r\n",38);
            WEBSERVER_LOG_INFO(g_logger) << "write rt = " << rt;
            });
    } else {
        WEBSERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test01() {
    webserver::IOManager iom(2, true, "IOM");
    iom.schedule(test_fiber1);
}

// int main(int argc, char** argv) {
//     g_logger->setLevel(webserver::LogLevel::INFO);
//     test01();
    
//     return 0;
// }
// #include <iostream>
// #include "src/http/http_connection.h"
// #include "src/log.h"
// #include "src/iomanager.h"
// #include "src/http/http_parser.h"
// #include "src/streams/zlib_stream.h"
// #include <fstream>

// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// void test_pool() {
//     webserver::http::HttpConnectionPool::ptr pool(new webserver::http::HttpConnectionPool(
//                 "www.sylar.top", "", 80, false, 10, 1000 * 30, 5));

//     webserver::IOManager::GetThis()->addTimer(1000, [pool](){
//             auto r = pool->doGet("/", 300);
//             WEBSERVER_LOG_INFO(g_logger) << r->toString();
//     }, true);
// }

// void run() {
//     webserver::Address::ptr addr = webserver::Address::LookupAnyIPAddress("www.sylar.top:80");
//     if(!addr) {
//         WEBSERVER_LOG_INFO(g_logger) << "get addr error";
//         return;
//     }

//     webserver::Socket::ptr sock = webserver::Socket::CreateTCP(addr);
//     bool rt = sock->connect(addr);
//     if(!rt) {
//         WEBSERVER_LOG_INFO(g_logger) << "connect " << *addr << " failed";
//         return;
//     }

//     webserver::http::HttpConnection::ptr conn(new webserver::http::HttpConnection(sock));
//     webserver::http::HttpRequest::ptr req(new webserver::http::HttpRequest);
//     req->setPath("/blog/");
//     req->setHeader("host", "www.sylar.top");
//     WEBSERVER_LOG_INFO(g_logger) << "req:" << std::endl
//         << *req;

//     conn->sendRequest(req);
//     auto rsp = conn->recvResponse();

//     if(!rsp) {
//         WEBSERVER_LOG_INFO(g_logger) << "recv response error";
//         return;
//     }
//     WEBSERVER_LOG_INFO(g_logger) << "rsp:" << std::endl
//         << *rsp;

//     std::ofstream ofs("rsp.dat");
//     ofs << *rsp;

//     WEBSERVER_LOG_INFO(g_logger) << "=========================";

//     auto r = webserver::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
//     WEBSERVER_LOG_INFO(g_logger) << "result=" << r->result
//         << " error=" << r->error
//         << " rsp=" << (r->response ? r->response->toString() : "");

//     WEBSERVER_LOG_INFO(g_logger) << "=========================";
//     test_pool();
// }

// void test_https() {
//     auto r = webserver::http::HttpConnection::DoGet("http://www.baidu.com/", 300, {
//                         {"Accept-Encoding", "gzip, deflate, br"},
//                         {"Connection", "keep-alive"},
//                         {"User-Agent", "curl/7.29.0"}
//             });
//     WEBSERVER_LOG_INFO(g_logger) << "result=" << r->result
//         << " error=" << r->error
//         << " rsp=" << (r->response ? r->response->toString() : "");

//     //webserver::http::HttpConnectionPool::ptr pool(new webserver::http::HttpConnectionPool(
//     //            "www.baidu.com", "", 80, false, 10, 1000 * 30, 5));
//     auto pool = webserver::http::HttpConnectionPool::Create(
//                     "https://www.baidu.com", "", 10, 1000 * 30, 5);
//     webserver::IOManager::GetThis()->addTimer(1000, [pool](){
//             auto r = pool->doGet("/", 3000, {
//                         {"Accept-Encoding", "gzip, deflate, br"},
//                         {"User-Agent", "curl/7.29.0"}
//                     });
//             WEBSERVER_LOG_INFO(g_logger) << r->toString();
//     }, true);
// }

// void test_data() {
//     webserver::Address::ptr addr = webserver::Address::LookupAny("www.baidu.com:80");
//     auto sock = webserver::Socket::CreateTCP(addr);

//     sock->connect(addr);
//     const char buff[] = "GET / HTTP/1.1\r\n"
//                 "connection: close\r\n"
//                 "Accept-Encoding: gzip, deflate, br\r\n"
//                 "Host: www.baidu.com\r\n\r\n";
//     sock->send(buff, sizeof(buff));

//     std::string line;
//     line.resize(1024);

//     std::ofstream ofs("http.dat", std::ios::binary);
//     int total = 0;
//     int len = 0;
//     while((len = sock->recv(&line[0], line.size())) > 0) {
//         total += len;
//         ofs.write(line.c_str(), len);
//     }
//     std::cout << "total: " << total << " tellp=" << ofs.tellp() << std::endl;
//     ofs.flush();
// }

// void test_parser() {
//     std::ifstream ifs("http.dat", std::ios::binary);
//     std::string content;
//     std::string line;
//     line.resize(1024);

//     int total = 0;
//     while(!ifs.eof()) {
//         ifs.read(&line[0], line.size());
//         content.append(&line[0], ifs.gcount());
//         total += ifs.gcount();
//     }

//     std::cout << "length: " << content.size() << " total: " << total << std::endl;
//     webserver::http::HttpResponseParser parser;
//     size_t nparse = parser.execute(&content[0], content.size(), false);
//     std::cout << "finish: " << parser.isFinished() << std::endl;
//     content.resize(content.size() - nparse);
//     std::cout << "rsp: " << *parser.getData() << std::endl;

//     auto& client_parser = parser.getParser();
//     std::string body;
//     int cl = 0;
//     do {
//         size_t nparse = parser.execute(&content[0], content.size(), true);
//         std::cout << "content_len: " << client_parser.content_len
//                   << " left: " << content.size()
//                   << std::endl;
//         cl += client_parser.content_len;
//         content.resize(content.size() - nparse);
//         body.append(content.c_str(), client_parser.content_len);
//         content = content.substr(client_parser.content_len + 2);
//     } while(!client_parser.chunks_done);

//     std::cout << "total: " << body.size() << " content:" << cl << std::endl;

//     webserver::ZlibStream::ptr stream = webserver::ZlibStream::CreateGzip(false);
//     stream->write(body.c_str(), body.size());
//     stream->flush();

//     body = stream->getResult();

//     std::ofstream ofs("http.txt");
//     ofs << body;
// }

// int main(int argc, char** argv) {
//     webserver::IOManager iom(2);
//     //iom.schedule(run);
//     iom.schedule(test_https);
//     return 0;
// }
#include <iostream>
#include "src/http/http_connection.h"
#include "src/log.h"
#include "src/iomanager.h"
#include "src/http/http_parser.h"
#include "src/streams/zlib_stream.h"
#include <fstream>

// 全局日志记录器
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// 测试连接池
void test_pool() {
    // 创建HTTP连接池
    webserver::http::HttpConnectionPool::ptr pool(new webserver::http::HttpConnectionPool(
                "www.sylar.top", "", 80, false, 10, 1000 * 30, 5));

    // 添加定时器任务，执行GET请求
    webserver::IOManager::GetThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 300);
            WEBSERVER_LOG_INFO(g_logger) << r->toString();
    }, true);
}

// 执行简单的HTTP GET请求
void run() {
    // 查找目标服务器的IP地址
    webserver::Address::ptr addr = webserver::Address::LookupAnyIPAddress("www.sylar.top:80");
    if(!addr) {
        WEBSERVER_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    // 创建TCP套接字并连接到目标服务器
    webserver::Socket::ptr sock = webserver::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        WEBSERVER_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    // 创建HTTP连接对象
    webserver::http::HttpConnection::ptr conn(new webserver::http::HttpConnection(sock));
    // 创建HTTP请求对象
    webserver::http::HttpRequest::ptr req(new webserver::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    WEBSERVER_LOG_INFO(g_logger) << "req:" << std::endl
        << *req;

    // 发送HTTP请求
    conn->sendRequest(req);
    // 接收HTTP响应
    auto rsp = conn->recvResponse();

    if(!rsp) {
        WEBSERVER_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    WEBSERVER_LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;

    // 将响应内容写入文件
    std::ofstream ofs("rsp.dat");
    ofs << *rsp;

    WEBSERVER_LOG_INFO(g_logger) << "=========================";

    // 使用静态函数执行HTTP GET请求
    auto r = webserver::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    WEBSERVER_LOG_INFO(g_logger) << "result=" << r->result
        << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    WEBSERVER_LOG_INFO(g_logger) << "=========================";
    // 执行连接池测试
    test_pool();
}

// 执行HTTPS请求
void test_https() {
    // 执行HTTPS GET请求
    auto r = webserver::http::HttpConnection::DoGet("http://www.baidu.com/", 300, {
                        {"Accept-Encoding", "gzip, deflate, br"},
                        {"Connection", "keep-alive"},
                        {"User-Agent", "curl/7.29.0"}
            });
    WEBSERVER_LOG_INFO(g_logger) << "result=" << r->result
        << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    // 创建HTTPS连接池并添加定时器任务，执行HTTPS GET请求
    auto pool = webserver::http::HttpConnectionPool::Create(
                    "https://www.baidu.com", "", 10, 1000 * 30, 5);
    webserver::IOManager::GetThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 3000, {
                        {"Accept-Encoding", "gzip, deflate, br"},
                        {"User-Agent", "curl/7.29.0"}
                    });
            WEBSERVER_LOG_INFO(g_logger) << r->toString();
    }, true);
}

// 测试发送HTTP请求并将响应保存至文件
void test_data() {
    // 查找目标服务器的IP地址
    webserver::Address::ptr addr = webserver::Address::LookupAny("www.baidu.com:80");
    auto sock = webserver::Socket::CreateTCP(addr);

    // 连接到目标服务器
    sock->connect(addr);
    const char buff[] = "GET / HTTP/1.1\r\n"
                "connection: close\r\n"
                "Accept-Encoding: gzip, deflate, br\r\n"
                "Host: www.baidu.com\r\n\r\n";
    // 发送HTTP请求
    sock->send(buff, sizeof(buff));

    std::string line;
    line.resize(1024);

    // 从套接字接收HTTP响应并保存至文件
    std::ofstream ofs("http.dat", std::ios::binary);
    int total = 0;
    int len = 0;
    while((len = sock->recv(&line[0], line.size())) > 0) {
        total += len;
        ofs.write(line.c_str(), len);
    }
    std::cout << "total: " << total << " tellp=" << ofs.tellp() << std::endl;
    ofs.flush();
}

// 测试HTTP响应解析器
void test_parser() {
    // 从文件读取HTTP响应内容
    std::ifstream ifs("http.dat", std::ios::binary);
    std::string content;
    std::string line;
    line.resize(1024);

    int total = 0;
    while(!ifs.eof()) {
        ifs.read(&line[0], line.size());
        content.append(&line[0], ifs.gcount());
        total += ifs.gcount();
    }

    std::cout << "length: " << content.size() << " total: " << total << std::endl;
    // 创建HTTP响应解析器
    webserver::http::HttpResponseParser parser;
    // 解析HTTP响应
    size_t nparse = parser.execute(&content[0], content.size(), false);
    std::cout << "finish: " << parser.isFinished() << std::endl;
    content.resize(content.size() - nparse);
    std::cout << "rsp: " << *parser.getData() << std::endl;

    auto& client_parser = parser.getParser();
    std::string body;
    int cl = 0;
    do {
        // 继续解析HTTP响应
        size_t nparse = parser.execute(&content[0], content.size(), true);
        std::cout << "content_len: " << client_parser.content_len
                  << " left: " << content.size()
                  << std::endl;
        cl += client_parser.content_len;
        content.resize(content.size() - nparse);
        body.append(content.c_str(), client_parser.content_len);
        content = content.substr(client_parser.content_len + 2);
    } while(!client_parser.chunks_done);

    std::cout << "total: " << body.size() << " content:" << cl << std::endl;

    // 使用Zlib进行解压缩
    webserver::ZlibStream::ptr stream = webserver::ZlibStream::CreateGzip(false);
    stream->write(body.c_str(), body.size());
    stream->flush();

    body = stream->getResult();

    // 将解压后的内容写入文件
    std::ofstream ofs("http.txt");
    ofs << body;
}

int main(int argc, char** argv) {
    // 创建IO线程池
    webserver::IOManager iom(2);
    // 执行运行函数
    iom.schedule(run);
    // 执行HTTPS测试函数
    // iom.schedule(test_https);
    return 0;
}

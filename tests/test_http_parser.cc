// #include "src/http/http_parser.h"
// #include "src/log.h"

// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// const char test_request_data[] = "POST / HTTP/1.1\r\n"
//                                 "Host: www.sylar.top\r\n"
//                                 "Content-Length: 10\r\n\r\n"
//                                 "1234567890";

// void test_request() {
//     webserver::http::HttpRequestParser parser;
//     std::string tmp = test_request_data;
//     size_t s = parser.execute(&tmp[0], tmp.size());
//     WEBSERVER_LOG_ERROR(g_logger) << "execute rt=" << s
//         << "has_error=" << parser.hasError()
//         << " is_finished=" << parser.isFinished()
//         << " total=" << tmp.size()
//         << " content_length=" << parser.getContentLength();
//     tmp.resize(tmp.size() - s);
//     WEBSERVER_LOG_INFO(g_logger) << parser.getData()->toString();
//     WEBSERVER_LOG_INFO(g_logger) << tmp;
// }

// const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
//         "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
//         "Server: Apache\r\n"
//         "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
//         "ETag: \"51-47cf7e6ee8400\"\r\n"
//         "Accept-Ranges: bytes\r\n"
//         "Content-Length: 81\r\n"
//         "Cache-Control: max-age=86400\r\n"
//         "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
//         "Connection: Close\r\n"
//         "Content-Type: text/html\r\n\r\n"
//         "<html>\r\n"
//         "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
//         "</html>\r\n";

// void test_response() {
//     webserver::http::HttpResponseParser parser;
//     std::string tmp = test_response_data;
//     size_t s = parser.execute(&tmp[0], tmp.size(), true);
//     WEBSERVER_LOG_ERROR(g_logger) << "execute rt=" << s
//         << " has_error=" << parser.hasError()
//         << " is_finished=" << parser.isFinished()
//         << " total=" << tmp.size()
//         << " content_length=" << parser.getContentLength()
//         << " tmp[s]=" << tmp[s];

//     tmp.resize(tmp.size() - s);

//     WEBSERVER_LOG_INFO(g_logger) << parser.getData()->toString();
//     WEBSERVER_LOG_INFO(g_logger) << tmp;
// }

// int main(int argc, char** argv) {
//     test_request();
//     WEBSERVER_LOG_INFO(g_logger) << "--------------";
//     test_response();
//     return 0;
// }
#include "src/http/http_parser.h" // 包含HTTP请求和响应解析器的头文件
#include "src/log.h" // 包含日志系统的头文件

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT(); // 全局日志对象

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: www.sylar.top\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890"; // 测试用的HTTP请求数据

/**
 * @brief 测试HTTP请求解析器
 * 
 */
void test_request() {
    webserver::http::HttpRequestParser parser; // 创建HTTP请求解析器对象
    std::string tmp = test_request_data; // 将测试数据转换为字符串
    size_t s = parser.execute(&tmp[0], tmp.size()); // 解析HTTP请求数据
    WEBSERVER_LOG_ERROR(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength();
    tmp.resize(tmp.size() - s); // 调整字符串长度
    WEBSERVER_LOG_INFO(g_logger) << parser.getData()->toString(); // 输出解析后的HTTP请求数据
    WEBSERVER_LOG_INFO(g_logger) << tmp; // 输出剩余未解析的数据
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n"; // 测试用的HTTP响应数据

/**
 * @brief 测试HTTP响应解析器
 * 
 */
void test_response() {
    webserver::http::HttpResponseParser parser; // 创建HTTP响应解析器对象
    std::string tmp = test_response_data; // 将测试数据转换为字符串
    size_t s = parser.execute(&tmp[0], tmp.size(), true); // 解析HTTP响应数据
    WEBSERVER_LOG_ERROR(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength()
        << " tmp[s]=" << tmp[s];
    tmp.resize(tmp.size() - s); // 调整字符串长度
    WEBSERVER_LOG_INFO(g_logger) << parser.getData()->toString(); // 输出解析后的HTTP响应数据
    WEBSERVER_LOG_INFO(g_logger) << tmp; // 输出剩余未解析的数据
}

/**
 * @brief 主函数，用于测试HTTP请求和响应解析器
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码
 */
int main(int argc, char** argv) {
    test_request(); // 测试HTTP请求解析器
    WEBSERVER_LOG_INFO(g_logger) << "--------------"; // 输出分隔线
    test_response(); // 测试HTTP响应解析器
    return 0; // 返回程序退出码
}

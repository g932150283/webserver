#include "http_parser.h"
#include "src/log.h"
#include "src/config.h"
#include <string.h>

namespace webserver {
namespace http {

/**
 * 全局日志记录器实例。
 * 
 * 用于系统级日志记录的全局日志记录器，通过 WEBSERVER_LOG_NAME 宏初始化，日志名称为 "system"。
 * 可用于记录 HTTP 服务器的系统级事件和错误。
 */
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * HTTP 请求缓冲区大小配置变量。
 *
 * 定义了一个用于存储 HTTP 请求缓冲区大小的配置变量。该变量可通过配置文件或运行时设置，
 * 默认值为 4KB（4096字节）。该缓冲区用于临时存储客户端发送的 HTTP 请求数据。
 */
static webserver::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    webserver::Config::Lookup("http.request.buffer_size"
                ,(uint64_t)(4 * 1024), "http request buffer size");

/**
 * HTTP 请求最大正文大小配置变量。
 *
 * 定义了一个用于限制 HTTP 请求正文大小的配置变量。该变量允许通过配置文件或运行时设置，
 * 默认值为 64MB（67108864字节）。这个限制帮助防止因接收过大的请求而导致的资源耗尽问题。
 */
static webserver::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    webserver::Config::Lookup("http.request.max_body_size"
                ,(uint64_t)(64 * 1024 * 1024), "http request max body size");

/**
 * HTTP 响应缓冲区大小配置变量。
 *
 * 定义了一个用于存储 HTTP 响应缓冲区大小的配置变量。该变量可通过配置文件或运行时设置，
 * 默认值为 4KB（4096字节）。该缓冲区用于临时存储服务器响应数据，直到它们被发送到客户端。
 */
static webserver::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    webserver::Config::Lookup("http.response.buffer_size"
                ,(uint64_t)(4 * 1024), "http response buffer size");

/**
 * HTTP 响应最大正文大小配置变量。
 *
 * 定义了一个用于限制 HTTP 响应正文大小的配置变量。该变量允许通过配置文件或运行时设置，
 * 默认值为 64MB（67108864字节）。这个限制有助于控制服务器发送的数据量，防止发送过大的响应。
 */
static webserver::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
    webserver::Config::Lookup("http.response.max_body_size"
                ,(uint64_t)(64 * 1024 * 1024), "http response max body size");

/**
 * 静态变量：HTTP 请求缓冲区大小。
 *
 * 用于存储 HTTP 请求处理时使用的缓冲区大小。该值在服务器启动时根据配置文件或默认设置初始化，
 * 并在整个服务器运行期间保持不变。缓冲区大小直接影响服务器处理大量请求时的内存使用和性能。
 */
static uint64_t s_http_request_buffer_size = 0;

/**
 * 静态变量：HTTP 请求最大正文大小。
 *
 * 定义了服务器接受的 HTTP 请求正文的最大允许大小。此限制有助于防止恶意用户通过发送过大的请求
 * 正文来尝试耗尽服务器资源。该值根据服务器配置初始化，并在运行期间不变。
 */
static uint64_t s_http_request_max_body_size = 0;

/**
 * 静态变量：HTTP 响应缓冲区大小。
 *
 * 指定了服务器在发送 HTTP 响应时使用的缓冲区大小。合理的缓冲区大小可以提高发送数据的效率，
 * 减少网络I/O次数，从而提升整体性能。该值在服务器启动时初始化，并在运行期间保持不变。
 */
static uint64_t s_http_response_buffer_size = 0;

/**
 * 静态变量：HTTP 响应最大正文大小。
 *
 * 设置了服务器能够发送的 HTTP 响应正文的最大大小。这个限制可以帮助服务器管理员控制流出的数据量，
 * 避免因单个响应体积过大而影响服务器的稳定性和性能。根据服务器配置在启动时初始化。
 */
static uint64_t s_http_response_max_body_size = 0;


/**
 * 获取 HTTP 请求缓冲区大小。
 *
 * 此方法提供了一种访问 HTTP 请求处理时使用的缓冲区大小的方式。缓冲区大小是在服务器启动时根据
 * 配置确定的，并影响到服务器处理请求的能力。
 *
 * 返回值:
 * - uint64_t: HTTP 请求缓冲区的大小，以字节为单位。
 */
uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

/**
 * 获取 HTTP 请求最大正文大小。
 *
 * 此方法允许访问服务器接受的 HTTP 请求正文的最大允许大小。这个大小限制有助于防止恶意用户试图
 * 通过发送巨大的请求正文来耗尽服务器资源。
 *
 * 返回值:
 * - uint64_t: HTTP 请求正文的最大允许大小，以字节为单位。
 */
uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

/**
 * 获取 HTTP 响应缓冲区大小。
 *
 * 此方法提供了一种访问服务器在发送 HTTP 响应时使用的缓冲区大小的方式。合适的缓冲区大小可以提高
 * 数据发送的效率，减少网络 I/O 操作次数。
 *
 * 返回值:
 * - uint64_t: HTTP 响应缓冲区的大小，以字节为单位。
 */
uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

/**
 * 获取 HTTP 响应最大正文大小。
 *
 * 此方法允许访问服务器能够发送的 HTTP 响应正文的最大大小。这个大小限制有助于管理员控制因响应体积
 * 过大而可能导致的性能问题。
 *
 * 返回值:
 * - uint64_t: HTTP 响应正文的最大允许大小，以字节为单位。
 */
uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}


namespace {
    // 定义一个结构体，用于在程序启动时初始化 HTTP 请求和响应的大小配置
    struct _RequestSizeIniter {
        // 构造函数，会在程序启动时自动调用
        _RequestSizeIniter() {
            // 初始化 HTTP 请求缓冲区大小的静态变量
            s_http_request_buffer_size = g_http_request_buffer_size->getValue();
            // 初始化 HTTP 请求最大正文大小的静态变量
            s_http_request_max_body_size = g_http_request_max_body_size->getValue();
            // 初始化 HTTP 响应缓冲区大小的静态变量
            s_http_response_buffer_size = g_http_response_buffer_size->getValue();
            // 初始化 HTTP 响应最大正文大小的静态变量
            s_http_response_max_body_size = g_http_response_max_body_size->getValue();

            // 为 HTTP 请求缓冲区大小配置添加监听器，以便在配置更改时自动更新静态变量
            g_http_request_buffer_size->addListener(
                    [](const uint64_t& ov, const uint64_t& nv){
                    s_http_request_buffer_size = nv; // 更新 HTTP 请求缓冲区大小的静态变量
            });

            // 为 HTTP 请求最大正文大小配置添加监听器，以便在配置更改时自动更新静态变量
            g_http_request_max_body_size->addListener(
                    [](const uint64_t& ov, const uint64_t& nv){
                    s_http_request_max_body_size = nv; // 更新 HTTP 请求最大正文大小的静态变量
            });

            // 为 HTTP 响应缓冲区大小配置添加监听器，以便在配置更改时自动更新静态变量
            g_http_response_buffer_size->addListener(
                    [](const uint64_t& ov, const uint64_t& nv){
                    s_http_response_buffer_size = nv; // 更新 HTTP 响应缓冲区大小的静态变量
            });

            // 为 HTTP 响应最大正文大小配置添加监听器，以便在配置更改时自动更新静态变量
            g_http_response_max_body_size->addListener(
                    [](const uint64_t& ov, const uint64_t& nv){
                    s_http_response_max_body_size = nv; // 更新 HTTP 响应最大正文大小的静态变量
            });
        }
    };
    // 创建 _RequestSizeIniter 类的一个静态实例，确保其构造函数在程序启动时被调用
    static _RequestSizeIniter _init;
}


/**
 * 处理HTTP请求方法
 * 
 * @param data 指向HttpRequestParser对象的指针，用于处理HTTP请求
 * @param at 指向请求方法开始位置的字符指针
 * @param length 请求方法的长度
 * 
 * 此函数解析HTTP请求方法并设置给HttpRequestParser对象。如果遇到无效的HTTP方法，
 * 会记录警告日志并设置解析器错误。
 */
void on_request_method(void *data, const char *at, size_t length) {
    // 将void指针转换为HttpRequestParser指针
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    // 将字符数组转换为HttpMethod枚举
    HttpMethod m = CharsToHttpMethod(at);

    // 判断方法是否有效
    if(m == HttpMethod::INVALID_METHOD) {
        // 记录无效方法的警告日志
        WEBSERVER_LOG_WARN(g_logger) << "invalid http request method: " << std::string(at, length);
        // 设置解析错误码
        parser->setError(1000);
        return;
    }
    // 设置HTTP方法
    parser->getData()->setMethod(m);
}

/**
 * 处理HTTP请求URI
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param at 指向URI开始位置的字符指针
 * @param length URI的长度
 * 
 * 此函数当前为空实现，用于以后可能的扩展。
 */
void on_request_uri(void *data, const char *at, size_t length) {
}

/**
 * 处理HTTP请求片段
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param at 指向片段开始位置的字符指针
 * @param length 片段的长度
 * 
 * 此函数解析HTTP请求中的片段信息并设置给HttpRequestParser对象。
 */
void on_request_fragment(void *data, const char *at, size_t length) {
    // 转换指针类型并设置请求片段
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));
}

/**
 * 处理HTTP请求路径
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param at 指向路径开始位置的字符指针
 * @param length 路径的长度
 * 
 * 此函数解析HTTP请求中的路径信息并设置给HttpRequestParser对象。
 */
void on_request_path(void *data, const char *at, size_t length) {
    // 转换指针类型并设置请求路径
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

/**
 * 处理HTTP请求查询字符串
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param at 指向查询字符串开始位置的字符指针
 * @param length 查询字符串的长度
 * 
 * 此函数解析HTTP请求中的查询字符串并设置给HttpRequestParser对象。
 */
void on_request_query(void *data, const char *at, size_t length) {
    // 转换指针类型并设置请求查询字符串
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

/**
 * 处理HTTP请求版本
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param at 指向版本信息开始位置的字符指针
 * @param length 版本信息的长度
 * 
 * 此函数解析HTTP请求中的版本信息并设置给HttpRequestParser对象。如果遇到无效版本信息，
 * 会记录警告日志并设置解析器错误。
 */
void on_request_version(void *data, const char *at, size_t length) {
    // 转换指针类型
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    // 默认版本为0
    uint8_t v = 0;

    // 匹配并设置HTTP版本
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        // 记录无效版本的警告日志
        WEBSERVER_LOG_WARN(g_logger) << "invalid http request version: " << std::string(at, length);
        // 设置解析错误码
        parser->setError(1001);
        return;
    }
    // 设置HTTP版本
    parser->getData()->setVersion(v);
}

/**
 * 处理HTTP请求头解析完成后的回调
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param at 指向数据开始位置的字符指针
 * @param length 数据的长度
 * 
 * 此函数当前为空实现，可用于在请求头解析完成后执行必要的操作。
 */
void on_request_header_done(void *data, const char *at, size_t length) {
}

/**
 * 处理HTTP请求中的头字段
 * 
 * @param data 指向HttpRequestParser对象的指针
 * @param field 指向字段名称开始位置的字符指针
 * @param flen 字段名称的长度
 * @param value 指向字段值开始位置的字符指针
 * @param vlen 字段值的长度
 * 
 * 此函数解析HTTP请求中的头字段及其值，并设置给HttpRequestParser对象。如果字段长度为0，
 * 会记录警告日志。
 */
void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    // 转换指针类型
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);

    // 检查字段名称长度
    if(flen == 0) {
        // 记录字段长度为0的警告日志
        WEBSERVER_LOG_WARN(g_logger) << "invalid http request field length == 0";
        return;
    }
    // 设置HTTP请求头字段
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}


/**
 * HttpRequestParser构造函数
 * 当解析到时调用回调函数，将对应的数据保存到报文中
 * 
 * 此构造函数初始化HttpRequestParser对象，包括错误码、数据对象以及HTTP解析器。
 * 它还将解析器的各个回调函数关联到相应的处理函数，并设置解析器的数据上下文为当前对象。
 */
HttpRequestParser::HttpRequestParser()
    : m_error(0) { // 初始化错误码为0
    // 创建一个新的HttpRequest对象并将其赋值给m_data智能指针
    m_data.reset(new webserver::http::HttpRequest);
    // 初始化HTTP解析器
    http_parser_init(&m_parser);
    // 设置解析器的各个回调函数
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    // 将解析器的数据上下文设置为当前HttpRequestParser对象
    m_parser.data = this;
}

/**
 * 获取Content-Length的值
 * 
 * @return 请求中"Content-Length"头字段的值，如果不存在则返回0。
 * 
 * 此方法用于获取HTTP请求头中"Content-Length"字段的值。
 */
uint64_t HttpRequestParser::getContentLength() {
    // 从HTTP请求中获取"Content-Length"字段的值
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

/**
 * 执行HTTP解析
 * 
 * @param data 指向数据缓冲区的指针
 * @param len 数据缓冲区中的数据长度
 * @return 解析过程中处理的字节数
 * 
 * 此方法将数据提供给HTTP解析器并执行解析操作。解析完成后，会更新数据缓冲区，
 * 移除已解析的数据，并返回处理的字节数。
 * 1 成功
 * -1 有错误
 * >0 已处理的字节数，并且data有效数据为len - v
 */
size_t HttpRequestParser::execute(char* data, size_t len) {
    // 执行HTTP解析，并返回处理的字节数
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    // 解析完将剩余数据移动到起始地址
    // 移动未处理的数据至缓冲区头部
    memmove(data, data + offset, (len - offset));
    return offset;
}

/**
 * 检查HTTP请求解析是否完成
 * 
 * @return 解析完成返回1，否则返回0。
 * 
 * 此方法检查HTTP解析器是否已完成对HTTP请求的解析。
 */
int HttpRequestParser::isFinished() {
    // 检查解析是否完成
    return http_parser_finish(&m_parser);
}

/**
 * 检查解析过程中是否发生错误
 * 
 * @return 有错误返回非0值，无错误返回0。
 * 
 * 此方法检查在HTTP解析过程中是否发生了错误。
 */
int HttpRequestParser::hasError() {
    // 检查是否有错误发生
    return m_error || http_parser_has_error(&m_parser);
}

/**
 * 处理HTTP响应的原因短语
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param at 指向响应原因短语开始位置的字符指针
 * @param length 原因短语的长度
 * 
 * 此函数解析HTTP响应的原因短语并将其设置到HttpResponseParser对象中。
 */
void on_response_reason(void *data, const char *at, size_t length) {
    // 将void指针转换为HttpResponseParser指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    // 设置响应的原因短语
    parser->getData()->setReason(std::string(at, length));
}

/**
 * 处理HTTP响应状态码
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param at 指向状态码开始位置的字符指针
 * @param length 状态码的长度
 * 
 * 此函数解析HTTP响应状态码并将其设置为HttpResponseParser对象中的状态。
 */
void on_response_status(void *data, const char *at, size_t length) {
    // 将void指针转换为HttpResponseParser指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    // 将字符数组转换为HttpStatus枚举
    HttpStatus status = (HttpStatus)(atoi(at));
    // 设置响应的状态
    parser->getData()->setStatus(status);
}

/**
 * 处理HTTP响应的块数据（当前未实现）
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param at 指向块数据开始位置的字符指针
 * @param length 块数据的长度
 * 
 * 此函数当前未实现，用于处理HTTP响应的块数据。
 */
void on_response_chunk(void *data, const char *at, size_t length) {
}

/**
 * 处理HTTP响应版本信息
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param at 指向版本信息开始位置的字符指针
 * @param length 版本信息的长度
 * 
 * 此函数解析HTTP响应的版本信息并将其设置到HttpResponseParser对象中。如果版本信息无效，
 * 会记录警告日志并设置解析器错误。
 */
void on_response_version(void *data, const char *at, size_t length) {
    // 将void指针转换为HttpResponseParser指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;

    // 匹配并设置HTTP响应版本
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        // 记录无效版本的警告日志
        WEBSERVER_LOG_WARN(g_logger) << "invalid http response version: " << std::string(at, length);
        // 设置解析错误码
        parser->setError(1001);
        return;
    }
    // 设置HTTP响应版本
    parser->getData()->setVersion(v);
}

/**
 * HTTP响应头解析完成时的回调（当前未实现）
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param at 指向数据开始位置的字符指针
 * @param length 数据的长度
 * 
 * 此函数当前未实现，用于在响应头解析完成时进行必要的操作。
 */
void on_response_header_done(void *data, const char *at, size_t length) {
}

/**
 * 处理HTTP响应的最后一个块（当前未实现）
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param at 指向块数据开始位置的字符指针
 * @param length 块数据的长度
 * 
 * 此函数当前未实现，用于处理HTTP响应的最后一个数据块。
 */
void on_response_last_chunk(void *data, const char *at, size_t length) {
}

/**
 * 处理HTTP响应头字段
 * 
 * @param data 指向HttpResponseParser对象的指针
 * @param field 指向字段名称开始位置的字符指针
 * @param flen 字段名称的长度
 * @param value 指向字段值开始位置的字符指针
 * @param vlen 字段值的长度
 * 
 * 此函数解析HTTP响应头中的字段及其值，并将它们设置到HttpResponseParser对象中。如果字段长度为0，
 * 会记录警告日志。
 */
void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    // 将void指针转换为HttpResponseParser指针
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);

    // 检查字段名称长度
    if(flen == 0) {
        // 记录字段长度为0的警告日志
        WEBSERVER_LOG_WARN(g_logger) << "invalid http response field length == 0";
        return;
    }
    // 设置HTTP响应头字段
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}


/**
 * HttpResponseParser构造函数
 * 
 * 此构造函数初始化HttpResponseParser对象，设置错误码为0，并创建一个新的HttpResponse对象。
 * 它还初始化HTTP客户端解析器并设置相关的回调函数，最后将解析器的数据上下文设置为当前对象。
 */
HttpResponseParser::HttpResponseParser()
    : m_error(0) { // 初始化错误码为0
    // 创建一个新的HttpResponse对象并将其赋值给m_data智能指针
    m_data.reset(new webserver::http::HttpResponse);
    // 初始化HTTP客户端解析器
    httpclient_parser_init(&m_parser);
    // 设置解析器的回调函数
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    // 将解析器的数据上下文设置为当前HttpResponseParser对象
    m_parser.data = this;
}

/**
 * 执行HTTP响应解析
 * 
 * @param data 指向数据缓冲区的指针
 * @param len 数据缓冲区中的数据长度
 * @param chunk 表示是否处理为块传输编码的标志
 * @return 解析过程中处理的字节数
 * 
 * 如果处理块传输编码的响应，会重新初始化解析器。此方法将数据提供给HTTP响应解析器并执行解析操作，
 * 解析完成后，会更新数据缓冲区，移除已解析的数据，并返回处理的字节数。
 */
size_t HttpResponseParser::execute(char* data, size_t len, bool chunk) {
    // 如果处理块编码，重新初始化解析器
    if(chunk) {
        httpclient_parser_init(&m_parser);
    }
    // 执行HTTP响应解析，并返回处理的字节数
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    // 移动未处理的数据至缓冲区头部
    memmove(data, data + offset, (len - offset));
    return offset;
}

/**
 * 检查HTTP响应解析是否完成
 * 
 * @return 解析完成返回1，否则返回0。
 * 
 * 此方法检查HTTP响应解析器是否已完成对HTTP响应的解析。
 */
int HttpResponseParser::isFinished() {
    // 检查解析是否完成
    return httpclient_parser_finish(&m_parser);
}

/**
 * 检查解析过程中是否发生错误
 * 
 * @return 有错误返回非0值，无错误返回0。
 * 
 * 此方法检查在HTTP响应解析过程中是否发生了错误。
 */
int HttpResponseParser::hasError() {
    // 检查是否有错误发生
    return m_error || httpclient_parser_has_error(&m_parser);
}

/**
 * 获取Content-Length的值
 * 
 * @return 请求中"Content-Length"头字段的值，如果不存在则返回0。
 * 
 * 此方法用于获取HTTP响应头中"Content-Length"字段的值。
 */
uint64_t HttpResponseParser::getContentLength() {
    // 从HTTP响应中获取"Content-Length"字段的值
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

}
}

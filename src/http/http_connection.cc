#include "http_connection.h"
#include "http_parser.h"
#include "src/log.h"
#include "src/streams/zlib_stream.h"

namespace webserver {
namespace http {

/**
 * 全局日志对象，用于记录HttpConnection类的析构函数调用
 */
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * 将HttpResult对象转换为字符串表示形式
 * 返回值：
 *   - 返回HttpResult对象的字符串表示形式
 * 详细描述：
 *  - 使用stringstream构建HttpResult对象的字符串表示形式，包括result、error和response字段的信息。
 */
std::string HttpResult::toString() const {
    std::stringstream ss; // 创建stringstream对象
    ss << "[HttpResult result=" << result // 添加result字段信息
       << " error=" << error // 添加error字段信息
       << " response=" << (response ? response->toString() : "nullptr") // 添加response字段信息
       << "]";
    return ss.str(); // 返回HttpResult对象的字符串表示形式
}

/**
 * 构造函数
 * 参数：
 *   - sock: 指向Socket对象的智能指针
 *   - owner: 表示是否拥有Socket对象的所有权
 * 详细描述：
 *  - 调用基类SocketStream的构造函数，初始化SocketStream成员变量。
 */
HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) // 调用基类SocketStream的构造函数，初始化SocketStream成员变量
{
}

/**
 * 析构函数
 * 详细描述：
 *  - 记录HttpConnection对象的析构函数调用日志。
 */
HttpConnection::~HttpConnection() {
    WEBSERVER_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection"; // 记录HttpConnection对象的析构函数调用日志
}

/**
 * 接收HTTP响应
 * 返回值：
 *   - 返回指向HttpResponse对象的智能指针，表示接收到的HTTP响应
 * 详细描述：
 *  - 创建HttpResponseParser对象，用于解析HTTP响应。
 *  - 使用循环读取数据，直到解析完整个HTTP响应。
 *  - 解析过程中可能涉及到分块编码和压缩，需要进行相应处理。
 *  - 解析完成后，将解析结果封装成HttpResponse对象返回。
 */
HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser); // 创建HttpResponseParser对象，用于解析HTTP响应
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize(); // 获取HTTP请求缓冲区大小
    // 智能指针接管
    std::shared_ptr<char> buffer(
            new char[buff_size + 1], [](char* ptr){
                delete[] ptr;
            }); // 创建共享指针buffer，用于接收HTTP响应数据
    char* data = buffer.get(); // 获取buffer指针
    int offset = 0; // 偏移量初始化为0
    do {
        // 在offset后面接着读数据
        int len = read(data + offset, buff_size - offset); // 从套接字中读取数据
        if(len <= 0) { // 如果读取失败或者连接关闭
            close(); // 关闭连接
            return nullptr; // 返回空指针
        }
        // 当前已经读取的数据长度
        len += offset; // 计算读取的数据长度
        data[len] = '\0'; // 设置字符串结尾
        // 解析缓冲区data中的数据
        // execute会将data向前移动nparse个字节，nparse为已经成功解析的字节数
        size_t nparse = parser->execute(data, len, false); // 执行解析操作
        if(parser->hasError()) { // 如果解析过程出错
            close(); // 关闭连接
            return nullptr; // 返回空指针
        }
        // 此时data还剩下len - nparse个字节
        offset = len - nparse; // 更新偏移量
        // 缓冲区满了还没解析完
        if(offset == (int)buff_size) { // 如果偏移量达到缓冲区大小
            close(); // 关闭连接
            return nullptr; // 返回空指针
        }
        // 解析结束
        if(parser->isFinished()) { // 如果解析完成
            break; // 跳出循环
        }
    } while(true); // 循环直到解析完成
    // 这里返回引用
    auto& client_parser = parser->getParser(); // 获取解析器对象的引用
    // 是否为chunk
    std::string body; // 声明存储HTTP响应体的字符串
    if(client_parser.chunked) { // 如果使用分块编码
        // 缓冲区剩余数据
        int len = offset; // 初始化长度为偏移量
        do {
            bool begin = true; // 是否为开始标志
            do {
                // 继续读数据
                if(!begin || len == 0) { // 如果不是开始或者长度为0
                    int rt = read(data + len, buff_size - len); // 读取数据
                    if(rt <= 0) { // 如果读取失败或连接关闭
                        close(); // 关闭连接
                        return nullptr; // 返回空指针
                    }
                    // 更新缓冲区数据数量
                    len += rt; // 更新长度
                }
                // 再末尾添加
                data[len] = '\0'; // 设置字符串结尾
                // 解析报文，重新初始化parser
                size_t nparse = parser->execute(data, len, true); // 执行解析操作
                if(parser->hasError()) { // 如果解析过程出错
                    close(); // 关闭连接
                    return nullptr; // 返回空指针
                }
                len -= nparse; // 更新长度
                if(len == (int)buff_size) { // 如果长度等于缓冲区大小
                    close(); // 关闭连接
                    return nullptr; // 返回空指针
                }
                begin = false; // 设置为非开始
            } while(!parser->isFinished()); // 循环直到解析完成
            // body小于缓冲区的数据
            if(client_parser.content_len + 2 <= len) { // 如果内容长度小于等于长度
                // 将body数据加进来
                body.append(data, client_parser.content_len); // 添加数据到body中
                // 移动data
                memmove(data, data + client_parser.content_len + 2
                        , len - client_parser.content_len - 2); // 移动数据
                // 缓冲区剩余数据数
                len -= client_parser.content_len + 2; // 更新长度
            } else { // 如果内容长度大于长度
                // 将缓冲区全部数据加进来
                body.append(data, len); // 添加数据到body中
                // 还剩多少body数据
                int left = client_parser.content_len - len + 2; // 计算剩余长度
                while(left > 0) { // 循环直到剩余长度为0
                    // 读最多缓冲区大小的数据
                    int rt = read(data, left > (int)buff_size ? (int)buff_size : left); // 读取数据
                    if(rt <= 0) { // 如果读取失败或连接关闭
                        close(); // 关闭连接
                        return nullptr; // 返回空指针
                    }
                    // 加到body
                    body.append(data, rt); // 添加数据到body中
                    left -= rt; // 更新剩余长度
                }
                body.resize(body.size() - 2); // 调整body的大小
                len = 0; // 更新长度
            }
        } while(!client_parser.chunks_done); // 循环直到分块完成
    } else { // 如果不使用分块编码
        // 获得body的长度
        int64_t length = parser->getContentLength(); // 获取内容长度
        if(length > 0) { // 如果内容长度大于0
            body.resize(length); // 调整body的大小

            int len = 0; // 初始化长度为0
            // 如果长度比缓冲区剩余的还大，将缓冲区全部加进来
            if(length >= offset) { // 如果内容长度大于等于偏移量
                memcpy(&body[0], data, offset); // 复制数据到body中
                len = offset; // 更新长度
            } else { // 如果内容长度小于偏移量
                // 否则将取length
                memcpy(&body[0], data, length); // 复制数据到body中
                len = length; // 更新长度
            }
            length -= offset; // 更新长度
            // 缓冲区里的数据也不够，继续读取直到满足length
            if(length > 0) { // 如果剩余长度大于0
                if(readFixSize(&body[len], length) <= 0) { // 读取剩余数据
                    close(); // 关闭连接
                    return nullptr; // 返回空指针
                }
            }
        }
    }
    if(!body.empty()) { // 如果body不为空
        auto content_encoding = parser->getData()->getHeader("content-encoding"); // 获取内容编码类型
        WEBSERVER_LOG_DEBUG(g_logger) << "content_encoding: " << content_encoding
            << " size=" << body.size(); // 记录内容编码类型和body大小的日志
        if(strcasecmp(content_encoding.c_str(), "gzip") == 0) { // 如果是gzip压缩
            auto zs = ZlibStream::CreateGzip(false); // 创建gzip解压缩流
            zs->write(body.c_str(), body.size()); // 写入数据
            zs->flush(); // 刷新流
            zs->getResult().swap(body); // 交换结果
        } else if(strcasecmp(content_encoding.c_str(), "deflate") == 0) { // 如果是deflate压缩
            auto zs = ZlibStream::CreateDeflate(false); // 创建deflate解压缩流
            zs->write(body.c_str(), body.size()); // 写入数据
            zs->flush(); // 刷新流
            zs->getResult().swap(body); // 交换结果
        }
        parser->getData()->setBody(body); // 设置HTTP响应体
    }
    //返回解析完的HttpResponse
    return parser->getData(); // 返回解析得到的HTTP响应
}


/**
 * 发送HTTP请求
 * 参数：
 *   - rsp: 指向HttpRequest对象的智能指针，表示待发送的HTTP请求
 * 返回值：
 *   - 返回值表示发送结果，大于等于0表示成功发送的字节数，小于0表示发送失败
 * 详细描述：
 *  - 将HttpRequest对象转换为字符串形式，并通过套接字发送出去。
 */
int HttpConnection::sendRequest(HttpRequest::ptr rsp) {
    std::stringstream ss; // 创建stringstream对象
    ss << *rsp; // 将HttpRequest对象转换为字符串形式
    std::string data = ss.str(); // 获取字符串形式的HTTP请求
    return writeFixSize(data.c_str(), data.size()); // 发送HTTP请求
}

/**
 * 执行GET请求
 * 参数：
 *   - url: 目标URL
 *   - timeout_ms: 超时时间（毫秒）
 *   - headers: 请求头部信息
 *   - body: 请求体信息
 * 返回值：
 *   - 返回执行结果的HttpResult对象
 * 详细描述：
 *  - 根据URL创建Uri对象，并调用DoGet(Uri::ptr, ...)函数执行GET请求。
 */
HttpResult::ptr HttpConnection::DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    Uri::ptr uri = Uri::Create(url); // 创建Uri对象
    if(!uri) { // 如果Uri对象为空
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url); // 返回错误的HttpResult对象
    }
    return DoGet(uri, timeout_ms, headers, body); // 调用DoGet(Uri::ptr, ...)函数执行GET请求
}

// 类似DoGet，省略注释
HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

/**
 * 发送HTTP请求
 * 参数：
 *   - method: 请求方法
 *   - url: 目标URL
 *   - timeout_ms: 超时时间（毫秒）
 *   - headers: 请求头部信息
 *   - body: 请求体信息
 * 返回值：
 *   - 返回执行结果的HttpResult对象
 * 详细描述：
 *  - 根据URL创建HttpRequest对象，并设置请求的各种信息，然后调用DoRequest(HttpRequest::ptr, ...)函数发送HTTP请求。
 */
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    Uri::ptr uri = Uri::Create(url); // 创建Uri对象
    if(!uri) { // 如果Uri对象为空
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url); // 返回错误的HttpResult对象
    }
    return DoRequest(method, uri, timeout_ms, headers, body); // 调用DoRequest(HttpMethod, Uri::ptr, ...)函数发送HTTP请求
}

/**
 * 发送HTTP请求
 * 最后所有的请求都是由这个方法实现的
 * 
 * 
 * 参数：
 *   - method: 请求方法
 *   - uri: 目标URI
 *   - timeout_ms: 超时时间（毫秒）
 *   - headers: 请求头部信息
 *   - body: 请求体信息
 * 返回值：
 *   - 返回执行结果的HttpResult对象
 * 详细描述：
 *  - 创建HttpRequest对象，并设置请求的各种信息，然后调用DoRequest(HttpRequest::ptr, ...)函数发送HTTP请求。
 */
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    // 创建http请求报文
    HttpRequest::ptr req = std::make_shared<HttpRequest>(); // 创建HttpRequest对象
    // 设置path
    req->setPath(uri->getPath()); // 设置请求路径
    // 设置qurey
    req->setQuery(uri->getQuery()); // 设置查询参数
    // 设置fragment
    req->setFragment(uri->getFragment()); // 设置片段
    // 设置请求方法
    req->setMethod(method); // 设置请求方法
    // 是否有主机号
    bool has_host = false; // 是否包含Host字段
    for(auto& i : headers) { // 遍历请求头部信息
        if(strcasecmp(i.first.c_str(), "connection") == 0) { // 如果是Connection字段
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) { // 如果值为keep-alive
                req->setClose(false); // 设置保持连接
            }
            continue; // 继续下一次循环
        }
        // 看有没有设置host
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) { // 如果还没有Host字段且是Host字段
            has_host = !i.second.empty(); // 设置是否包含Host字段
        }
        // 设置header
        req->setHeader(i.first, i.second); // 设置请求头部信息
    }
    //若没有host，则用uri的host
    if(!has_host) { // 如果没有Host字段
        req->setHeader("Host", uri->getHost()); // 设置Host字段
    }
    // 设置body
    req->setBody(body); // 设置请求体信息
    return DoRequest(req, uri, timeout_ms); // 调用DoRequest(HttpRequest::ptr, ...)函数发送HTTP请求
}


/**
 * 执行HTTP请求
 * 参数：
 *   - req: 待发送的HttpRequest对象
 *   - uri: 目标URI
 *   - timeout_ms: 超时时间（毫秒）
 * 返回值：
 *   - 返回执行结果的HttpResult对象
 * 详细描述：
 *  - 根据URI的scheme判断是否为HTTPS，创建目标地址。然后创建TCP套接字，连接目标地址。
 *  - 如果连接失败，返回连接失败的HttpResult对象。
 *  - 发送请求，如果发送过程中被对端关闭连接，则返回发送关闭的HttpResult对象。
 *  - 如果发送出错，返回发送套接字错误的HttpResult对象。
 *  - 接收响应，如果接收超时，则返回超时的HttpResult对象。
 *  - 返回接收到的响应内容。
 */
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms) {
    bool is_ssl = uri->getScheme() == "https"; // 判断是否为HTTPS
    // 通过uri创建address
    Address::ptr addr = uri->createAddress(); // 创建目标地址
    if(!addr) { // 如果地址为空
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                , nullptr, "invalid host: " + uri->getHost()); // 返回无效主机的HttpResult对象
    }
    // 创建TCPsocket
    Socket::ptr sock = is_ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr); // 创建TCP套接字
    if(!sock) { // 如果套接字为空
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                        + " errno=" + std::to_string(errno)
                        + " errstr=" + std::string(strerror(errno))); // 返回创建套接字失败的HttpResult对象
    }
    // 发起请求连接
    if(!sock->connect(addr)) { // 连接目标地址
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString()); // 返回连接失败的HttpResult对象
    }
    // 设置接收超时时间
    sock->setRecvTimeout(timeout_ms); // 设置接收超时时间
    // 创建httpconnection
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock); // 创建HttpConnection对象
    // 发送请求报文
    int rt = conn->sendRequest(req); // 发送请求
    // 若为0，则表示远端关闭连接
    if(rt == 0) { // 如果发送被对端关闭
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString()); // 返回发送关闭的HttpResult对象
    }
    // 小于0，失败
    if(rt < 0) { // 如果发送出错
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno))); // 返回发送套接字错误的HttpResult对象
    }
    // 接收响应报文
    auto rsp = conn->recvResponse(); // 接收响应
    if(!rsp) { // 如果接收超时
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms)); // 返回接收超时的HttpResult对象
    }
    // 结果成功，返回响应报文
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok"); // 返回成功的HttpResult对象
}

/**
 * 创建HTTP连接池
 * 参数：
 *   - uri: 目标URI
 *   - vhost: 虚拟主机名
 *   - max_size: 最大连接数
 *   - max_alive_time: 最大保持时间
 *   - max_request: 最大请求数
 * 返回值：
 *   - 返回创建的HttpConnectionPool对象
 * 详细描述：
 *  - 根据URI创建HttpConnectionPool对象。
 */
HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string& uri
                                                   ,const std::string& vhost
                                                   ,uint32_t max_size
                                                   ,uint32_t max_alive_time
                                                   ,uint32_t max_request) {
    Uri::ptr turi = Uri::Create(uri); // 创建URI对象
    if(!turi) { // 如果URI对象为空
        WEBSERVER_LOG_ERROR(g_logger) << "invalid uri=" << uri; // 记录错误日志
    }
    return std::make_shared<HttpConnectionPool>(turi->getHost()
            , vhost, turi->getPort(), turi->getScheme() == "https"
            , max_size, max_alive_time, max_request); // 创建HttpConnectionPool对象
}

/**
 * 构造函数
 * 参数：
 *   - host: 目标主机名
 *   - vhost: 虚拟主机名
 *   - port: 目标端口号
 *   - is_https: 是否为HTTPS
 *   - max_size: 最大连接数
 *   - max_alive_time: 最大保持时间
 *   - max_request: 最大请求数
 * 详细描述：
 *  - 初始化各成员变量。
 */
HttpConnectionPool::HttpConnectionPool(const std::string& host
                                        ,const std::string& vhost
                                        ,uint32_t port
                                        ,bool is_https
                                        ,uint32_t max_size
                                        ,uint32_t max_alive_time
                                        ,uint32_t max_request)
    :m_host(host)
    ,m_vhost(vhost)
    ,m_port(port ? port : (is_https ? 443 : 80))
    ,m_maxSize(max_size)
    ,m_maxAliveTime(max_alive_time)
    ,m_maxRequest(max_request)
    ,m_isHttps(is_https) {
}

/**
 * 从连接池中获取HTTP连接
 * 返回值：
 *   - 返回一个智能指针，指向获取的HttpConnection对象
 * 详细描述：
 *  - 遍历连接池中的HttpConnection对象，选择合适的连接。如果找到可用的连接，则从连接池中移除该连接并返回；如果没有可用的连接，则创建一个新的连接。
 */
HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = webserver::GetCurrentMS(); // 获取当前时间（毫秒）
    // 非法的连接
    std::vector<HttpConnection*> invalid_conns; // 无效的连接列表
    HttpConnection* ptr = nullptr; // 指向选定的连接
    MutexType::Lock lock(m_mutex); // 加锁
    // 若连接池不为空
    while(!m_conns.empty()) { // 遍历连接池中的连接
        // 取出第一个connection
        auto conn = *m_conns.begin(); // 获取连接池中的第一个连接
        // 弹掉
        m_conns.pop_front(); // 移除连接池中的第一个连接
        // 不在连接状态，放入非法vec中
        if(!conn->isConnected()) { // 如果连接已断开
            invalid_conns.push_back(conn); // 将连接添加到无效连接列表中
            continue; // 继续下一次循环
        }
        // 已经超过了最大连接时间，放入非法vec中
        if((conn->m_createTime + m_maxAliveTime) > now_ms) { // 如果连接的创建时间超过了最大存活时间
            invalid_conns.push_back(conn); // 将连接添加到无效连接列表中
            continue; // 继续下一次循环
        }
        // 获得当前connection
        ptr = conn; // 找到可用的连接
        break; // 退出循环
    }
    lock.unlock(); // 解锁
    for(auto i : invalid_conns) { // 销毁无效的连接
        delete i;
    }
    m_total -= invalid_conns.size(); // 更新连接池中连接的数量

    if(!ptr) { // 如果没有可用的连接
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host); // 获取目标主机的IP地址
        if(!addr) { // 如果获取失败
            WEBSERVER_LOG_ERROR(g_logger) << "get addr fail: " << m_host; // 记录日志
            return nullptr; // 返回空指针
        }
        addr->setPort(m_port); // 设置端口号
        Socket::ptr sock = m_isHttps ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr); // 创建套接字
        if(!sock) { // 如果创建套接字失败
            WEBSERVER_LOG_ERROR(g_logger) << "create sock fail: " << *addr; // 记录日志
            return nullptr; // 返回空指针
        }
        if(!sock->connect(addr)) { // 如果连接目标地址失败
            WEBSERVER_LOG_ERROR(g_logger) << "sock connect fail: " << *addr; // 记录日志
            return nullptr; // 返回空指针
        }

        ptr = new HttpConnection(sock); // 创建新的HttpConnection对象
        ++m_total; // 更新连接池中连接的数量
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr
                               , std::placeholders::_1, this)); // 返回指向HttpConnection对象的智能指针
}

/**
 * 释放HttpConnection指针的回调函数
 * 参数：
 *   - ptr: 指向HttpConnection对象的指针
 *   - pool: 指向当前HttpConnectionPool对象的指针
 * 详细描述：
 *  - 根据HttpConnection的状态判断是否需要销毁该连接。如果连接已断开、超过了最大存活时间或达到了最大请求次数，则销毁该连接；否则将连接放回连接池。
 */
void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request; // 增加连接的请求数量
    // 已经关闭了链接，超时，超过最大请求数量
    if(!ptr->isConnected() // 如果连接已断开
            || ((ptr->m_createTime + pool->m_maxAliveTime) >= webserver::GetCurrentMS()) // 或者连接已经超过了最大存活时间
            || (ptr->m_request >= pool->m_maxRequest)) { // 或者连接已达到最大请求次数
        delete ptr; // 销毁连接
        --pool->m_total; // 更新连接池中连接的数量
        return;
    }
    MutexType::Lock lock(pool->m_mutex); // 加锁
    // 重新放入连接池中
    pool->m_conns.push_back(ptr); // 将连接放回连接池
}

// 类似doGet，省略注释


/**
 * 执行HTTP的GET请求
 * 参数：
 *   - url: 请求的URL地址
 *   - timeout_ms: 请求超时时间（毫秒）
 *   - headers: 请求的头部信息
 *   - body: 请求的消息体
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doGet(const std::string& url
                          , uint64_t timeout_ms
                          , const std::map<std::string, std::string>& headers
                          , const std::string& body) {
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

/**
 * 执行HTTP的GET请求（使用Uri对象）
 * 参数：
 *   - uri: 请求的Uri对象
 *   - timeout_ms: 请求超时时间（毫秒）
 *   - headers: 请求的头部信息
 *   - body: 请求的消息体
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string>& headers
                                   , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

/**
 * 执行HTTP的POST请求
 * 参数：
 *   - url: 请求的URL地址
 *   - timeout_ms: 请求超时时间（毫秒）
 *   - headers: 请求的头部信息
 *   - body: 请求的消息体
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doPost(const std::string& url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string>& headers
                                   , const std::string& body) {
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

/**
 * 执行HTTP的POST请求（使用Uri对象）
 * 参数：
 *   - uri: 请求的Uri对象
 *   - timeout_ms: 请求超时时间（毫秒）
 *   - headers: 请求的头部信息
 *   - body: 请求的消息体
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string>& headers
                                   , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}


/**
 * 执行HTTP请求
 * 参数：
 *   - method: HTTP请求方法，GET、POST等
 *   - url: 请求的URL地址
 *   - timeout_ms: 请求超时时间（毫秒）
 *   - headers: 请求的头部信息
 *   - body: 请求的消息体
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                    , const std::string& url
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body) {
    // 创建一个HttpRequest对象
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    // 设置请求的路径
    req->setPath(url);
    // 设置请求方法
    req->setMethod(method);
    // 默认不关闭连接
    req->setClose(false);
    bool has_host = false;
    // 遍历headers，设置请求头部信息
    for(auto& i : headers) {
        // 如果请求头部包含"connection"字段
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            // 如果值为"keep-alive"，则设置为不关闭连接
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        // 如果请求头部中未包含"host"字段，则标记为未设置host
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        // 设置请求头部信息
        req->setHeader(i.first, i.second);
    }
    // 如果请求头部未设置host字段，则根据是否设置虚拟主机信息来设置host
    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    // 设置请求消息体
    req->setBody(body);
    // 调用重载的doRequest函数执行请求
    return doRequest(req, timeout_ms);
}

/**
 * 执行HTTP请求（使用Uri对象）
 * 参数：
 *   - method: HTTP请求方法，GET、POST等
 *   - uri: 请求的Uri对象
 *   - timeout_ms: 请求超时时间（毫秒）
 *   - headers: 请求的头部信息
 *   - body: 请求的消息体
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                    , Uri::ptr uri
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers
                                    , const std::string& body) {
    // 构造请求URL
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    // 调用doRequest执行请求
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

/**
 * 执行HTTP请求
 * 参数：
 *   - req: HTTP请求对象
 *   - timeout_ms: 请求超时时间（毫秒）
 * 返回值：
 *   - 返回一个智能指针，指向HttpResult对象，表示请求的结果
 */
HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
                                        , uint64_t timeout_ms) {
    // 获取连接
    auto conn = getConnection();
    if(!conn) {
        // 如果获取连接失败，则返回错误结果
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    auto sock = conn->getSocket();
    if(!sock) {
        // 如果获取到的套接字为空，则返回错误结果
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    // 设置套接字接收超时时间
    sock->setRecvTimeout(timeout_ms);
    // 发送请求
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        // 如果发送请求失败（返回0），则返回错误结果
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if(rt < 0) {
        // 如果发送请求出错，则返回错误结果
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno)));
    }
    // 接收响应
    auto rsp = conn->recvResponse();
    if(!rsp) {
        // 如果接收响应超时，则返回超时错误结果
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));
    }
    // 返回成功结果
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

}
}

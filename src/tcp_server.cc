#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace webserver {

/**
 * 定义一个配置变量来表示TCP服务器的读取超时时间。
 * 
 * webserver::ConfigVar<uint64_t>::ptr 是一个指向配置变量的智能指针，
 * 该变量用于存储和访问配置信息。g_tcp_server_read_timeout 是该配置变量的名称，
 * 它用于设置和检索TCP服务器读取操作的超时时间。
 * 
 * 'webserver::Config::Lookup' 函数用于查找或创建一个名为"tcp_server.read_timeout"的配置变量。
 * 如果变量不存在，它将创建该变量并设置默认值为120000毫秒（即2分钟）。
 * "tcp server read timeout" 是关于这个配置变量的描述信息。
 */
static webserver::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    webserver::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");

/**
 * 定义一个日志器，用于记录系统相关的日志信息。
 * 
 * webserver::Logger::ptr 是一个指向Logger对象的智能指针。g_logger 是这个日志器的名称，
 * 用于在程序的不同部分中记录日志。WEBSERVER_LOG_NAME("system") 是一个宏调用，
 * 它创建或获取一个名为"system"的日志器。这样，通过g_logger 可以方便地在程序的任何位置记录系统相关的日志。
 */
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");


/**
 * @brief TcpServer构造函数。
 * 
 * 初始化TcpServer实例，设置工作线程、IO工作线程、接受连接的工作线程，
 * 以及根据全局配置设置的接收超时时间。服务器默认名称被设置，并将停止标志设为真。
 * 
 * @param worker 负责处理工作任务的IO管理器。
 * @param io_worker 负责处理IO任务的IO管理器。
 * @param accept_worker 负责接受新连接的IO管理器。
 */
TcpServer::TcpServer(webserver::IOManager* worker, webserver::IOManager* io_worker, webserver::IOManager* accept_worker)
    : m_worker(worker)  // 初始化工作IOManager。
    , m_ioWorker(io_worker)  // 初始化IO工作IOManager。
    , m_acceptWorker(accept_worker)  // 初始化接受连接的IOManager。
    , m_recvTimeout(g_tcp_server_read_timeout->getValue())  // 从配置获取读取超时设置。
    , m_name("webserver/1.0.0")  // 设置服务器默认名称。
    , m_isStop(true) {  // 初始化停止标志为真。
}

/**
 * @brief TcpServer析构函数。
 * 
 * 当TcpServer实例被销毁时，关闭所有socket连接，并清空socket列表。
 */
TcpServer::~TcpServer() {
    for(auto& i : m_socks) {  // 遍历所有socket。
        i->close();  // 关闭socket连接。
    }
    m_socks.clear();  // 清空socket列表。
}

/**
 * @brief 设置TcpServer配置。
 * 
 * 接收一个TcpServerConf配置对象，并将其深拷贝到成员变量m_conf中以更新服务器配置。
 * 
 * @param v TcpServer配置对象。
 */
void TcpServer::setConf(const TcpServerConf& v) {
    m_conf.reset(new TcpServerConf(v));  // 使用新配置对象更新m_conf。
}

/**
 * @brief 尝试绑定到一个单一地址。
 * 
 * 创建一个新的socket并尝试将其绑定到提供的地址上。如果ssl为true，则创建一个SSL socket。
 * 
 * @param addr 要绑定的地址。
 * @param ssl 是否使用SSL加密。
 * @return 绑定成功返回true，失败返回false。
 */
bool TcpServer::bind(webserver::Address::ptr addr, bool ssl) {
    std::vector<Address::ptr> addrs;  // 创建一个地址向量。
    std::vector<Address::ptr> fails;  // 创建一个用于存储绑定失败地址的向量。
    addrs.push_back(addr);  // 将地址添加到地址向量中。
    return bind(addrs, fails, ssl);  // 调用bind的另一个重载版本。
}

/**
 * @brief 尝试绑定到一组地址。
 * 
 * 遍历地址列表，为每个地址创建一个新socket并尝试绑定和监听。绑定或监听失败的地址会被记录并添加到fails列表中。
 * 
 * @param addrs 要绑定的地址列表。
 * @param fails 绑定失败的地址列表。
 * @param ssl 是否使用SSL加密。
 * @return 所有地址绑定成功返回true，任一失败返回false。
 */
bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl) {
    m_ssl = ssl;  // 设置SSL标志。
    for (auto& addr : addrs) {  // 遍历所有地址。
        // 根据ssl标志创建TCP或SSL socket。
        Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        if (!sock->bind(addr)) {  // 尝试绑定地址。
            // 记录绑定失败日志。
            WEBSERVER_LOG_ERROR(g_logger) << "bind fail errno=" << errno << " errstr=" << strerror(errno)
                                          << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);  // 添加到失败列表。
            continue;
        }
        if (!sock->listen()) {  // 尝试监听地址。
            // 记录监听失败日志。
            WEBSERVER_LOG_ERROR(g_logger) << "listen fail errno=" << errno << " errstr=" << strerror(errno)
                                          << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);  // 添加到失败列表。
            continue;
        }
        m_socks.push_back(sock);  // 监听成功，添加到socket列表。
    }

    // 检查是否有失败的绑定。
    if (!fails.empty()) {
        m_socks.clear();  // 清空socket列表并返回失败。
        return false;
    }

    // 所有地址绑定成功，记录成功日志。
    for (auto& i : m_socks) {
        WEBSERVER_LOG_INFO(g_logger) << "type=" << m_type << " name=" << m_name
                                     << " ssl=" << m_ssl << " server bind success: " << *i;
    }
    return true;
}


/**
 * @brief 开始接受客户端连接，该函数在服务器启动后运行，不断接受客户端连接并将其加入IO工作线程池处理
 * 
 * @param sock 监听套接字
 */
void TcpServer::startAccept(Socket::ptr sock) {
    // 循环接受客户端连接，直到服务器停止运行
    while(!m_isStop) {
        // 接受客户端连接
        Socket::ptr client = sock->accept();
        if(client) {
            // 设置客户端接收超时时间
            client->setRecvTimeout(m_recvTimeout);
            // 将客户端处理任务加入到IO工作线程池中
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient,
                        shared_from_this(), client));
        } else {
            // 打印错误日志
            WEBSERVER_LOG_ERROR(g_logger) << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}

/**
 * @brief 启动服务器，该函数负责启动服务器并分配接受客户端连接任务给监听套接字所在的工作线程池
 * 
 * @return true 启动成功
 * @return false 启动失败
 */
bool TcpServer::start() {
    // 如果服务器未停止，直接返回true
    if(!m_isStop) {
        return true;
    }
    // 标记服务器开始运行
    m_isStop = false;
    // 遍历服务器监听套接字，为每一个套接字分配一个接受客户端连接的任务
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(), sock));
    }
    return true;
}

/**
 * @brief 停止服务器，该函数负责停止服务器并关闭所有监听套接字
 * 
 */
void TcpServer::stop() {
    // 停止服务器
    m_isStop = true;
    // 获取当前对象的shared_ptr
    auto self = shared_from_this();
    // 在接受连接的工作线程中执行关闭套接字的操作
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            // 取消所有未完成的IO操作
            sock->cancelAll();
            // 关闭套接字
            sock->close();
        }
        // 清空套接字列表
        m_socks.clear();
    });
}

/**
 * @brief 处理客户端连接，该函数负责处理客户端连接，并记录处理日志
 * 
 * @param client 客户端套接字
 */
void TcpServer::handleClient(Socket::ptr client) {
    // 打印客户端处理日志
    WEBSERVER_LOG_INFO(g_logger) << "handleClient: " << *client;
}

/**
 * @brief 加载服务器证书，该函数负责加载服务器监听套接字的证书
 * 
 * @param cert_file 证书文件路径
 * @param key_file 私钥文件路径
 * @return true 加载成功
 * @return false 加载失败
 */
bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file) {
    // 加载证书
    for(auto& i : m_socks) {
        auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
        if(ssl_socket) {
            if(!ssl_socket->loadCertificates(cert_file, key_file)) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 获取服务器信息的字符串表示，该函数返回服务器的各项配置信息字符串表示
 * 
 * @param prefix 前缀字符串
 * @return std::string 服务器信息的字符串表示
 */
std::string TcpServer::toString(const std::string& prefix) {
    // 格式化服务器信息
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name << " ssl=" << m_ssl
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    // 将监听套接字信息加入到字符串流中
    for(auto& i : m_socks) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}


}

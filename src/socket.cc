#include "socket.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include <limits.h>

namespace webserver {

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * 创建TCP套接字对象，根据指定地址创建相应类型的套接字。
 *
 * @param address 指定的地址对象指针
 * @return 返回创建的TCP套接字对象指针
 */
Socket::ptr Socket::CreateTCP(webserver::Address::ptr address) {
    // 根据地址对象的地址族创建相应类型的TCP套接字
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

/**
 * 创建UDP套接字对象，根据指定地址创建相应类型的套接字，并进行初始化。
 *
 * @param address 指定的地址对象指针
 * @return 返回创建并初始化的UDP套接字对象指针
 */
Socket::ptr Socket::CreateUDP(webserver::Address::ptr address) {
    // 根据地址对象的地址族创建相应类型的UDP套接字
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    // 初始化UDP套接字
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * 创建IPv4 TCP套接字对象。
 *
 * @return 返回创建的IPv4 TCP套接字对象指针
 */
Socket::ptr Socket::CreateTCPSocket() {
    // 使用IPv4地址族创建TCP套接字
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

/**
 * 创建IPv4 UDP套接字对象，并进行初始化。
 *
 * @return 返回创建并初始化的IPv4 UDP套接字对象指针
 */
Socket::ptr Socket::CreateUDPSocket() {
    // 使用IPv4地址族创建UDP套接字
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    // 初始化UDP套接字
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * 创建IPv6 TCP套接字对象。
 *
 * @return 返回创建的IPv6 TCP套接字对象指针
 */
Socket::ptr Socket::CreateTCPSocket6() {
    // 使用IPv6地址族创建TCP套接字
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

/**
 * 创建IPv6 UDP套接字对象，并进行初始化。
 *
 * @return 返回创建并初始化的IPv6 UDP套接字对象指针
 */
Socket::ptr Socket::CreateUDPSocket6() {
    // 使用IPv6地址族创建UDP套接字
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    // 初始化UDP套接字
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

/**
 * 创建Unix域TCP套接字对象。
 *
 * @return 返回创建的Unix域TCP套接字对象指针
 */
Socket::ptr Socket::CreateUnixTCPSocket() {
    // 使用Unix域地址族创建TCP套接字
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

/**
 * 创建Unix域UDP套接字对象，并进行初始化。
 *
 * @return 返回创建并初始化的Unix域UDP套接字对象指针
 */
Socket::ptr Socket::CreateUnixUDPSocket() {
    // 使用Unix域地址族创建UDP套接字
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    // 初始化UDP套接字
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}


/**
 * Socket类的构造函数，用于初始化Socket对象。
 *
 * @param family   地址族 (AF_INET、AF_INET6等)
 * @param type     套接字类型 (SOCK_STREAM、SOCK_DGRAM等)
 * @param protocol 协议类型 (IPPROTO_TCP、IPPROTO_UDP等)
 */
Socket::Socket(int family, int type, int protocol)
    : m_sock(-1)        // 初始化套接字描述符为-1
    , m_family(family)  // 设置地址族
    , m_type(type)      // 设置套接字类型
    , m_protocol(protocol)  // 设置协议类型
    , m_isConnected(false)  // 初始化连接状态为未连接
{
    // 构造函数体为空，因为实际的Socket对象创建和连接操作将在之后的代码中完成
}

/**
 * Socket类的析构函数，用于关闭套接字连接。
 * 调用close()函数关闭套接字。
 */
Socket::~Socket() {
    close();  // 关闭套接字连接
}


/**
 * 获取发送超时时间。
 *
 * @return 发送超时时间，以毫秒为单位；如果获取失败或未设置超时时间，则返回-1。
 */
int64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);  // 获取套接字描述符的上下文信息
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);  // 返回发送超时时间
    }
    return -1;  // 获取失败时返回-1
}

/**
 * 设置发送超时时间。
 *
 * @param v 超时时间，以毫秒为单位。
 */
void Socket::setSendTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};  // 将毫秒转换为timeval结构体
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);  // 设置发送超时时间
}

/**
 * 获取接收超时时间。
 *
 * @return 接收超时时间，以毫秒为单位；如果获取失败或未设置超时时间，则返回-1。
 */
int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);  // 获取套接字描述符的上下文信息
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);  // 返回接收超时时间
    }
    return -1;  // 获取失败时返回-1
}

/**
 * 设置接收超时时间。
 *
 * @param v 超时时间，以毫秒为单位。
 */
void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};  // 将毫秒转换为timeval结构体
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);  // 设置接收超时时间
}


/**
 * 获取套接字选项的值。
 *
 * @param level  选项级别
 * @param option 选项名称
 * @param result 存储选项值的缓冲区指针
 * @param len    缓冲区长度指针
 * @return 如果获取成功，则返回true；否则返回false。
 */
bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
    int rt = getsockopt(m_sock, level, option, result, len);  // 调用getsockopt获取选项值
    if(rt) {
        // 获取失败时记录日志并返回false
        WEBSERVER_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;  // 获取成功时返回true
}

/**
 * 设置套接字选项的值。
 *
 * @param level  选项级别
 * @param option 选项名称
 * @param result 选项值的指针
 * @param len    选项值的长度
 * @return 如果设置成功，则返回true；否则返回false。
 */
bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
    if(setsockopt(m_sock, level, option, result, len)) {  // 调用setsockopt设置选项值
        // 设置失败时记录日志并返回false
        WEBSERVER_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;  // 设置成功时返回true
}


/**
 * 接受新的连接并返回对应的Socket对象。
 *
 * @return 成功接受连接并初始化Socket对象时返回Socket::ptr；否则返回nullptr。
 */
Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));  // 创建新的Socket对象
    int newsock = ::accept(m_sock, nullptr, nullptr);  // 调用accept函数接受新的连接
    if(newsock == -1) {
        // 如果接受连接失败，记录错误日志并返回nullptr
        WEBSERVER_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
            << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    if(sock->init(newsock)) {  // 初始化新的Socket对象
        return sock;  // 返回成功初始化的Socket对象
    }
    return nullptr;  // 初始化失败时返回nullptr
}

/**
 * 初始化Socket对象。
 *
 * @param sock 新的套接字描述符
 * @return 如果成功初始化Socket对象，则返回true；否则返回false。
 */
bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);  // 获取套接字描述符的上下文信息
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        // 如果上下文信息有效且套接字未关闭，则初始化Socket对象
        m_sock = sock;  // 设置套接字描述符
        m_isConnected = true;  // 设置连接状态为已连接
        initSock();  // 初始化套接字
        getLocalAddress();  // 获取本地地址信息
        getRemoteAddress();  // 获取远程地址信息
        return true;  // 返回成功初始化的标志
    }
    return false;  // 初始化失败时返回false
}

/**
 * 将Socket对象绑定到指定的地址。
 *
 * @param addr 要绑定的地址对象指针
 * @return 如果绑定成功，则返回true；否则返回false。
 */
bool Socket::bind(const Address::ptr addr) {
    // 如果套接字无效，则创建新的套接字
    if (!isValid()) {
        newSock();
        // 如果创建套接字失败，则返回false
        if (WEBSERVER_UNLIKELY(!isValid())) {
            return false;
        }
    }

    // 检查要绑定的地址家族与套接字家族是否一致
    if (WEBSERVER_UNLIKELY(addr->getFamily() != m_family)) {
        WEBSERVER_LOG_ERROR(g_logger) << "bind sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;
    }

    // 如果是Unix地址，则创建临时Unix套接字并连接，连接成功则说明地址已被使用，需要先解绑
    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if (uaddr) {
        Socket::ptr sock = Socket::CreateUnixTCPSocket();
        if (sock->connect(uaddr)) {
            return false;  // 连接成功说明地址已被使用
        } else {
            webserver::FSUtil::Unlink(uaddr->getPath(), true);  // 解绑地址
        }
    }

    // 调用系统的bind函数绑定套接字和地址
    if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        WEBSERVER_LOG_ERROR(g_logger) << "bind error errno=" << errno
            << " errstr=" << strerror(errno);
        return false;  // 绑定失败则返回false
    }

    // 绑定成功后获取本地地址信息并返回true
    getLocalAddress();
    return true;
}


/**
 * 重新连接到远程地址。
 *
 * @param timeout_ms 连接超时时间，以毫秒为单位
 * @return 如果重新连接成功，则返回true；否则返回false。
 */
bool Socket::reconnect(uint64_t timeout_ms) {
    if (!m_remoteAddress) {
        // 如果远程地址为空，则记录错误日志并返回false
        WEBSERVER_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null";
        return false;
    }
    m_localAddress.reset();  // 重置本地地址信息
    return connect(m_remoteAddress, timeout_ms);  // 调用connect函数重新连接
}

/**
 * 连接到指定地址。
 *
 * @param addr       要连接的地址对象指针
 * @param timeout_ms 连接超时时间，以毫秒为单位
 * @return 如果连接成功，则返回true；否则返回false。
 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;  // 设置远程地址
    if (!isValid()) {
        newSock();  // 如果套接字无效，则创建新的套接字
        if (WEBSERVER_UNLIKELY(!isValid())) {
            return false;  // 如果创建套接字失败，则返回false
        }
    }

    // 检查要连接的地址家族与套接字家族是否一致
    if (WEBSERVER_UNLIKELY(addr->getFamily() != m_family)) {
        WEBSERVER_LOG_ERROR(g_logger) << "connect sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;  // 不一致则返回false
    }

    // 根据超时时间选择调用不同的连接函数
    if (timeout_ms == (uint64_t)-1) {
        if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            // 连接失败时记录错误日志并关闭套接字
            WEBSERVER_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            // 连接超时时记录错误日志并关闭套接字
            WEBSERVER_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                << ") timeout=" << timeout_ms << " error errno="
                << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }

    // 连接成功后设置连接状态，并获取远程地址和本地地址信息
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;  // 返回连接成功标志
}


/**
 * 监听套接字连接请求。
 *
 * @param backlog 最大等待连接队列的长度
 * @return 如果监听成功，则返回true；否则返回false。
 */
bool Socket::listen(int backlog) {
    if (!isValid()) {
        // 如果套接字无效，则记录错误日志并返回false
        WEBSERVER_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    if (::listen(m_sock, backlog)) {
        // 调用系统的listen函数监听套接字，如果失败则记录错误日志并返回false
        WEBSERVER_LOG_ERROR(g_logger) << "listen error errno=" << errno
            << " errstr=" << strerror(errno);
        return false;
    }
    return true;  // 监听成功则返回true
}

/**
 * 关闭套接字连接。
 *
 * @return 如果成功关闭套接字，则返回false；否则返回true。
 */
bool Socket::close() {
    // 如果套接字未连接且描述符为-1，则直接返回true
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    // 更新连接状态为未连接
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);// 调用系统的close函数关闭套接字
        m_sock = -1;// 重置描述符为-1
    }
    return false;
}

/**
 * 发送数据到连接的对端。
 *
 * @param buffer 要发送的数据缓冲区指针
 * @param length 要发送的数据长度
 * @param flags  发送操作的标志
 * @return 如果发送成功，则返回发送的字节数；否则返回-1。
 */
int Socket::send(const void* buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::send(m_sock, buffer, length, flags);  // 调用系统的send函数发送数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 发送数据到连接的对端，支持多个数据缓冲区。
 *
 * @param buffers  要发送的数据缓冲区数组指针
 * @param length   要发送的数据缓冲区个数
 * @param flags    发送操作的标志
 * @return 如果发送成功，则返回发送的字节数；否则返回-1。
 */
int Socket::send(const iovec* buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);  // 调用系统的sendmsg函数发送数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 发送数据到指定地址。
 *
 * @param buffer 要发送的数据缓冲区指针
 * @param length 要发送的数据长度
 * @param to     目标地址对象指针
 * @param flags  发送操作的标志
 * @return 如果发送成功，则返回发送的字节数；否则返回-1。
 */
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if (isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());  // 调用系统的sendto函数发送数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 发送数据到指定地址，支持多个数据缓冲区。
 *
 * @param buffers 要发送的数据缓冲区数组指针
 * @param length  要发送的数据缓冲区个数
 * @param to      目标地址对象指针
 * @param flags   发送操作的标志
 * @return 如果发送成功，则返回发送的字节数；否则返回-1。
 */
int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);  // 调用系统的sendmsg函数发送数据
    }
    return -1;  // 如果未连接，则返回-1
}


/**
 * 接收数据从连接的对端。
 *
 * @param buffer 要接收数据的缓冲区指针
 * @param length 要接收数据的长度
 * @param flags  接收操作的标志
 * @return 如果接收成功，则返回接收到的字节数；否则返回-1。
 */
int Socket::recv(void* buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::recv(m_sock, buffer, length, flags);  // 调用系统的recv函数接收数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 接收数据从连接的对端，支持多个数据缓冲区。
 *
 * @param buffers 要接收数据的缓冲区数组指针
 * @param length  要接收数据的缓冲区个数
 * @param flags   接收操作的标志
 * @return 如果接收成功，则返回接收到的字节数；否则返回-1。
 */
int Socket::recv(iovec* buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);  // 调用系统的recvmsg函数接收数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 接收数据从指定地址。
 *
 * @param buffer 要接收数据的缓冲区指针
 * @param length 要接收数据的长度
 * @param from   源地址对象指针，用于存储接收数据的来源地址
 * @param flags  接收操作的标志
 * @return 如果接收成功，则返回接收到的字节数；否则返回-1。
 */
int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if (isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);  // 调用系统的recvfrom函数接收数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 接收数据从指定地址，支持多个数据缓冲区。
 *
 * @param buffers 要接收数据的缓冲区数组指针
 * @param length  要接收数据的缓冲区个数
 * @param from    源地址对象指针，用于存储接收数据的来源地址
 * @param flags   接收操作的标志
 * @return 如果接收成功，则返回接收到的字节数；否则返回-1。
 */
int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);  // 调用系统的recvmsg函数接收数据
    }
    return -1;  // 如果未连接，则返回-1
}

/**
 * 获取连接的远程地址。
 *
 * @return 如果成功获取远程地址，则返回远程地址对象指针；否则返回空指针。
 */
Address::ptr Socket::getRemoteAddress() {
    if (m_remoteAddress) {
        return m_remoteAddress;  // 如果已经获取过远程地址，则直接返回远程地址对象指针
    }

    Address::ptr result;
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());  // 创建IPv4地址对象
            break;
        case AF_INET6:
            result.reset(new IPv6Address());  // 创建IPv6地址对象
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());  // 创建Unix地址对象
            break;
        default:
            result.reset(new UnknownAddress(m_family));  // 创建未知地址对象
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if (getpeername(m_sock, result->getAddr(), &addrlen)) {
        // 获取远程地址失败时返回未知地址对象
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;  // 缓存远程地址对象
    return m_remoteAddress;
}

/**
 * 获取套接字的本地地址。
 *
 * @return 如果成功获取本地地址，则返回本地地址对象指针；否则返回空指针。
 */
Address::ptr Socket::getLocalAddress() {
    if (m_localAddress) {
        return m_localAddress;  // 如果已经获取过本地地址，则直接返回本地地址对象指针
    }

    Address::ptr result;
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());  // 创建IPv4地址对象
            break;
        case AF_INET6:
            result.reset(new IPv6Address());  // 创建IPv6地址对象
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());  // 创建Unix地址对象
            break;
        default:
            result.reset(new UnknownAddress(m_family));  // 创建未知地址对象
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if (getsockname(m_sock, result->getAddr(), &addrlen)) {
        // 获取本地地址失败时记录错误日志并返回未知地址对象
        WEBSERVER_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if (m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;  // 缓存本地地址对象
    return m_localAddress;
}


/**
 * 检查套接字是否有效。
 *
 * @return 如果套接字描述符不为-1，则返回true；否则返回false。
 */
bool Socket::isValid() const {
    return m_sock != -1;  // 如果套接字描述符不为-1，则表示套接字有效
}

/**
 * 获取套接字的错误状态。
 *
 * @return 如果成功获取错误状态，则返回错误码；否则返回errno值。
 */
int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;  // 如果获取错误状态失败，则将errno值作为错误码
    }
    return error;
}


/**
 * 将Socket对象的信息输出到输出流中。
 *
 * @param os 输出流对象的引用
 * @return 输出流对象的引用
 */
std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if (m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if (m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

/**
 * 将Socket对象的信息转换为字符串。
 *
 * @return 包含Socket对象信息的字符串
 */
std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);  // 调用dump函数将信息输出到字符串流中
    return ss.str();  // 返回字符串流中的内容
}


/**
 * 取消套接字的读事件监听。
 *
 * @return 如果成功取消读事件监听，则返回true；否则返回false。
 */
bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, webserver::IOManager::READ);  // 调用IOManager的cancelEvent函数取消读事件监听
}

/**
 * 取消套接字的写事件监听。
 *
 * @return 如果成功取消写事件监听，则返回true；否则返回false。
 */
bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, webserver::IOManager::WRITE);  // 调用IOManager的cancelEvent函数取消写事件监听
}

/**
 * 取消套接字的接收事件监听。
 *
 * @return 如果成功取消接收事件监听，则返回true；否则返回false。
 */
bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, webserver::IOManager::READ);  // 调用IOManager的cancelEvent函数取消接收事件监听
}

/**
 * 取消套接字的所有事件监听。
 *
 * @return 如果成功取消所有事件监听，则返回true；否则返回false。
 */
bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sock);  // 调用IOManager的cancelAll函数取消所有事件监听
}

/**
 * 初始化套接字选项。
 */
void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));  // 设置SO_REUSEADDR选项
    if (m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));  // 设置TCP_NODELAY选项
    }
}

/**
 * 创建新的套接字并初始化。
 */
void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);  // 创建新的套接字
    if (WEBSERVER_LIKELY(m_sock != -1)) {
        initSock();  // 初始化套接字选项
    } else {
        WEBSERVER_LOG_ERROR(g_logger) << "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " errstr=" << strerror(errno);
    }
}

/*
这段代码定义了一个匿名命名空间，在其中声明了一个名为_SSLInit的结构体，并定义了一个名为s_init的静态变量。
在_SSLInit结构体的构造函数中，调用了一系列OpenSSL库的初始化函数，包括SSL_library_init()、SSL_load_error_strings()和OpenSSL_add_all_algorithms()。

这样的设计可以确保在程序启动时进行一次性的SSL库初始化工作，避免了重复的初始化操作，并且利用了C++中静态变量的特性，确保了初始化顺序和生命周期的控制。
由于使用了匿名命名空间，这些初始化相关的代码不会暴露给外部使用，保持了代码的封装性和隐私性。
*/
namespace {

struct _SSLInit {
    _SSLInit() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }
};

static _SSLInit s_init;

}

/**
 * SSLSocket类的构造函数，调用基类Socket的构造函数进行初始化。
 */
SSLSocket::SSLSocket(int family, int type, int protocol)
    : Socket(family, type, protocol) {
}

/**
 * 接受新的SSL连接请求。
 *
 * @return 如果成功接受连接并初始化SSL套接字，则返回SSL套接字对象指针；否则返回空指针。
 */
Socket::ptr SSLSocket::accept() {
    SSLSocket::ptr sock(new SSLSocket(m_family, m_type, m_protocol));  // 创建新的SSL套接字对象
    int newsock = ::accept(m_sock, nullptr, nullptr);  // 接受连接请求
    if (newsock == -1) {
        WEBSERVER_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
            << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    sock->m_ctx = m_ctx;  // 复制SSL上下文
    if (sock->init(newsock)) {  // 初始化SSL套接字
        return sock;
    }
    return nullptr;
}

/**
 * 绑定SSL套接字到指定地址。
 *
 * @param addr 指定的地址对象指针
 * @return 如果绑定成功，则返回true；否则返回false。
 */
bool SSLSocket::bind(const Address::ptr addr) {
    return Socket::bind(addr);  // 调用基类Socket的bind函数进行绑定
}

/**
 * 发起SSL连接请求。
 *
 * @param addr 连接目标地址对象指针
 * @param timeout_ms 连接超时时间（毫秒）
 * @return 如果连接成功，则返回true；否则返回false。
 */
bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    bool v = Socket::connect(addr, timeout_ms);  // 调用基类Socket的connect函数进行连接
    if (v) {
        // 创建SSL上下文和SSL对象，并将SSL对象与套接字关联
        m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
        m_ssl.reset(SSL_new(m_ctx.get()),  SSL_free);
        SSL_set_fd(m_ssl.get(), m_sock);
        v = (SSL_connect(m_ssl.get()) == 1);  // 发起SSL连接
    }
    return v;
}

/**
 * 启动SSL套接字的监听。
 *
 * @param backlog 监听队列的最大长度
 * @return 如果启动监听成功，则返回true；否则返回false。
 */
bool SSLSocket::listen(int backlog) {
    return Socket::listen(backlog);  // 调用基类Socket的listen函数启动监听
}

/**
 * 关闭SSL套接字。
 *
 * @return 如果关闭成功，则返回true；否则返回false。
 */
bool SSLSocket::close() {
    return Socket::close();  // 调用基类Socket的close函数关闭套接字
}


/**
 * 发送数据到SSL连接。
 *
 * @param buffer 待发送数据的缓冲区指针
 * @param length 待发送数据的长度
 * @param flags 发送标志
 * @return 如果发送成功，则返回发送的字节数；否则返回-1表示发送失败。
 */
int SSLSocket::send(const void* buffer, size_t length, int flags) {
    if (m_ssl) {
        return SSL_write(m_ssl.get(), buffer, length);  // 使用SSL_write函数发送数据
    }
    return -1;
}

/**
 * 发送数据到SSL连接。
 *
 * @param buffers 包含待发送数据的iovec数组指针
 * @param length iovec数组的长度
 * @param flags 发送标志
 * @return 如果发送成功，则返回发送的字节数；否则返回-1表示发送失败。
 */
int SSLSocket::send(const iovec* buffers, size_t length, int flags) {
    if (!m_ssl) {
        return -1;
    }
    int total = 0;
    for (size_t i = 0; i < length; ++i) {
        int tmp = SSL_write(m_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);  // 使用SSL_write函数发送数据
        if (tmp <= 0) {
            return tmp;  // 发送失败，返回错误码
        }
        total += tmp;
        if (tmp != (int)buffers[i].iov_len) {
            break;  // 发送未完成，退出循环
        }
    }
    return total;  // 返回总共发送的字节数
}

/**
 * 发送数据到指定的地址（未实现）。
 *
 * @param buffer 待发送数据的缓冲区指针
 * @param length 待发送数据的长度
 * @param to 目标地址对象指针
 * @param flags 发送标志
 * @return 返回-1表示发送失败。
 */
int SSLSocket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    WEBSERVER_ASSERT(false);  // 未实现该函数，断言失败
    return -1;
}

/**
 * 发送数据到指定的地址（未实现）。
 *
 * @param buffers 包含待发送数据的iovec数组指针
 * @param length iovec数组的长度
 * @param to 目标地址对象指针
 * @param flags 发送标志
 * @return 返回-1表示发送失败。
 */
int SSLSocket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    WEBSERVER_ASSERT(false);  // 未实现该函数，断言失败
    return -1;
}


/**
 * 从SSL连接接收数据。
 *
 * @param buffer 接收数据的缓冲区指针
 * @param length 缓冲区长度
 * @param flags 接收标志
 * @return 如果接收成功，则返回接收的字节数；否则返回-1表示接收失败。
 */
int SSLSocket::recv(void* buffer, size_t length, int flags) {
    if (m_ssl) {
        return SSL_read(m_ssl.get(), buffer, length);  // 使用SSL_read函数接收数据
    }
    return -1;
}

/**
 * 从SSL连接接收数据。
 *
 * @param buffers 包含接收数据的iovec数组指针
 * @param length iovec数组的长度
 * @param flags 接收标志
 * @return 如果接收成功，则返回接收的字节数；否则返回-1表示接收失败。
 */
int SSLSocket::recv(iovec* buffers, size_t length, int flags) {
    if (!m_ssl) {
        return -1;
    }
    int total = 0;
    for (size_t i = 0; i < length; ++i) {
        int tmp = SSL_read(m_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);  // 使用SSL_read函数接收数据
        if (tmp <= 0) {
            return tmp;  // 接收失败，返回错误码
        }
        total += tmp;
        if (tmp != (int)buffers[i].iov_len) {
            break;  // 接收未完成，退出循环
        }
    }
    return total;  // 返回总共接收的字节数
}

/**
 * 从指定地址接收数据（未实现）。
 *
 * @param buffer 接收数据的缓冲区指针
 * @param length 缓冲区长度
 * @param from 源地址对象指针
 * @param flags 接收标志
 * @return 返回-1表示接收失败。
 */
int SSLSocket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    WEBSERVER_ASSERT(false);  // 未实现该函数，断言失败
    return -1;
}

/**
 * 从指定地址接收数据（未实现）。
 *
 * @param buffers 包含接收数据的iovec数组指针
 * @param length iovec数组的长度
 * @param from 源地址对象指针
 * @param flags 接收标志
 * @return 返回-1表示接收失败。
 */
int SSLSocket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    WEBSERVER_ASSERT(false);  // 未实现该函数，断言失败
    return -1;
}

/**
 * 初始化SSL套接字。
 *
 * @param sock 套接字描述符
 * @return 如果初始化成功，则返回true；否则返回false。
 */
bool SSLSocket::init(int sock) {
    // 调用基类Socket的init函数初始化套接字
    bool v = Socket::init(sock);
    if (v) {
        // 创建SSL对象，并将SSL对象与套接字关联
        m_ssl.reset(SSL_new(m_ctx.get()),  SSL_free);
        SSL_set_fd(m_ssl.get(), m_sock);
        // 发起SSL握手，如果握手成功则返回1，否则返回其他值
        v = (SSL_accept(m_ssl.get()) == 1);
    }
    return v;
}

/**
 * 加载SSL证书和密钥。
 *
 * @param cert_file SSL证书文件路径
 * @param key_file SSL密钥文件路径
 * @return 如果加载成功，则返回true；否则返回false。
 */
bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file) {
    // 创建SSL上下文，并指定SSL版本为服务器端
    m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
    // 使用SSL_CTX_use_certificate_chain_file加载SSL证书链文件，返回值为1表示成功，否则失败
    if (SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1) {
        // 记录加载证书链文件失败的日志信息
        WEBSERVER_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file("
            << cert_file << ") error";
        return false;
    }
    // 使用SSL_CTX_use_PrivateKey_file加载SSL私钥文件，返回值为1表示成功，否则失败
    if (SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
        // 记录加载私钥文件失败的日志信息
        WEBSERVER_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file("
            << key_file << ") error";
        return false;
    }
    // 检查加载的私钥是否匹配证书，返回值为1表示匹配，否则不匹配
    if (SSL_CTX_check_private_key(m_ctx.get()) != 1) {
        // 记录私钥和证书不匹配的日志信息
        WEBSERVER_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key cert_file="
            << cert_file << " key_file=" << key_file;
        return false;
    }
    return true;
}

/**
 * 创建SSL TCP套接字对象，根据指定地址创建相应类型的套接字。
 *
 * @param address 指定的地址对象指针
 * @return 返回创建的SSL TCP套接字对象指针
 */
SSLSocket::ptr SSLSocket::CreateTCP(webserver::Address::ptr address) {
    // 根据地址对象的地址族创建相应类型的SSL套接字
    SSLSocket::ptr sock(new SSLSocket(address->getFamily(), TCP, 0));
    return sock;
}

/**
 * 创建SSL TCP套接字对象，使用IPv4地址族创建套接字。
 *
 * @return 返回创建的SSL TCP套接字对象指针
 */
SSLSocket::ptr SSLSocket::CreateTCPSocket() {
    // 使用IPv4地址族创建SSL TCP套接字
    SSLSocket::ptr sock(new SSLSocket(IPv4, TCP, 0));
    return sock;
}

/**
 * 创建SSL TCP套接字对象，使用IPv6地址族创建套接字。
 *
 * @return 返回创建的SSL TCP套接字对象指针
 */
SSLSocket::ptr SSLSocket::CreateTCPSocket6() {
    // 使用IPv6地址族创建SSL TCP套接字
    SSLSocket::ptr sock(new SSLSocket(IPv6, TCP, 0));
    return sock;
}

/**
 * 将SSLSocket对象的信息输出到给定的输出流中。
 *
 * @param os 输出流对象引用
 * @return 返回输出流对象引用
 */
std::ostream& SSLSocket::dump(std::ostream& os) const {
    os << "[SSLSocket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if (m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if (m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

/**
 * 重载输出流操作符<<，用于将Socket对象的信息输出到给定的输出流中。
 *
 * @param os 输出流对象引用
 * @param sock Socket对象引用
 * @return 返回输出流对象引用
 */
std::ostream& operator<<(std::ostream& os, const Socket& sock) {
    // 调用Socket对象的dump函数将信息输出到给定的输出流中
    return sock.dump(os);
}


}

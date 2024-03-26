#include "http_server.h"
#include "src/log.h"
#include "src/http/servlets/config_servlet.h"
#include "src/http/servlets/status_servlet.h"

namespace webserver {
namespace http {

/**
 * 描述：定义一个静态Logger对象，用于记录系统日志。
 * 详细描述：
 *  - 使用webserver命名空间内的Logger类定义一个静态指针g_logger，并初始化为指定名称的Logger对象。
 */
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * 类名：HttpServer
 * 功能：实现HTTP服务器，处理HTTP请求并响应客户端。
 * 详细描述：
 *  - 继承自TcpServer类，用于管理TCP服务器。
 *  - 通过ServletDispatch类进行HTTP请求的分发和处理。
 */

/**
 * 构造函数
 * 功能：初始化HttpServer对象。
 * 参数：
 *   - keepalive: bool类型，表示是否启用HTTP长连接。
 *   - worker: 指向IOManager对象的指针，表示工作线程。
 *   - io_worker: 指向IOManager对象的指针，表示IO线程。
 *   - accept_worker: 指向IOManager对象的指针，表示接收连接的线程。
 * 详细描述：
 *  - 调用基类TcpServer的构造函数，初始化工作线程、IO线程和接收连接线程。
 *  - 设置m_isKeepalive为提供的keepalive参数。
 *  - 创建一个ServletDispatch对象，并设置默认的NotFoundServlet。
 *  - 设置服务器类型为"http"。
 *  - 注册默认的StatusServlet和ConfigServlet。
 */
HttpServer::HttpServer(bool keepalive
               ,webserver::IOManager* worker
               ,webserver::IOManager* io_worker
               ,webserver::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) // 调用TcpServer的构造函数
    ,m_isKeepalive(keepalive) { // 初始化m_isKeepalive成员变量
    m_dispatch.reset(new ServletDispatch); // 创建ServletDispatch对象并交给智能指针管理

    m_type = "http"; // 设置服务器类型为HTTP

    // 注册内置的StatusServlet和ConfigServlet
    m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

/**
 * 设置服务器名称
 * 参数：
 *   - v: const std::string&类型，表示要设置的服务器名称。
 * 详细描述：
 *  - 调用基类TcpServer的setName函数设置服务器名称。
 *  - 设置ServletDispatch的默认NotFoundServlet的名称。
 */
void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v); // 调用基类TcpServer的setName函数设置服务器名称
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v)); // 设置默认的NotFoundServlet
}

/**
 * 处理客户端连接
 * 参数：
 *   - client: 指向Socket对象的智能指针，表示客户端连接的套接字。
 * 详细描述：
 *  - 使用Logger对象记录处理客户端连接的日志。
 *  - 创建HttpSession对象处理HTTP会话。
 *  - 循环接收HTTP请求并处理，直到客户端断开连接或不再保持长连接。
 *  - 在处理完毕后关闭会话。
 */
void HttpServer::handleClient(Socket::ptr client) {
    WEBSERVER_LOG_DEBUG(g_logger) << "handleClient " << *client; // 记录处理客户端连接的日志
    HttpSession::ptr session(new HttpSession(client)); // 创建HttpSession对象处理HTTP会话
    do {
        auto req = session->recvRequest(); // 接收HTTP请求
        if(!req) { // 如果接收失败
            WEBSERVER_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive; // 记录错误日志
            break;
        }

        // 创建HTTP响应对象
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName()); // 设置响应头中的Server字段
        m_dispatch->handle(req, rsp, session); // 调用ServletDispatch处理HTTP请求
        session->sendResponse(rsp); // 发送HTTP响应

        // 如果不再保持长连接或请求要求关闭连接，则跳出循环
        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close(); // 关闭会话
}

}
}

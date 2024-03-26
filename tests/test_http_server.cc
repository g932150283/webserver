// #include "src/http/http_server.h"
// #include "src/log.h"

// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// #define XX(...) #__VA_ARGS__


// webserver::IOManager::ptr worker;
// void run() {
//     g_logger->setLevel(webserver::LogLevel::INFO);
//     //webserver::http::HttpServer::ptr server(new webserver::http::HttpServer(true, worker.get(), webserver::IOManager::GetThis()));
//     webserver::http::HttpServer::ptr server(new webserver::http::HttpServer(true));
//     webserver::Address::ptr addr = webserver::Address::LookupAnyIPAddress("0.0.0.0:8020");
//     while(!server->bind(addr)) {
//         sleep(2);
//     }
//     auto sd = server->getServletDispatch();
//     sd->addServlet("/webserver/xx", [](webserver::http::HttpRequest::ptr req
//                 ,webserver::http::HttpResponse::ptr rsp
//                 ,webserver::http::HttpSession::ptr session) {
//             rsp->setBody(req->toString());
//             return 0;
//     });

//     sd->addGlobServlet("/webserver/*", [](webserver::http::HttpRequest::ptr req
//                 ,webserver::http::HttpResponse::ptr rsp
//                 ,webserver::http::HttpSession::ptr session) {
//             rsp->setBody("Glob:\r\n" + req->toString());
//             return 0;
//     });

//     sd->addGlobServlet("/webserverx/*", [](webserver::http::HttpRequest::ptr req
//                 ,webserver::http::HttpResponse::ptr rsp
//                 ,webserver::http::HttpSession::ptr session) {
//             rsp->setBody(XX(<html>
// <head><title>404 Not Found</title></head>
// <body>
// <center><h1>404 Not Found</h1></center>
// <hr><center>nginx/1.16.0</center>
// </body>
// </html>
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// ));
//             return 0;
//     });

//     server->start();
// }

// int main(int argc, char** argv) {
//     webserver::IOManager iom(1, true, "main");
//     worker.reset(new webserver::IOManager(3, false, "worker"));
//     iom.schedule(run);
//     return 0;
// }
#include "src/http/http_server.h" // 包含HTTP服务器头文件
#include "src/log.h" // 包含日志头文件

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT(); // 定义全局Logger对象

#define XX(...) #__VA_ARGS__ // 定义宏，将参数转换为字符串

webserver::IOManager::ptr worker; // 定义全局IO管理器指针

/**
 * 函数名：run
 * 功能：运行HTTP服务器
 * 详细描述：
 *  - 设置全局Logger对象的日志级别为INFO。
 *  - 创建HttpServer对象并设置为长连接模式。
 *  - 绑定服务器地址为0.0.0.0:8020，如果绑定失败则每隔2秒重试。
 *  - 添加ServletDispatch的处理逻辑，包括指定路径和通配符路径的处理函数。
 *  - 启动HTTP服务器。
 */
void run() {
    g_logger->setLevel(webserver::LogLevel::INFO); // 设置全局Logger对象的日志级别为INFO

    webserver::http::HttpServer::ptr server(new webserver::http::HttpServer(true)); // 创建HttpServer对象
    webserver::Address::ptr addr = webserver::Address::LookupAnyIPAddress("0.0.0.0:8020"); // 解析服务器地址
    while(!server->bind(addr)) { // 绑定服务器地址
        sleep(2); // 绑定失败则休眠2秒后重试
    }
    
    auto sd = server->getServletDispatch(); // 获取ServletDispatch对象

    // 添加指定路径的处理函数，将请求信息作为响应体返回
    sd->addServlet("/sylar/xx", [](webserver::http::HttpRequest::ptr req
                ,webserver::http::HttpResponse::ptr rsp
                ,webserver::http::HttpSession::ptr session) {
            rsp->setBody(req->toString()); // 设置响应体为请求信息
            return 0; // 返回处理结果
    });

    // 添加通配符路径的处理函数，将请求信息作为响应体返回
    sd->addGlobServlet("/sylar/*", [](webserver::http::HttpRequest::ptr req
                ,webserver::http::HttpResponse::ptr rsp
                ,webserver::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString()); // 设置响应体为请求信息
            return 0; // 返回处理结果
    });

    // 添加通配符路径的处理函数，返回404错误页面
    sd->addGlobServlet("/sylarx/*", [](webserver::http::HttpRequest::ptr req
                ,webserver::http::HttpResponse::ptr rsp
                ,webserver::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
)); // 设置响应体为404错误页面
            return 0; // 返回处理结果
    });

    server->start(); // 启动HTTP服务器
}

/**
 * 函数名：main
 * 功能：程序入口函数
 * 参数：
 *   - argc: int类型，表示命令行参数个数。
 *   - argv: char**类型，表示命令行参数数组。
 * 返回值：整型，表示程序执行结果。
 * 详细描述：
 *  - 创建主IO管理器对象iom，设置线程数量为1，启动时调度该IO管理器。
 *  - 创建工作IO管理器对象worker，设置线程数量为3，不使用主IO管理器。
 *  - 将run函数调度到主IO管理器中执行。
 *  - 返回程序执行结果。
 */
int main(int argc, char** argv) {
    webserver::IOManager iom(1, true, "main"); // 创建主IO管理器对象iom
    worker.reset(new webserver::IOManager(3, false, "worker")); // 创建工作IO管理器对象worker

    iom.schedule(run); // 调度run函数到主IO管理器中执行

    return 0; // 返回程序执行结果
}

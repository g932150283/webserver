#include "servlet.h"
#include <fnmatch.h>

namespace webserver {
namespace http {

/**
 * 描述：构造函数
 * 功能：创建FunctionServlet对象并初始化。
 * 参数：
 *   - cb: FunctionServlet::callback类型，表示要执行的回调函数。
 * 详细描述：
 *  - 调用基类Servlet的构造函数，设置Servlet的名称为"FunctionServlet"。
 *  - 将回调函数cb赋值给成员变量m_cb。
 */
FunctionServlet::FunctionServlet(callback cb)
    :Servlet("FunctionServlet")
    ,m_cb(cb) {
}

/**
 * 描述：处理HTTP请求
 * 参数：
 *   - request: 指向HttpRequest对象的智能指针，表示HTTP请求。
 *   - response: 指向HttpResponse对象的智能指针，表示HTTP响应。
 *   - session: 指向HttpSession对象的智能指针，表示HTTP会话。
 * 返回值：int32_t类型，表示处理结果，一般为0。
 * 详细描述：
 *  - 调用回调函数m_cb处理HTTP请求。
 */
int32_t FunctionServlet::handle(webserver::http::HttpRequest::ptr request
               , webserver::http::HttpResponse::ptr response
               , webserver::http::HttpSession::ptr session) {
    return m_cb(request, response, session);
}

/**
 * 描述：构造函数
 * 功能：创建ServletDispatch对象并初始化。
 * 详细描述：
 *  - 调用基类Servlet的构造函数，设置Servlet的名称为"ServletDispatch"。
 *  - 创建默认的NotFoundServlet，并交给智能指针管理。
 */
ServletDispatch::ServletDispatch()
    :Servlet("ServletDispatch") {
    m_default.reset(new NotFoundServlet("webserver/1.0"));
}

/**
 * 描述：处理HTTP请求
 * 参数：
 *   - request: 指向HttpRequest对象的智能指针，表示HTTP请求。
 *   - response: 指向HttpResponse对象的智能指针，表示HTTP响应。
 *   - session: 指向HttpSession对象的智能指针，表示HTTP会话。
 * 返回值：int32_t类型，表示处理结果，一般为0。
 * 详细描述：
 *  - 获取匹配的Servlet，并调用其处理函数处理HTTP请求。
 */
int32_t ServletDispatch::handle(webserver::http::HttpRequest::ptr request
               , webserver::http::HttpResponse::ptr response
               , webserver::http::HttpSession::ptr session) {
    auto slt = getMatchedServlet(request->getPath()); // 获取匹配的Servlet
    if(slt) { // 如果找到匹配的Servlet
        slt->handle(request, response, session); // 调用其处理函数处理HTTP请求
    }
    return 0;
}

/**
 * 描述：添加指定URI的Servlet
 * 参数：
 *   - uri: const std::string&类型，表示要添加的URI。
 *   - slt: Servlet::ptr类型，表示要添加的Servlet。
 * 详细描述：
 *  - 加写锁，保护共享资源m_datas。
 *  - 将URI与Servlet的映射关系存储到m_datas中。
 */
void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    m_datas[uri] = std::make_shared<HoldServletCreator>(slt); // 存储URI与Servlet的映射关系
}

/**
 * 描述：添加指定URI的Servlet创建器
 * 参数：
 *   - uri: const std::string&类型，表示要添加的URI。
 *   - creator: IServletCreator::ptr类型，表示要添加的Servlet创建器。
 * 详细描述：
 *  - 加写锁，保护共享资源m_datas。
 *  - 将URI与Servlet创建器的映射关系存储到m_datas中。
 */
void ServletDispatch::addServletCreator(const std::string& uri, IServletCreator::ptr creator) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    m_datas[uri] = creator; // 存储URI与Servlet创建器的映射关系
}

/**
 * 描述：添加匹配指定URI模式的Servlet创建器
 * 参数：
 *   - uri: const std::string&类型，表示要添加的URI模式。
 *   - creator: IServletCreator::ptr类型，表示要添加的Servlet创建器。
 * 详细描述：
 *  - 加写锁，保护共享资源m_globs。
 *  - 遍历m_globs，如果已存在相同URI模式，则先移除原有的。
 *  - 将URI模式与Servlet创建器的映射关系存储到m_globs中。
 */
void ServletDispatch::addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) { // 如果已存在相同URI模式
            m_globs.erase(it); // 先移除原有的
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, creator)); // 存储URI模式与Servlet创建器的映射关系
}

/**
 * 构造函数
 * 参数：
 *   - cb: FunctionServlet::callback类型的回调函数
 * 详细描述：
 *  - 使用提供的回调函数创建一个FunctionServlet对象，并将其添加到ServletDispatch中。
 */
void ServletDispatch::addServlet(const std::string& uri
                        ,FunctionServlet::callback cb) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    m_datas[uri] = std::make_shared<HoldServletCreator>( // 将FunctionServlet对象添加到m_datas中
                        std::make_shared<FunctionServlet>(cb));
}

/**
 * 添加全局Servlet
 * 参数：
 *   - uri: 表示要匹配的URI
 *   - slt: 指向Servlet对象的智能指针
 * 详细描述：
 *  - 将提供的Servlet对象添加到m_globs中，以匹配uri。
 */
void ServletDispatch::addGlobServlet(const std::string& uri
                                    ,Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    for(auto it = m_globs.begin(); // 遍历m_globs
            it != m_globs.end(); ++it) {
        if(it->first == uri) { // 如果找到相同uri的Servlet，删除之
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri // 将提供的Servlet对象添加到m_globs中
                , std::make_shared<HoldServletCreator>(slt)));
}

/**
 * 添加全局FunctionServlet
 * 参数：
 *   - uri: 表示要匹配的URI
 *   - cb: FunctionServlet::callback类型的回调函数
 * 详细描述：
 *  - 使用提供的回调函数创建一个FunctionServlet对象，并将其作为Servlet对象添加到m_globs中，以匹配uri。
 */
void ServletDispatch::addGlobServlet(const std::string& uri
                                ,FunctionServlet::callback cb) {
    return addGlobServlet(uri, std::make_shared<FunctionServlet>(cb)); // 调用上面的addGlobServlet函数
}

/**
 * 删除指定URI的Servlet
 * 参数：
 *   - uri: 表示要删除的URI
 * 详细描述：
 *  - 从m_datas中删除指定URI的Servlet。
 */
void ServletDispatch::delServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    m_datas.erase(uri); // 删除指定URI的Servlet
}

/**
 * 删除指定URI的全局Servlet
 * 参数：
 *   - uri: 表示要删除的URI
 * 详细描述：
 *  - 从m_globs中删除指定URI的全局Servlet。
 */
void ServletDispatch::delGlobServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex); // 加写锁
    for(auto it = m_globs.begin(); // 遍历m_globs
            it != m_globs.end(); ++it) {
        if(it->first == uri) { // 如果找到相同uri的全局Servlet，删除之
            m_globs.erase(it);
            break;
        }
    }
}


/**
 * 获取指定URI的Servlet
 * 参数：
 *   - uri: 表示要获取的URI
 * 返回值：
 *   - 返回指向Servlet对象的智能指针
 * 详细描述：
 *  - 使用读锁锁定m_mutex，查找指定URI的Servlet。
 *  - 如果找到，返回对应的Servlet对象；否则返回nullptr。
 */
Servlet::ptr ServletDispatch::getServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex); // 加读锁
    auto it = m_datas.find(uri); // 在m_datas中查找指定URI的Servlet
    return it == m_datas.end() ? nullptr : it->second->get(); // 返回找到的Servlet对象，如果不存在则返回nullptr
}

/**
 * 获取匹配的全局Servlet
 * 参数：
 *   - uri: 表示要匹配的URI
 * 返回值：
 *   - 返回指向Servlet对象的智能指针
 * 详细描述：
 *  - 使用读锁锁定m_mutex，遍历m_globs查找匹配的全局Servlet。
 *  - 如果找到匹配的URI，返回对应的Servlet对象；否则返回nullptr。
 */
Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex); // 加读锁
    for(auto it = m_globs.begin(); // 遍历m_globs
            it != m_globs.end(); ++it) {
        if(it->first == uri) { // 如果找到相同uri的全局Servlet
            return it->second->get(); // 返回对应的Servlet对象
        }
    }
    return nullptr; // 没有找到匹配的全局Servlet，返回nullptr
}

/**
 * 获取匹配的Servlet
 * 参数：
 *   - uri: 表示要匹配的URI
 * 返回值：
 *   - 返回指向Servlet对象的智能指针
 * 详细描述：
 *  - 使用读锁锁定m_mutex，先在m_datas中查找匹配的Servlet，如果找到则返回对应的Servlet对象。
 *  - 如果在m_datas中未找到匹配的Servlet，则在m_globs中使用fnmatch函数进行模式匹配查找。
 *  - 如果找到匹配的全局Servlet，返回对应的Servlet对象；否则返回m_default指向的默认Servlet对象。
 */
Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex); // 加读锁
    auto mit = m_datas.find(uri); // 在m_datas中查找指定URI的Servlet
    if(mit != m_datas.end()) { // 如果找到
        return mit->second->get(); // 返回对应的Servlet对象
    }
    for(auto it = m_globs.begin(); // 遍历m_globs
            it != m_globs.end(); ++it) {
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) { // 使用fnmatch函数进行模式匹配
            return it->second->get(); // 如果找到匹配的全局Servlet，返回对应的Servlet对象
        }
    }
    return m_default; // 返回默认的NotFoundServlet对象
}

/**
 * 获取所有Servlet的创建者
 * 参数：
 *   - infos: 保存Servlet URI与IServletCreator智能指针的映射
 * 详细描述：
 *  - 使用读锁锁定m_mutex，遍历m_datas，将所有的Servlet URI与对应的IServletCreator智能指针保存到infos中。
 */
void ServletDispatch::listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos) {
    RWMutexType::ReadLock lock(m_mutex); // 加读锁
    for(auto& i : m_datas) { // 遍历m_datas
        infos[i.first] = i.second; // 将Servlet URI与对应的IServletCreator智能指针保存到infos中
    }
}

/**
 * 获取所有全局Servlet的创建者
 * 参数：
 *   - infos: 保存Servlet URI与IServletCreator智能指针的映射
 * 详细描述：
 *  - 使用读锁锁定m_mutex，遍历m_globs，将所有的Servlet URI与对应的IServletCreator智能指针保存到infos中。
 */
void ServletDispatch::listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos) {
    RWMutexType::ReadLock lock(m_mutex); // 加读锁
    for(auto& i : m_globs) { // 遍历m_globs
        infos[i.first] = i.second; // 将Servlet URI与对应的IServletCreator智能指针保存到infos中
    }
}


/**
 * 构造函数
 * 参数：
 *   - name: const std::string&类型，表示NotFoundServlet的名称
 * 详细描述：
 *  - 使用提供的名称初始化Servlet基类，并保存到成员变量m_name中。
 *  - 构建一个404 Not Found的HTML响应内容，包括标题、主体和页面底部信息，并保存到成员变量m_content中。
 */
NotFoundServlet::NotFoundServlet(const std::string& name)
    :Servlet("NotFoundServlet") // 调用基类Servlet的构造函数，指定名称为"NotFoundServlet"
    ,m_name(name) // 初始化成员变量m_name
{
    // 构建404 Not Found的HTML响应内容，包括标题、主体和页面底部信息
    m_content = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>" + name + "</center></body></html>";
}

/**
 * 处理HTTP请求
 * 参数：
 *   - request: 指向HttpRequest对象的智能指针，表示HTTP请求
 *   - response: 指向HttpResponse对象的智能指针，表示HTTP响应
 *   - session: 指向HttpSession对象的智能指针，表示HTTP会话
 * 返回值：
 *   - 返回0表示处理成功
 * 详细描述：
 *  - 设置HTTP响应状态为404 Not Found。
 *  - 设置响应头中的Server字段为"webserver/1.0.0"。
 *  - 设置响应头中的Content-Type字段为"text/html"。
 *  - 设置响应体内容为构造函数中保存的404 Not Found的HTML响应内容。
 */
int32_t NotFoundServlet::handle(webserver::http::HttpRequest::ptr request
                   , webserver::http::HttpResponse::ptr response
                   , webserver::http::HttpSession::ptr session) {
    response->setStatus(webserver::http::HttpStatus::NOT_FOUND); // 设置HTTP响应状态为404 Not Found
    response->setHeader("Server", "webserver/1.0.0"); // 设置响应头中的Server字段为"webserver/1.0.0"
    response->setHeader("Content-Type", "text/html"); // 设置响应头中的Content-Type字段为"text/html"
    response->setBody(m_content); // 设置响应体内容为构造函数中保存的404 Not Found的HTML响应内容
    return 0; // 返回0表示处理成功
}



}
}

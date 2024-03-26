#include "http.h"
#include "src/util.h"

namespace webserver {
namespace http {

/**
 * 将字符串转换为对应的HttpMethod枚举。
 * 
 * @param m 输入字符串，表示HTTP方法的名称。
 * @return HttpMethod枚举，代表对应的HTTP方法。
 * 
 * 该函数通过比较输入字符串与预定义的HTTP方法字符串，来返回相应的HttpMethod枚举值。
 * 如果没有匹配的方法，则返回INVALID_METHOD。
 */
HttpMethod StringToHttpMethod(const std::string& m) {
    // 定义宏XX，用于简化多个if语句的书写。该宏通过比较字符串，判断是否与输入匹配。
    #define XX(num, name, string) \
        if(strcmp(#string, m.c_str()) == 0) { \
            return HttpMethod::name; \
        }
    // 调用HTTP_METHOD_MAP宏，实际上是展开成多个XX宏调用，每个调用对应一种HTTP方法的判断。
    HTTP_METHOD_MAP(XX);
    // 宏定义结束后，取消定义XX，以避免影响其他地方。
    #undef XX
    // 如果没有匹配的HTTP方法，返回INVALID_METHOD。
    return HttpMethod::INVALID_METHOD;
}


/**
 * 将字符数组转换为对应的HttpMethod枚举。
 * 
 * @param m 输入的字符数组，表示HTTP方法的名称。
 * @return HttpMethod枚举，代表对应的HTTP方法。
 * 
 * 该函数通过比较输入字符数组与预定义的HTTP方法字符串，来返回相应的HttpMethod枚举值。
 * 如果没有匹配的方法，则返回INVALID_METHOD。
 * 与StringToHttpMethod函数相比，此函数适用于直接与C风格字符串进行比较的场景。
 */
HttpMethod CharsToHttpMethod(const char* m) {
    // 定义宏XX，与StringToHttpMethod中的定义类似，但使用strncmp和strlen来比较固定长度的字符串。
    #define XX(num, name, string) \
        if(strncmp(#string, m, strlen(#string)) == 0) { \
            return HttpMethod::name; \
        }
    // 调用HTTP_METHOD_MAP宏，对每种HTTP方法进行比较判断。
    HTTP_METHOD_MAP(XX);
    // 取消宏XX的定义，避免影响其他代码。
    #undef XX
    // 如果没有匹配的HTTP方法，返回INVALID_METHOD。
    return HttpMethod::INVALID_METHOD;
}


/**
 * 定义一个字符串数组，用于存储HTTP方法的字符串表示。
 * 
 * 该数组通过宏`XX`和`HTTP_METHOD_MAP`的组合来填充。每次`XX`宏被调用时，
 * 它将HTTP方法的字符串表示作为元素添加到数组中。
 * `HTTP_METHOD_MAP`宏定义了一个枚举到字符串的映射，其中包括了各种HTTP方法，
 * 如GET、POST等，每种方法通过调用`XX`宏来添加到`s_method_string`数组中。
 */
static const char* s_method_string[] = {
    // 定义宏XX，它接受三个参数：数字编号、枚举名称和方法的字符串表示。
    // 宏的作用是将方法的字符串表示转换为字符串，并作为数组的一个元素。
#define XX(num, name, string) #string,
    // 调用HTTP_METHOD_MAP宏，这个宏内部会多次调用XX宏，为每种HTTP方法添加一个字符串表示到数组中。
    HTTP_METHOD_MAP(XX)
    // 完成宏的调用后，取消对XX宏的定义，防止其在其他地方意外被使用，保持了宏的作用域仅限于此数组的初始化过程中。
#undef XX
};


/**
 * 将HttpMethod枚举转换为对应的字符串表示。
 * 
 * @param m HttpMethod枚举值，表示HTTP方法。
 * @return const char* 字符串，代表对应的HTTP方法名称。如果传入的枚举值不在预定义范围内，则返回"<unknown>"。
 * 
 * 该函数通过将HttpMethod枚举值作为索引从预定义的字符串数组中查找对应的字符串。
 * 如果索引超出了数组的范围，则返回"<unknown>"表示未知的HTTP方法。
 */
const char* HttpMethodToString(const HttpMethod& m) {
    // 将枚举值转换为uint32_t类型的索引。
    uint32_t idx = (uint32_t)m;
    // 检查索引是否超出了s_method_string数组的范围。
    if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        // 如果索引超出范围，返回"<unknown>"。
        return "<unknown>";
    }
    // 如果索引在范围内，根据索引从数组中返回对应的字符串。
    return s_method_string[idx];
}


/**
 * 将HttpStatus枚举转换为对应的字符串消息。
 * 
 * @param s HttpStatus枚举值，表示HTTP状态码。
 * @return const char* 字符串，代表对应的HTTP状态消息。如果传入的枚举值不在预定义范围内，则返回"<unknown>"。
 * 
 * 该函数通过switch语句匹配HttpStatus枚举值，返回对应的状态消息字符串。
 * 如果没有匹配的case，则默认返回"<unknown>"表示未知的HTTP状态。
 */
const char* HttpStatusToString(const HttpStatus& s) {
    // 使用switch语句对HttpStatus枚举进行匹配。
    switch(s) {
        // 定义宏XX，用于简化case语句的书写。对于每个HTTP状态码，返回对应的消息字符串。
#define XX(code, name, msg) \
        case HttpStatus::name: \
            return #msg;
        // 调用HTTP_STATUS_MAP宏，实际上展开成多个case语句，每个对应一种HTTP状态。
        HTTP_STATUS_MAP(XX);
#undef XX
        // 如果没有匹配的状态码，返回"<unknown>"。
        default:
            return "<unknown>";
    }
}

/**
 * 功能: 比较两个字符串，忽略大小写差异。
 * 参数:
 *   lhs - const std::string&，左侧字符串，用于比较。
 *   rhs - const std::string&，右侧字符串，用于比较。
 * 返回值: bool - 如果lhs在不考虑大小写的情况下小于rhs，则返回true；否则返回false。
 */
bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


/**
 * 功能: 初始化HttpRequest对象。
 * 参数:
 *   version - uint8_t，HTTP协议的版本号。
 *   close - bool，表示是否在响应后关闭连接。
 * 成员变量初始化列表:
 *   m_method - HttpMethod::GET，默认请求方法为GET。
 *   m_version - version，根据参数设置HTTP版本。
 *   m_close - close，根据参数设置连接是否在响应后关闭。
 *   m_websocket - false，默认不使用websocket。
 *   m_parserParamFlag - 0，初始化参数解析标志位。
 *   m_path - "/"，默认请求路径为根目录。
 */
HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(HttpMethod::GET), m_version(version), m_close(close),
      m_websocket(false), m_parserParamFlag(0), m_path("/") {
}


/**
 * 功能: 从HTTP请求的头部中获取指定字段的值。如果指定字段不存在，则返回一个默认值。
 * 参数:
 *   key - const std::string&，请求头中要检索的字段名。
 *   def - const std::string&，如果指定字段不存在时返回的默认值。
 * 返回值: std::string - 如果找到指定字段，则返回其值；否则返回默认值def。
 */
std::string HttpRequest::getHeader(const std::string& key, const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}


/**
 * 功能: 创建一个HttpResponse对象，并初始化它的版本和关闭标志。
 * 参数: 无。
 * 返回值: std::shared_ptr<HttpResponse> - 指向新创建的HttpResponse对象的智能指针。
 *
 * 此函数基于HttpRequest对象的当前状态（版本号和关闭标志）创建一个新的HttpResponse对象。
 * 使用std::shared_ptr进行管理，以便自动处理资源回收，增强内存安全性。
 */
std::shared_ptr<HttpResponse> HttpRequest::createResponse() {
    // 使用HttpRequest的版本号和关闭标志创建HttpResponse对象
    auto rsp = std::make_shared<HttpResponse>(getVersion(), isClose());
    return rsp;
}


/**
 * 功能: 从HTTP请求中获取指定的参数值。
 * 参数:
 *   key - const std::string&，要检索的参数名。
 *   def - const std::string&，如果参数不存在时的默认返回值。
 * 返回值: std::string - 如果找到指定参数，则返回其值；否则返回默认值def。
 *
 * 该函数首先初始化查询参数和正文参数（如果尚未初始化），然后尝试查找并返回请求中的指定参数值。
 * 如果请求中不存在该参数，则返回用户提供的默认值。
 */
std::string HttpRequest::getParam(const std::string& key, const std::string& def) {
    initQueryParam();  // 初始化查询参数
    initBodyParam();   // 初始化正文参数
    auto it = m_params.find(key);  // 查找指定的参数
    return it == m_params.end() ? def : it->second;  // 返回参数值或默认值
}


/**
 * 功能: 从HTTP请求中获取指定的Cookie值。
 * 参数:
 *   key - const std::string&，要检索的Cookie名。
 *   def - const std::string&，如果Cookie不存在时的默认返回值。
 * 返回值: std::string - 如果找到指定Cookie，则返回其值；否则返回默认值def。
 *
 * 该函数首先初始化Cookie（如果尚未初始化），然后尝试查找并返回请求中的指定Cookie值。
 * 如果请求中不存在该Cookie，则返回用户提供的默认值。
 */
std::string HttpRequest::getCookie(const std::string& key, const std::string& def) {
    initCookies();  // 初始化Cookie
    auto it = m_cookies.find(key);  // 查找指定的Cookie
    return it == m_cookies.end() ? def : it->second;  // 返回Cookie值或默认值
}

void HttpRequest::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}

void HttpRequest::setParam(const std::string& key, const std::string& val) {
    m_params[key] = val;
}

void HttpRequest::setCookie(const std::string& key, const std::string& val) {
    m_cookies[key] = val;
}

void HttpRequest::delHeader(const std::string& key) {
    m_headers.erase(key);
}

void HttpRequest::delParam(const std::string& key) {
    m_params.erase(key);
}

void HttpRequest::delCookie(const std::string& key) {
    m_cookies.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string* val) {
    auto it = m_headers.find(key);
    if(it == m_headers.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasParam(const std::string& key, std::string* val) {
    initQueryParam();
    initBodyParam();
    auto it = m_params.find(key);
    if(it == m_params.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string& key, std::string* val) {
    initCookies();
    auto it = m_cookies.find(key);
    if(it == m_cookies.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

/**
 * 将HttpRequest对象的内容格式化并输出到指定的ostream中。
 * 
 * @param os 输出流对象的引用，用于接收格式化的HTTP请求数据。
 * @return 返回输出流对象的引用，以支持链式调用。
 * 
 * 功能描述:
 * 1. 格式化HTTP请求行和头部，包括方法、路径、版本等。
 * 2. 忽略WebSocket连接的connection头部。
 * 3. 输出所有其他头部信息。
 * 4. 如果请求体存在，输出content-length头部和请求体内容。
 * 5. 支持链式调用，通过返回输出流对象的引用。
 */
std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << HttpMethodToString(m_method)  // 将HTTP方法转换为字符串并写入流
       << " "  // 添加空格分隔HTTP方法和请求路径
       << m_path  // 添加请求的路径
       << (m_query.empty() ? "" : "?")  // 如果查询字符串不为空，则添加问号
       << m_query  // 添加查询字符串
       << (m_fragment.empty() ? "" : "#")  // 如果fragment不为空，则添加井号
       << m_fragment  // 添加fragment
       << " HTTP/"  // 添加HTTP协议标识
       << ((uint32_t)(m_version >> 4))  // 提取并输出HTTP版本号的主版本部分
       << "."  // 添加点以分隔主副版本号
       << ((uint32_t)(m_version & 0x0F))  // 提取并输出HTTP版本号的副版本部分
       << "\r\n";  // 添加请求行结束符

    if (!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";  // 输出连接头部
    }

    for (auto& i : m_headers) {
        if (!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;  // 对于非WebSocket连接，忽略connection头部
        }
        os << i.first << ": " << i.second << "\r\n";  // 输出其他头部
    }

    if (!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"  // 输出内容长度头部和两个换行
           << m_body;  // 输出请求体
    } else {
        os << "\r\n";  // 如果没有请求体，仅输出一个换行符
    }

    return os;  // 返回输出流对象的引用
}

/**
 * 初始化HttpRequest对象，设置连接状态。
 * 
 * 功能描述:
 * - 根据"connection"头部的值设置m_close成员变量。
 * 
 * 参数: 无
 * 返回值: 无
 */
void HttpRequest::init() {
    std::string conn = getHeader("connection");  // 获取"connection"头部的值
    if (!conn.empty()) {  // 如果"connection"头部存在
        if (strcasecmp(conn.c_str(), "keep-alive") == 0) {  // 如果是保持连接状态
            m_close = false;  // 设置m_close为false，表示保持连接
        } else {  // 否则
            m_close = true;  // 设置m_close为true，表示关闭连接
        }
    }
}

/**
 * 初始化HttpRequest对象的参数，包括查询参数、请求体参数和cookies。
 * 
 * 功能描述:
 * - 依次调用初始化查询参数、请求体参数和cookies的方法。
 * 
 * 参数: 无
 * 返回值: 无
 */
void HttpRequest::initParam() {
    initQueryParam();  // 初始化查询参数
    initBodyParam();  // 初始化请求体参数
    initCookies();  // 初始化cookies
}


/**
 * 初始化HttpRequest对象的查询参数。
 * 
 * 功能描述:
 * - 解析URL的查询字符串，并填充到m_params成员变量。
 * 
 * 参数: 无
 * 返回值: 无
 */
void HttpRequest::initQueryParam() {
    if (m_parserParamFlag & 0x1) {  // 如果已经解析过查询参数，则返回
        return;
    }

    // 宏定义，用于解析参数并填充到指定的map中
    // str: 待解析的字符串
    // m: 存储解析结果的map
    // flag: 参数分隔符
    // trim: 处理键值前后的空白字符的函数
    #define PARSE_PARAM(str, m, flag, trim) \
        size_t pos = 0; \
        do { \
            size_t last = pos; \
            pos = str.find('=', pos); \
            if (pos == std::string::npos) { \
                break; \
            } \
            size_t key = pos; \
            pos = str.find(flag, pos); \
            m.insert(std::make_pair(trim(str.substr(last, key - last)), \
                        webserver::StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
            if (pos == std::string::npos) { \
                break; \
            } \
            ++pos; \
        } while (true);

    // 使用宏解析查询字符串并填充到m_params中
    PARSE_PARAM(m_query, m_params, '&', );
    m_parserParamFlag |= 0x1;  // 设置标志位，表示查询参数已解析
}

/**
 * 初始化HttpRequest对象的请求体参数。
 * 
 * 功能描述:
 * - 如果请求体类型为application/x-www-form-urlencoded，则解析请求体并填充到m_params成员变量。
 * 
 * 参数: 无
 * 返回值: 无
 */
void HttpRequest::initBodyParam() {
    if (m_parserParamFlag & 0x2) {  // 如果已经解析过请求体参数，则返回
        return;
    }
    std::string content_type = getHeader("content-type");  // 获取请求体类型
    if (strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr) {  // 如果请求体类型不是表单格式
        m_parserParamFlag |= 0x2;  // 设置标志位，表示请求体参数已解析
        return;
    }
    // 使用宏解析请求体并填充到m_params中
    PARSE_PARAM(m_body, m_params, '&', );
    m_parserParamFlag |= 0x2;  // 设置标志位，表示请求体参数已解析
}


/**
 * 初始化 HTTP 请求中的 Cookies。
 *
 * 该方法从 HTTP 请求头中解析 "cookie" 字段，以初始化请求中的 Cookies。
 * 它首先检查是否已经解析过 Cookies（通过 m_parserParamFlag 标志位），
 * 避免重复解析。如果请求中包含 Cookies，该方法会解析它们，并将结果存储在
 * m_cookies 成员变量中。解析时，使用 ';' 作为 Cookie 项的分隔符，并对每个
 * Cookie 项应用 Trim 操作以去除前后空格。
 *
 * 注意：该方法仅在 Cookies 尚未解析时执行。一旦 m_parserParamFlag 的第 3 位
 * 被设置，表示 Cookies 已解析，该方法将直接返回。
 *
 * 参数：无。
 *
 * 返回值：无（void）。
 */
void HttpRequest::initCookies() {
    // 检查 m_parserParamFlag 的第 3 位以判断 Cookies 是否已解析
    if(m_parserParamFlag & 0x4) {
        return; // 如果已解析，则直接返回
    }

    // 从 HTTP 请求头中获取 "cookie" 字段的值
    std::string cookie = getHeader("cookie");

    // 如果 cookie 字符串为空，则表示请求中没有 Cookie，设置 m_parserParamFlag 的第 3 位，然后返回
    if(cookie.empty()) {
        m_parserParamFlag |= 0x4;
        return;
    }

    // 使用 PARSE_PARAM 宏（或函数）解析 Cookie 字符串，使用 ';' 作为分隔符，将结果存储在 m_cookies 中
    // 对每个 Cookie 项使用 webserver::StringUtil::Trim 函数去除前后空格
    PARSE_PARAM(cookie, m_cookies, ';', webserver::StringUtil::Trim);

    // 解析完成，设置 m_parserParamFlag 的第 3 位为 1
    m_parserParamFlag |= 0x4;
}


/**
 * 构造函数：初始化 HttpResponse 对象。
 *
 * 构造一个 HttpResponse 对象，并设置其初始状态、版本、连接关闭标志以及 websocket 标志。
 *
 * 参数:
 * - version (uint8_t): HTTP 响应的版本号。
 * - close (bool): 指示在响应后是否关闭连接。
 *
 * 返回值: 无（构造函数）。
 */
HttpResponse::HttpResponse(uint8_t version, bool close)
    : m_status(HttpStatus::OK) // 默认状态码为 OK (200)
    , m_version(version)        // 设置 HTTP 版本
    , m_close(close)            // 设置连接是否关闭
    , m_websocket(false)        // 默认 websocket 支持为关闭
{
}

/**
 * 获取指定 HTTP 响应头的值。
 *
 * 如果给定的 key 存在于响应头中，则返回其对应的值；否则，返回提供的默认值 def。
 *
 * 参数:
 * - key (const std::string&): 要获取的响应头的键。
 * - def (const std::string&): 如果键不存在时返回的默认值。
 *
 * 返回值:
 * - std::string: 响应头的值，或者默认值。
 */
std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const {
    auto it = m_headers.find(key); // 在响应头映射中查找键
    return it == m_headers.end() ? def : it->second; // 如果找到，则返回值；否则返回默认值
}

/**
 * 设置 HTTP 响应头。
 *
 * 在响应头中添加或更新一个键值对。如果键已存在，其值将被更新为提供的 val。
 *
 * 参数:
 * - key (const std::string&): 要设置的响应头的键。
 * - val (const std::string&): 要设置的响应头的值。
 *
 * 返回值: 无。
 */
void HttpResponse::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val; // 设置或更新响应头
}

/**
 * 删除 HTTP 响应头。
 *
 * 如果给定的键存在于响应头中，则删除该键及其对应的值。
 *
 * 参数:
 * - key (const std::string&): 要删除的响应头的键。
 *
 * 返回值: 无。
 */
void HttpResponse::delHeader(const std::string& key) {
    m_headers.erase(key); // 从响应头映射中删除键
}

/**
 * 设置重定向。
 *
 * 将响应的状态码设置为 302 (Found)，并设置 "Location" 响应头以指示重定向的 URI。
 *
 * 参数:
 * - uri (const std::string&): 重定向目标的 URI。
 *
 * 返回值: 无。
 */
void HttpResponse::setRedirect(const std::string& uri) {
    m_status = HttpStatus::FOUND; // 设置状态码为 302 Found
    setHeader("Location", uri);   // 设置 "Location" 响应头
}

/**
 * 设置一个 Cookie。
 *
 * 在响应中添加一个 Set-Cookie 头部，允许你指定一个 Cookie 的键、值、过期时间、路径、域以及是否仅通过安全连接发送。
 *
 * 参数:
 * - key (const std::string&): Cookie 的键。
 * - val (const std::string&): Cookie 的值。
 * - expired (time_t): Cookie 的过期时间（UNIX 时间戳）。如果大于 0，则设置过期时间。
 * - path (const std::string&): Cookie 的路径。如果非空，则添加到 Cookie 中。
 * - domain (const std::string&): Cookie 的域。如果非空，则添加到 Cookie 中。
 * - secure (bool): 如果为 true，则 Cookie 仅通过安全连接发送。
 *
 * 返回值: 无。
 */
void HttpResponse::setCookie(const std::string& key, const std::string& val,
                             time_t expired, const std::string& path,
                             const std::string& domain, bool secure) {
    std::stringstream ss;
    ss << key << "=" << val; // 添加键值对
    if(expired > 0) {
        // 如果指定了过期时间，将其格式化并添加
        ss << ";expires=" << webserver::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    if(!domain.empty()) {
        ss << ";domain=" << domain; // 添加域
    }
    if(!path.empty()) {
        ss << ";path=" << path; // 添加路径
    }
    if(secure) {
        ss << ";secure"; // 添加安全标记
    }
    m_cookies.push_back(ss.str()); // 将生成的 Cookie 字符串添加到响应的 Cookie 列表中
}

/**
 * 将 HTTP 响应转换为字符串。
 *
 * 使用内部的 dump 方法将响应的各部分（状态行、头部、Cookies 和正文）序列化为一个字符串。
 *
 * 参数: 无。
 *
 * 返回值:
 * - std::string: 表示整个 HTTP 响应的字符串。
 */
std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss); // 调用 dump 方法输出响应的各部分
    return ss.str(); // 返回序列化后的字符串
}

/**
 * 输出 HTTP 响应到流。
 *
 * 将 HTTP 响应的状态行、头部、Cookies 和正文输出到提供的输出流中。如果是 WebSocket 连接，
 * 则忽略 "connection" 头部。此外，根据连接是否保持活动，输出 "connection" 头部的值。
 *
 * 参数:
 * - os (std::ostream&): 要输出到的流。
 *
 * 返回值:
 * - std::ostream&: 输出流的引用，允许链式调用。
 */
std::ostream& HttpResponse::dump(std::ostream& os) const {
    // 输出状态行
    os << "HTTP/"
       << ((uint32_t)(m_version >> 4)) // 主版本号
       << "."
       << ((uint32_t)(m_version & 0x0F)) // 副版本号
       << " "
       << (uint32_t)m_status // 状态码
       << " "
       << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason) // 原因短语
       << "\r\n";

    // 输出头部，忽略 WebSocket 连接的 "connection" 头部
    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    // 输出 Cookies
    for(auto& i : m_cookies) {
        os << "Set-Cookie: " << i << "\r\n";
    }
    // 输出连接状态
    if(!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    // 如果有正文，输出正文长度和正文本身
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

/**
 * 重载 << 运算符以允许直接向流输出 HttpRequest 对象。
 */
std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

/**
 * 重载 << 运算符以允许直接向流输出 HttpResponse 对象。
 */
std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}


}
}

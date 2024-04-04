#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "endian.h"

namespace webserver {

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * @brief CreateMask函数用于创建一个掩码，该掩码在指定位数之前都是1，其余位都是0。
 *          根据前缀长度创建掩码
 * 
 * @tparam T 模板参数T，代表掩码的数据类型。
 * @param bits 要设置为1的位数。
 * @return T 返回一个掩码，其前bits位设置为1，其余位为0。
 * 
 *  sizeof(T) * 8: 算出有多少位
 *  sizeof(T) * 8 - bits: 算出掩码0的个数
 *  1 <<: 将 1 左移0的个数位
 *  -1: 前bits位数置为0，后面的 sizeof(T) * 8 - bits 位都为1
 *  ~: 将前bits位置为1，后面全部置为0
 */
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

/**
 * @brief CountBytes函数用于计算给定值的二进制表示中包含的1的位数。
 *          根据掩码计算前缀长度
 * 
 * @tparam T 模板参数T，代表输入值的数据类型。
 * @param value 要计算的值。
 * @return uint32_t 返回给定值二进制表示中包含的1的位数。
 */
template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0; // 初始化结果为0，用于存储1的位数
    for(; value; ++result) { // 开始迭代，直到value为0
        // 将最右边的1置为0
        value &= value - 1; // 将value与value-1进行按位与操作，相当于将value中的最低位的1清零
    }
    return result; // 返回1的位数
}

/**
 * @brief Address::LookupAny函数用于在给定主机名、协议族、套接字类型和协议的情况下查找可用的地址。
 *          通过host返回任意Address
 * 
 * @param host 要查找的主机名。
 * @param family 协议族，如AF_INET（IPv4）或AF_INET6（IPv6）。
 * @param type 套接字类型，如SOCK_STREAM（流套接字）或SOCK_DGRAM（数据报套接字）。
 * @param protocol 协议，如IPPROTO_TCP（TCP协议）或IPPROTO_UDP（UDP协议）。
 * @return Address::ptr 返回一个地址智能指针，指向查找到的第一个可用地址，如果未找到则返回nullptr。
 */
Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol) {
    std::vector<Address::ptr> result; // 创建一个地址智能指针的vector，用于存储查找到的地址
    if(Lookup(result, host, family, type, protocol)) { // 调用Lookup函数查找地址，并将结果存储在result中
        return result[0]; // 如果成功找到地址，则返回结果中的第一个地址
    }
    return nullptr; // 如果未找到地址，则返回nullptr
}


/**
 * @brief Address::LookupAnyIPAddress 静态成员函数用于查询指定主机名的任意可用IP地址。
 *          通过host返回任意IPAddress
 * 
 * @param host 主机名，表示要查询的主机名。
 * @param family 地址族类型，指定要查询的地址族类型（AF_INET表示IPv4，AF_INET6表示IPv6）。
 * @param type 套接字类型，指定要查询的套接字类型。
 * @param protocol 协议类型，指定要查询的协议类型。
 * @return IPAddress::ptr 返回一个IPv4或IPv6地址智能指针，指向查询到的任意可用IP地址，如果查询失败则返回nullptr。
 */
IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result; // 用于存储查询结果的地址对象列表
    // 调用Lookup函数查询指定主机名的地址信息，并将结果存储到result列表中
    if (Lookup(result, host, family, type, protocol)) {
        // 遍历查询结果列表
        for (auto& i : result) {
            // 尝试将地址对象转换为IPAddress智能指针
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) { // 如果转换成功，表示查询到了IPv4或IPv6地址
                return v; // 返回查询到的IPv4或IPv6地址智能指针
            }
        }
    }
    return nullptr; // 返回nullptr表示查询失败
}



/**
 * @brief Address::Lookup 静态成员函数用于查询指定主机名的地址信息。
 *              通过host地址返回所有Address
 * 
 * @param result 用于存储查询结果的地址对象列表。
 * @param host 主机名，表示要查询的主机名。
 * @param family 地址族类型，指定要查询的地址族类型（AF_INET表示IPv4，AF_INET6表示IPv6）。
 * @param type 套接字类型，指定要查询的套接字类型。
 * @param protocol 协议类型，指定要查询的协议类型。
 * @return bool 查询是否成功，成功返回true，失败返回false。
 */
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    memset(&hints, 0, sizeof(addrinfo)); // 将hints结构体的内存清零

    hints.ai_flags = 0; // 设置查询标志，0表示不进行特殊处理
    hints.ai_family = family; // 设置地址族类型
    hints.ai_socktype = type; // 设置套接字类型
    hints.ai_protocol = protocol; // 设置协议类型
    hints.ai_addrlen = 0; // 初始化地址长度为0
    hints.ai_canonname = NULL; // 初始化规范名称为空指针
    hints.ai_addr = NULL; // 初始化地址信息为空指针
    hints.ai_next = NULL; // 初始化下一个地址信息为空指针

    std::string node; // 存储节点名
    const char* service = NULL; // 存储服务名

    // 检查IPv6地址的格式
    //  host = www.baidu.com:http
    //  检查 ipv6address service  [address]:
    if (!host.empty() && host[0] == '[') {
        // 查找第一个 ] 的位置，返回该指针
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        // 找到了 ]
        if (endipv6) {
            //  是否为 ：
            if (*(endipv6 + 1) == ':') {
                //  endipv6后两个字节为端口号
                service = endipv6 + 2;
            }
            //  地址为[]里的内容
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    // 检查节点名和服务名
    //  ipv4    ip:port
    if (node.empty()) {
        // 找到第一个:
        service = (const char*)memchr(host.c_str(), ':', host.size());
        // 找到了
        if (service) {
            // 后面没有 : 了
            if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                // 拿到地址
                node = host.substr(0, service - host.c_str());
                // ：后面就是端口号
                ++service;
            }
        }
    }
    // 如果没设置端口号，就将host赋给他
    if (node.empty()) {
        node = host;
    }
    
    // 获得地址链表
    // 调用getaddrinfo函数查询地址信息
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    /*
    函数将主机名、主机地址、服务名和端口的字符串表示转换成套接字地址结构体，应用程序只要处理由getaddrinfo函数填写的套接口地址结构。
    1）nodename:节点名可以是主机名，也可以是数字地址。（IPV4的10进点分，或是IPV6的16进制）；
    2）servname:包含十进制数的端口号或服务名如（ftp,http）；
    3）hints:是一个空指针或指向一个addrinfo结构的指针，由调用者填写关于它所想返回的信息类型的线索；
    4）res:存放返回addrinfo结构链表的指针；
    */
    if (error) { // 如果查询失败，则返回false
        WEBSERVER_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << gai_strerror(error); 
            /*  getaddrinfo函数返回的错误码转换为对应的错误信息字符串    */
        return false;
    }

    // results指向头节点，用next遍历
    // 将查询到的地址信息存储到result列表中
    next = results;
    while (next) {
        // 将得到的地址创建出来放到result容器中
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }
    /*  释放addrinfo指针    */
    freeaddrinfo(results); // 释放getaddrinfo返回的结果内存
    return !result.empty(); // 返回查询结果是否为空
}

/**
 * @brief Address::GetInterfaceAddresses 静态成员函数用于获取本地网络接口的地址信息。
 *          返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
 * 
 * @param result 用于存储获取到的网络接口地址信息的multimap容器。
 * @param family 地址族类型，指定要获取的地址族类型（AF_INET表示IPv4，AF_INET6表示IPv6）。
 * @return bool 获取是否成功，成功返回true，失败返回false。
 */
bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result,
                                    int family) {
    struct ifaddrs *next, *results;
    // 调用getifaddrs函数获取本地网络接口的地址信息，并将结果存储到results指针中
    // 函数用于获取系统中所有网络接口的信息。
    /*  getifaddrs创建一个链表，链表上的每个节点都是一个struct ifaddrs结构，getifaddrs()返回链表第一个元素的指针。
    *  成功返回0, 失败返回-1,同时errno会被赋允相应错误码。 */
    if (getifaddrs(&results) != 0) {
        WEBSERVER_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false; // 获取失败，返回false
    }

    try {
        // results指向头节点，用next遍历
        // 遍历获取到的网络接口地址信息列表
        for (next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u; // 默认设置前缀长度为无效值（全1）

            // 如果指定了地址族类型，并且当前网络接口的地址族类型不符合，则跳过该接口
            // 地址族确定 并且 该地址族与解析出来的不同
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }

            // 根据网络接口的地址族类型进行处理
            switch (next->ifa_addr->sa_family) {
                case AF_INET: // 如果是IPv4地址族
                    {
                        // 创建IPv4地址对象，并获取子网掩码，计算前缀长度
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        // 掩码地址
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        // 前缀长度，网络地址的长度
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6: // 如果是IPv6地址族
                    {
                        // 创建IPv6地址对象，并获取子网掩码，计算前缀长度
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        // 掩码地址
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        // 前缀长度，16字节挨个算
                        for (int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }
            //	插入到容器中，保存了网卡名，地址和前缀长度
            if (addr) { // 如果成功创建了地址对象
                // 将网络接口名称及其对应的地址对象和前缀长度插入到result容器中
                result.insert(std::make_pair(next->ifa_name,
                                std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) { // 捕获可能的异常
        WEBSERVER_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        /*  释放ifaddrs */
        freeifaddrs(results); // 释放获取到的网络接口地址信息内存
        return false; // 获取失败，返回false
    }
    /*  释放ifaddrs */
    freeifaddrs(results); // 释放获取到的网络接口地址信息内存
    return !result.empty(); // 返回是否获取到了网络接口地址信息
}


/**
 * @brief Address::GetInterfaceAddresses 静态成员函数用于获取指定网络接口的地址信息。
 *          获取指定网卡的地址和子网掩码位数
 * 
 * @param result 用于存储获取到的网络接口地址信息的vector容器。
 * @param iface 指定的网络接口名称。
 * @param family 地址族类型，指定要获取的地址族类型（AF_INET表示IPv4，AF_INET6表示IPv6）。
 * @return bool 获取是否成功，成功返回true，失败返回false。
 */
bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result,
                                    const std::string& iface, int family) {
    if (iface.empty() || iface == "*") { // 如果未指定网络接口或指定为通配符
        if (family == AF_INET || family == AF_UNSPEC) { // 如果指定了IPv4地址族
            // 创建一个IPv4地址对象，并将其插入到result容器中
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if (family == AF_INET6 || family == AF_UNSPEC) { // 如果指定了IPv6地址族
            // 创建一个IPv6地址对象，并将其插入到result容器中
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true; // 获取成功，返回true
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results; // 用于存储查询结果的multimap容器

    // 调用GetInterfaceAddresses函数获取所有网络接口的地址信息，并将结果存储到results容器中
    if (!GetInterfaceAddresses(results, family)) {
        return false; // 获取失败，返回false
    }

    // 在results容器中查找指定名称的网络接口地址信息，并将结果存储到result容器中
    auto its = results.equal_range(iface);
    for (; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty(); // 返回是否获取到了指定名称的网络接口地址信息
}

// 返回协议簇
int Address::getFamily() const {
    return getAddr()->sa_family;
}
// 返回可读性字符串
std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

/**
 * @brief Address::Create 静态成员函数用于根据给定的sockaddr结构创建相应的地址对象。
 * 
 * @param addr 指向sockaddr结构的指针，表示要创建地址对象的地址信息。
 * @param addrlen sockaddr结构的长度。
 * @return Address::ptr 返回一个Address智能指针，指向根据给定地址信息创建的地址对象，如果地址信息为nullptr，则返回nullptr。
 */
Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
    if(addr == nullptr) { // 如果地址信息为nullptr，则无法创建地址对象，直接返回nullptr
        return nullptr;
    }

    Address::ptr result; // 声明一个Address智能指针，用于存储创建的地址对象

    // 根据地址的地址族类型创建相应类型的地址对象
    switch(addr->sa_family) {
        case AF_INET: // 如果地址族为IPv4
            // 使用IPv4Address构造函数创建IPv4地址对象，并将其赋值给result
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6: // 如果地址族为IPv6
            // 使用IPv6Address构造函数创建IPv6地址对象，并将其赋值给result
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default: // 其他情况，默认为未知地址
            // 使用UnknownAddress构造函数创建未知地址对象，并将其赋值给result
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result; // 返回创建的地址对象指针
}


/**
 * @brief Address::operator< 重载了小于运算符，用于比较两个地址对象的大小。
 * 
 * @param rhs 另一个Address对象，与当前对象进行比较。
 * @return true 如果当前对象小于rhs。
 * @return false 如果当前对象大于等于rhs。
 */
bool Address::operator<(const Address& rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen()); // 获取两个地址的最小长度
    int result = memcmp(getAddr(), rhs.getAddr(), minlen); // 使用内存比较函数memcmp比较两个地址的内容
    if(result < 0) { // 如果当前地址的内容小于rhs的内容
        return true; // 返回true，表示当前对象小于rhs
    } else if(result > 0) { // 如果当前地址的内容大于rhs的内容
        return false; // 返回false，表示当前对象大于rhs
    } else if(getAddrLen() < rhs.getAddrLen()) { // 如果地址内容相等但当前地址的长度小于rhs的长度
        return true; // 返回true，表示当前对象小于rhs
    }
    return false; // 否则返回false，表示当前对象大于等于rhs
}


/**
 * @brief Address::operator== 重载了等于运算符，用于比较两个地址对象是否相等。
 * 
 * @param rhs 另一个Address对象，与当前对象进行比较。
 * @return true 如果两个地址对象相等。
 * @return false 如果两个地址对象不相等。
 */
bool Address::operator==(const Address& rhs) const {
    // 比较两个地址对象的长度和内容是否相等
    return getAddrLen() == rhs.getAddrLen() // 首先比较地址对象的长度是否相等
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0; // 然后比较地址对象的内容是否相等
}


/**
 * @brief Address::operator!= 重载了不等于运算符，用于比较两个地址对象是否不相等。
 * 
 * @param rhs 另一个Address对象，与当前对象进行比较。
 * @return true 如果两个地址对象不相等。
 * @return false 如果两个地址对象相等。
 */
bool Address::operator!=(const Address& rhs) const {
    // 使用等于运算符的结果取反来判断两个地址对象是否不相等
    return !(*this == rhs); // 如果两个地址对象相等，则返回false，否则返回true
}


/**
 * @brief IPAddress::Create 静态成员函数用于根据给定的地址字符串和端口号创建地址对象。
 *          通过IP、域名、服务器名,端口号创建IPAddress
 * 
 * @param address 地址字符串，表示要创建地址对象的地址信息。
 * @param port 端口号，表示要创建地址对象的端口信息。
 * @return IPAddress::ptr 返回一个IPAddress智能指针，指向根据给定地址信息创建的地址对象，如果出现错误则返回nullptr。
 */
IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo)); // 将hints结构体的内存清零，以避免潜在的垃圾值

    hints.ai_flags = AI_NUMERICHOST; // 使用AI_NUMERICHOST标志来指示以数字格式解析地址字符串
    // 协议无关
    hints.ai_family = AF_UNSPEC; // 表示支持任何地址族类型

    // 调用getaddrinfo函数解析地址字符串
    int error = getaddrinfo(address, NULL, &hints, &results);
    if (error) { // 如果解析出错，则返回nullptr
        WEBSERVER_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        // 调用Address::Create函数创建地址对象，并尝试转换为IPAddress智能指针
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if (result) { // 如果成功创建地址对象
            result->setPort(port); // 设置地址对象的端口号
        }
        freeaddrinfo(results); // 释放getaddrinfo返回的结果内存
        return result; // 返回创建的地址对象指针
    } catch (...) { // 捕获可能的异常
        freeaddrinfo(results); // 释放getaddrinfo返回的结果内存
        return nullptr; // 返回nullptr表示创建失败
    }
}


/**
 * @brief IPv4Address::Create 静态成员函数用于根据给定的IPv4地址字符串和端口号创建IPv4地址对象。
 *                      通过IP，端口号创建IPv4Address
 * 
 * @param address IPv4地址字符串，表示要创建地址对象的IPv4地址信息。
 * @param port 端口号，表示要创建地址对象的端口信息。
 * @return IPv4Address::ptr 返回一个IPv4地址智能指针，指向根据给定地址信息创建的IPv4地址对象，如果出现错误则返回nullptr。
 */
IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address); // 创建一个IPv4地址智能指针，用于存储创建的IPv4地址对象
    rt->m_addr.sin_port = byteswapOnLittleEndian(port); // 将端口号按小端字节序转换后赋值给m_addr.sin_port
    
    // 将一个IP地址的字符串表示转换为网络字节序的二进制形式
    // 调用inet_pton函数将IPv4地址字符串转换为网络字节序表示的二进制形式，并存储到m_addr.sin_addr中
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0) { // 如果转换失败，则返回nullptr
        WEBSERVER_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt; // 返回创建的IPv4地址对象指针
}

// 构造函数 通过sockaddr_in
IPv4Address::IPv4Address(const sockaddr_in& address) {
    m_addr = address;
}

/**
 * @brief IPv4Address::IPv4Address 构造函数用于创建IPv4地址对象。
 *          // 通过ip地址和端口号
 * 
 * @param address IPv4地址，以32位无符号整数表示。
 * @param port 端口号，以16位无符号整数表示。
 */
IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr)); // 将m_addr结构体的内存清零，以避免潜在的垃圾值
    m_addr.sin_family = AF_INET; // 设置地址族为IPv4
    m_addr.sin_port = byteswapOnLittleEndian(port); // 将端口号按小端字节序转换后赋值给m_addr.sin_port
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address); // 将IPv4地址按小端字节序转换后赋值给m_addr.sin_addr.s_addr
}


// 返回sockaddr指针
sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&m_addr;
}
// 返回sockaddr指针
const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}
// 返回sockaddr的长度
socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

/**
 * @brief IPv4Address::insert 成员函数用于将IPv4地址信息插入到输出流中。
 *                                            可读性输出地址
 * 
 * @param os 输出流，表示要插入信息的目标流。
 * @return std::ostream& 返回输出流本身，以支持链式调用。
 */
std::ostream& IPv4Address::insert(std::ostream& os) const {
    // 将IPv4地址按小端字节序转换
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    
    // 将IPv4地址的四个字节按格式插入到输出流中
    os << ((addr >> 24) & 0xff) << "." // 输出第一个字节（高位字节）
       << ((addr >> 16) & 0xff) << "." // 输出第二个字节
       << ((addr >> 8) & 0xff) << "."  // 输出第三个字节
       << (addr & 0xff);               // 输出第四个字节（低位字节）
    
    // 插入端口号到输出流中，先将端口号按小端字节序转换
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    
    return os; // 返回输出流本身，以支持链式调用
}


/**
 * @brief IPv4Address::broadcastAddress 成员函数用于计算广播地址。
 * 
 * @param prefix_len 子网前缀长度，表示网络部分的位数。
 * @return IPAddress::ptr 返回一个IPv4地址智能指针，指向计算得到的广播地址，如果prefix_len大于32则返回nullptr。
 */
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32) { // 如果prefix_len大于32，无效的子网前缀长度，无法计算广播地址
        return nullptr; // 返回nullptr表示无效
    }

    // 创建一个新的sockaddr_in结构体，其内容与当前对象的m_addr相同
    sockaddr_in baddr(m_addr);
    // 将广播地址的网络部分的位数设置为prefix_len
    // 主机号全为1
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    
    // 返回新计算得到的广播地址，使用IPv4Address智能指针进行封装
    return IPv4Address::ptr(new IPv4Address(baddr));
}

// 返回网络地址
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    // 主机号全为0
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

/**
 * @brief IPv4Address::subnetMask 成员函数用于计算指定前缀长度的子网掩码。
 * 
 * @param prefix_len 子网前缀长度，表示网络部分的位数。
 * @return IPAddress::ptr 返回一个IPv4地址智能指针，指向计算得到的子网掩码地址。
 */
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet; // 创建一个sockaddr_in结构体，用于存储子网掩码地址
    memset(&subnet, 0, sizeof(subnet)); // 将subnet结构体的内存清零，以避免潜在的垃圾值
    subnet.sin_family = AF_INET; // 设置地址族为IPv4
    
    // 根据前缀长度获得子网掩码
    // 计算子网掩码地址，将IPv4地址的网络部分的前prefix_len位设置为1，其余位设置为0
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    
    // 返回计算得到的子网掩码地址，使用IPv4Address智能指针进行封装
    return IPv4Address::ptr(new IPv4Address(subnet));
}


/**
 * @brief IPv4Address::getPort 成员函数用于获取IPv4地址的端口号。
 * 
 * @return uint32_t 返回以小端字节序表示的端口号。
 */
uint32_t IPv4Address::getPort() const {
    // 返回以小端字节序表示的端口号
    return byteswapOnLittleEndian(m_addr.sin_port);
}

/**
 * @brief IPv4Address::setPort 成员函数用于设置IPv4地址的端口号。
 * 
 * @param v 要设置的端口号，以16位无符号整数表示。
 */
void IPv4Address::setPort(uint16_t v) {
    // 将参数v按小端字节序转换后赋值给m_addr.sin_port
    m_addr.sin_port = byteswapOnLittleEndian(v);
}


/**
 * @brief IPv6Address::Create 静态成员函数用于根据给定的IPv6地址字符串和端口号创建IPv6地址对象。
 * 
 * @param address IPv6地址字符串，表示要创建地址对象的IPv6地址信息。
 * @param port 端口号，表示要创建地址对象的端口信息。
 * @return IPv6Address::ptr 返回一个IPv6地址智能指针，指向根据给定地址信息创建的IPv6地址对象，如果出现错误则返回nullptr。
 */
IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address); // 创建一个IPv6地址智能指针，用于存储创建的IPv6地址对象
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port); // 将端口号按小端字节序转换后赋值给m_addr.sin6_port
    
    // 调用inet_pton函数将IPv6地址字符串转换为网络字节序表示的二进制形式，并存储到m_addr.sin6_addr中
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if (result <= 0) { // 如果转换失败，则返回nullptr
        WEBSERVER_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt; // 返回创建的IPv6地址对象指针
}


IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) {
    m_addr = address;
}

/**
 * @brief IPv6Address::IPv6Address 构造函数用于创建IPv6地址对象。
 * 
 * @param address IPv6地址，以长度为16字节的无符号字节数组表示。
 * @param port 端口号，以16位无符号整数表示。
 */
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr)); // 将m_addr结构体的内存清零，以避免潜在的垃圾值
    m_addr.sin6_family = AF_INET6; // 设置地址族为IPv6
    m_addr.sin6_port = byteswapOnLittleEndian(port); // 将端口号按小端字节序转换后赋值给m_addr.sin6_port
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16); // 将IPv6地址复制到m_addr.sin6_addr.s6_addr
}


sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

/**
 * @brief IPv6Address::insert 成员函数用于将IPv6地址信息插入到输出流中。
 * 
 * @param os 输出流，表示要插入信息的目标流。
 * @return std::ostream& 返回输出流本身，以支持链式调用。
 */
std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "["; // 输出IPv6地址开始的左中括号
    // 两个字节为一块
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr; // 将IPv6地址强制转换为uint16_t指针，便于访问每个16位块
    bool used_zeros = false; // 标志变量，用于表示是否已经输出过连续的0块
    
    // 遍历IPv6地址的16位块
    for(size_t i = 0; i < 8; ++i) {
        // 将连续0块省略
        if(addr[i] == 0 && !used_zeros) { // 如果当前16位块为0且之前未输出过连续的0块，则跳过
            continue;
        }
        // 上一块为0，多输出个:
        if(i && addr[i - 1] == 0 && !used_zeros) { // 如果当前16位块不是第一个非零块且之前未输出过连续的0块，则输出一个冒号
            os << ":"; // 输出冒号分隔符
            // 省略过了，后面不能再省略连续0块了
            used_zeros = true; // 标记为已经输出过连续的0块
        }
        // 每个块后都要输出:
        if(i) { // 如果当前16位块不是第一个块，则输出一个冒号作为分隔符
            os << ":"; // 输出冒号分隔符
        }
        // 按十六进制输出
        // 输出当前16位块的值，使用十六进制表示，并按小端字节序转换
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    // 若最后一块为0省略
    // 如果IPv6地址末尾有连续的0块，且未输出过，则输出双冒号
    if(!used_zeros && addr[7] == 0) {
        os << "::"; // 输出双冒号，表示连续的0块
    }

    // 输出IPv6地址的端口号，使用小端字节序表示
    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port); // 输出IPv6地址结束的右中括号和端口号

    return os; // 返回输出流本身，以支持链式调用
}


/**
 * @brief IPv6Address::broadcastAddress 成员函数用于计算指定前缀长度的IPv6广播地址。
 * 
 * @param prefix_len 子网前缀长度，表示网络部分的位数。
 * @return IPAddress::ptr 返回一个IPv6地址智能指针，指向计算得到的广播地址。
 */
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr); // 创建一个sockaddr_in6结构体，用于存储广播地址
    // 根据指定的前缀长度计算广播地址
    /*	找到前缀长度结尾在第几个字节，在该字节在哪个位置。
     *	将该字节前剩余位置全部置为1	*/
    baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
    // 将网络部分之后的所有字节设置为0xff，表示广播地址
    // 将后面其余字节置为1
    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    // 返回计算得到的IPv6广播地址，使用IPv6Address智能指针进行封装
    return IPv6Address::ptr(new IPv6Address(baddr));
}

/**
 * @brief IPv6Address::networkAddress 成员函数用于计算指定前缀长度的IPv6网络地址。
 * 
 * @param prefix_len 子网前缀长度，表示网络部分的位数。
 * @return IPAddress::ptr 返回一个IPv6地址智能指针，指向计算得到的网络地址。
 */
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr); // 创建一个sockaddr_in6结构体，用于存储网络地址
    // 根据指定的前缀长度计算网络地址，将网络部分之后的所有字节设置为0
    /*	找到前缀长度结尾在第几个字节，在该字节在哪个位置。
     *	将该字节前剩余位置全部置为0	*/
    baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
    // 将后面其余字节置为0
    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    // 返回计算得到的IPv6网络地址，使用IPv6Address智能指针进行封装
    return IPv6Address::ptr(new IPv6Address(baddr));
}

/**
 * @brief IPv6Address::subnetMask 成员函数用于计算指定前缀长度的IPv6子网掩码。
 * 
 * @param prefix_len 子网前缀长度，表示网络部分的位数。
 * @return IPAddress::ptr 返回一个IPv6地址智能指针，指向计算得到的子网掩码地址。
 */
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet; // 创建一个sockaddr_in6结构体，用于存储子网掩码地址
    memset(&subnet, 0, sizeof(subnet)); // 将subnet结构体的内存清零，以避免潜在的垃圾值
    subnet.sin6_family = AF_INET6; // 设置地址族为IPv6

    // 根据指定的前缀长度计算子网掩码地址
    /*	找到前缀长度结尾在第几个字节，在该字节在哪个位置  */
    subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
    // 将前缀长度之前的字节设置为0xff，表示网络部分
    // 将前面的字节全部置为1
    for (uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }

    // 返回计算得到的IPv6子网掩码地址，使用IPv6Address智能指针进行封装
    return IPv6Address::ptr(new IPv6Address(subnet));
}


uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

// Unix域套接字路径名的最大长度。-1是减去'\0'
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

/**
 * @brief UnixAddress::UnixAddress 默认构造函数用于创建一个未初始化的Unix域套接字地址对象。
 */
UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr)); // 将m_addr结构体的内存清零，以避免潜在的垃圾值
    m_addr.sun_family = AF_UNIX; // 设置地址族为AF_UNIX，表示Unix域套接字
    // 计算地址结构体的长度，包括sun_path成员的偏移量和MAX_PATH_LEN（最大路径长度）
    // sun_path的偏移量+最大路径
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

/**
 * @brief UnixAddress::UnixAddress 构造函数用于创建一个指定路径的Unix域套接字地址对象。
 *          通过路径
 * 
 * @param path Unix域套接字的路径。
 */
UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr)); // 将m_addr结构体的内存清零，以避免潜在的垃圾值
    m_addr.sun_family = AF_UNIX; // 设置地址族为AF_UNIX，表示Unix域套接字

    // 加上'\0'的长度
    m_length = path.size() + 1; // 计算路径的长度，并加上字符串结尾的空字符

    // 如果路径不为空且以空字符开头，则长度减去一个空字符位置
    if (!path.empty() && path[0] == '\0') {
        --m_length;
    }

    // 如果路径长度大于地址结构体sun_path成员的最大长度，抛出逻辑错误异常
    if (m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }

    // 将路径复制到地址结构体的sun_path成员中
    // 将path放入结构体
    memcpy(m_addr.sun_path, path.c_str(), m_length);

    // 将地址结构体的长度设置为路径长度加上sun_path成员的偏移量
    // 偏移量+路径长度
    m_length += offsetof(sockaddr_un, sun_path);
}

// 设置sockaddr的长度
void UnixAddress::setAddrLen(uint32_t v) {
    m_length = v;
}
// 返回sockaddr指针
sockaddr* UnixAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}
// 返回sockaddr的长度
socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

std::string UnixAddress::getPath() const {
    std::stringstream ss;
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

/**
 * @brief UnixAddress::insert 成员函数用于将Unix域套接字地址信息插入到输出流中。
 *              可读性输出地址
 * 
 * @param os 输出流，表示要插入信息的目标流。
 * @return std::ostream& 返回输出流本身，以支持链式调用。
 */
std::ostream& UnixAddress::insert(std::ostream& os) const {
    // 检查地址长度是否大于sun_path成员的偏移量，并且sun_path首字符是否为'\0'
    if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
        // 如果是，则输出字符串"\0"以及sun_path成员的后续内容（去除首字符'\0'）
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    // 否则，直接输出sun_path成员的内容
    return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
}

sockaddr* UnknownAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

}

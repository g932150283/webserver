#include "src/address.h"
#include "src/log.h"

webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

void test() {
    std::vector<webserver::Address::ptr> addrs;

    WEBSERVER_LOG_INFO(g_logger) << "begin";
    // bool v = webserver::Address::Lookup(addrs, "localhost:3080");
    bool v = webserver::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // bool v = webserver::Address::Lookup(addrs, "www.sylar.top", AF_INET);
    WEBSERVER_LOG_INFO(g_logger) << "end";
    if(!v) {
        WEBSERVER_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        WEBSERVER_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = webserver::Address::LookupAny("localhost:4080");
    if(addr) {
        WEBSERVER_LOG_INFO(g_logger) << *addr;
    } else {
        WEBSERVER_LOG_ERROR(g_logger) << "error";
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<webserver::Address::ptr, uint32_t> > results;

    bool v = webserver::Address::GetInterfaceAddresses(results);
    if(!v) {
        WEBSERVER_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        WEBSERVER_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

// 通过静态方法获取IPAddress
void test_ip() {
    auto addr = webserver::IPAddress::Create("www.baidu.com");
    if (addr) {
        WEBSERVER_LOG_INFO(g_logger) << addr->toString();
    }
}


// 创建IPv4地址
void test_ipv4() {
    // auto addr = webserver::IPAddress::Create("www.sylar.top");
    auto addr = webserver::IPAddress::Create("127.0.0.8");
    if(addr) {
        WEBSERVER_LOG_INFO(g_logger) << addr->toString();
    }
}

void test_ipv6() {
    auto addr = webserver::IPv6Address::Create("fe80::215:5dff:fe20:e26a");
    if (addr) {
        WEBSERVER_LOG_INFO(g_logger) << addr->toString();
    }
}


int main(int argc, char** argv) {
    // test_ipv4();
    test_ipv6();
    test_ip();
    // test_iface();
    // test();
    return 0;
}

#include"util.h"
#include "log.h"
#include <execinfo.h>
#include"fiber.h"
#include "util.h"
#include <execinfo.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <google/protobuf/unknown_field_set.h>

#include "log.h"
#include "fiber.h"
namespace webserver{

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");
// webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");
// 返回当前线程的ID
pid_t GetThreadId(){
    return syscall(SYS_gettid);    
}

// 返回当前协程的ID
uint32_t GetFiberId(){
    return webserver::Fiber::GetFiberId();
}

// 尽量不要在栈上分配很大的对象，能用指针用指针
/**
 * @brief 捕获当前线程的调用栈。
 * 
 * @param bt 存储调用栈信息的字符串数组。
 * @param size 指定捕获调用栈的最大深度。
 * @param skip 跳过调用栈顶部的指定层数。
 */
void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    // 动态分配内存，用于存储调用栈地址
    void** array = (void**)malloc((sizeof(void*) * size));
    // 获取当前线程的调用栈，返回实际捕获的层数
    size_t s = ::backtrace(array, size);

    // 将调用栈地址转换为人类可读的符号字符串
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) { // 如果转换失败
        WEBSERVER_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) { // 遍历调用栈
        // bt.push_back(demangle(strings[i])); // C++符号需要demangle处理，这里简化为直接添加
        bt.push_back(strings[i]);
    }

    free(strings); // 释放backtrace_symbols分配的内存
    free(array); // 释放存储调用栈地址的内存
}


/**
 * @brief 将当前线程的调用栈转换为字符串。
 * 
 * @param size 指定捕获调用栈的最大深度。
 * @param skip 跳过调用栈顶部的指定层数。
 * @param prefix 每一行调用栈信息前的前缀字符串。
 * @return 调用栈信息的字符串表示。
 */
std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt; // 存储调用栈信息
    Backtrace(bt, size, skip); // 捕获调用栈
    std::stringstream ss; // 使用stringstream组装字符串
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl; // 添加前缀并换行
    }
    return ss.str(); // 返回调用栈信息的字符串
}


uint64_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul  + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul  + tv.tv_usec;
}

std::string Time2Str(time_t ts, const std::string& format) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

time_t Str2Time(const char* str, const char* format) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    if(!strptime(str, format, &t)) {
        return 0;
    }
    return mktime(&t);
}

std::string ToUpper(const std::string& name) {
    std::string rt = name;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
    return rt;
}

std::string ToLower(const std::string& name) {
    std::string rt = name;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
    return rt;
}








}
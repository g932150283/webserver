#include"util.h"
#include "log.h"
#include <execinfo.h>
#include"fiber.cc"
namespace webserver{

// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");
webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");
// 返回当前线程的ID
pid_t GetThreadId(){
    return syscall(SYS_gettid);    
}

// 返回当前协程的ID
uint32_t GetFiberId(){
    return webserver::Fiber::GetFiberId();
}

// 尽量不要在栈上分配很大的对象，能用指针用指针
void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        WEBSERVER_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        // bt.push_back(demangle(strings[i]));
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}










}
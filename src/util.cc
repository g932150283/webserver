#include"util.h"

namespace webserver{

// 返回当前线程的ID
pid_t GetThreadId(){
    return syscall(SYS_gettid);    
}

// 返回当前协程的ID
uint32_t GetFiberId(){
    return 0;
}












}
#ifndef __WEBSERVER_UTIL_H__
#define __WEBSERVER_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>

namespace webserver{

// 返回当前线程的ID
pid_t GetThreadId();

// 返回当前协程的ID
uint32_t GetFiberId();
















}

#endif
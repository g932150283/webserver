
#ifndef __WEBSERVER_MACRO_H__
#define __WEBSERVER_MACRO_H__

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define WEBSERVER_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define WEBSERVER_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define WEBSERVER_LIKELY(x)      (x)
#   define WEBSERVER_UNLIKELY(x)      (x)
#endif

/// 断言宏封装
#define WEBSERVER_ASSERT(x) \
    if(WEBSERVER_UNLIKELY(!(x))) { \
        WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << webserver::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

/// 断言宏封装
#define WEBSERVER_ASSERT2(x, w) \
    if(WEBSERVER_UNLIKELY(!(x))) { \
        WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << webserver::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif

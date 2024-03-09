
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
// 定义一个宏，接受一个参数x，用于进行断言检查。
// 使用WEBSERVER_UNLIKELY优化分支预测，若x为假，即断言失败，则执行大括号内的代码。
// 后接错误发生时的堆栈回溯。
// 使用日志系统记录错误，打印断言失败的表达式。
// 调用BacktraceToString函数生成格式化的堆栈回溯字符串。
// 使用标准断言assert，如果x为假则终止程序。
/**
 * @brief WEBSERVER_ASSERT(x)
功能: 检查一个条件（x）是否为真，并在条件为假时触发断言失败的处理逻辑。
详细实现:
如果条件x为假（即条件检查失败），则执行以下步骤：
记录错误: 使用服务器的日志系统输出一条错误消息，包括断言失败的条件表达式x。
堆栈回溯: 输出当前线程的堆栈回溯信息，以帮助开发者了解错误发生的上下文。
触发断言: 调用assert宏，如果编译时定义了NDEBUG，则无效；否则，如果x为假，将导致程序终止。
 * 
 */
#define WEBSERVER_ASSERT(x) \
    if(WEBSERVER_UNLIKELY(!(x))) { \
        WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << webserver::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

/// 断言宏封装，带有额外信息w
// 定义一个宏，接受两个参数x和w，x用于断言检查，w用于输出额外的调试信息。
// 使用WEBSERVER_UNLIKELY优化分支预测，若x为假，即断言失败，则执行大括号内的代码。
// 使用日志系统记录错误，打印断言失败的表达式。
// 输出额外的调试信息w。
// 后接错误发生时的堆栈回溯。
// 调用BacktraceToString函数生成格式化的堆栈回溯字符串。
// 使用标准断言assert，如果x为假则终止程序。
/**
 * @brief WEBSERVER_ASSERT2(x, w)
功能: 类似于WEBSERVER_ASSERT(x)，不过在触发断言失败的处理逻辑时，除了条件x本身外，还允许开发者提供额外的描述信息w，
以便在日志中输出更多上下文信息。
详细实现:
如果条件x为假（即条件检查失败），则执行以下步骤：
记录错误和额外信息: 使用服务器的日志系统输出一条错误消息，包括断言失败的条件表达式x，以及开发者提供的额外信息w。
堆栈回溯: 输出当前线程的堆栈回溯信息，以帮助开发者了解错误发生的上下文。
触发断言: 调用assert宏，同样，如果x为假，将导致程序终止。
 * 
 */
#define WEBSERVER_ASSERT2(x, w) \
    if(WEBSERVER_UNLIKELY(!(x))) { \
        WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << webserver::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif

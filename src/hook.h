#ifndef __WEBSERVER_HOOK_H__
#define __WEBSERVER_HOOK_H__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/*
在编程领域中，"hook" 是指拦截、修改或者扩展程序中某些特定点的执行流程的技术或方法。
通常，hook 被用于在程序执行某些操作之前或之后注入自定义的代码，以改变程序的行为或者监控程序的执行流程。

进行 hook 操作的原因有多种：

定制行为：通过 hook，可以在程序运行时修改其行为，以满足特定需求或定制功能。
例如，可以 hook 标准库中的某些函数来改变它们的行为，使其符合特定的业务逻辑。

监控和调试：通过 hook，可以监控程序的执行流程并记录关键信息，用于调试或分析程序的运行状况。
例如，可以 hook 文件操作函数来记录文件的读写操作，或者 hook 网络操作函数来监控网络通信。

安全防护：通过 hook，可以增强程序的安全性，防范恶意攻击或者非法操作。
例如，可以 hook 系统调用函数来检测和阻止恶意程序的执行。

性能优化：通过 hook，可以优化程序的性能，例如在关键路径上注入异步操作或者缓存数据，以加速程序的执行速度。

总之，hook 是一种强大的技术手段，可以用于改变程序的行为、监控程序的执行、增强程序的安全性和优化程序的性能。
然而，需要谨慎使用 hook，因为错误的 hook 操作可能会导致程序的异常行为或者安全漏洞。

*/
namespace webserver {
    /**
     * @brief 当前线程是否hook
     */
    bool is_hook_enable();
    /**
     * @brief 设置当前线程的hook状态
     */
    void set_hook_enable(bool flag);
}
/*
将函数接口都存放到extern "C"作用域下，指定函数按照C语言的方式进行编译和链接。
它的作用是为了解决C++中函数名重载的问题，使得C++代码可以和C语言代码进行互操作。
*/
extern "C" {

/* 重新定义同签名的接口 只列举了几个*/
// sleep_fun 为函数指针
//sleep
typedef unsigned int (*sleep_fun)(unsigned int seconds);
// 它是一个sleep_fun类型的函数指针变量，表示该变量在其他文件中已经定义，我们只是在当前文件中引用它。
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

//socket
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

//read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

//write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int s, const void *msg, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

//
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;


/**
 * @brief 带有超时设置的连接函数
 * @param fd 文件描述符
 * @param addr 指向目标地址结构的指针
 * @param addrlen 地址结构的长度
 * @param timeout_ms 连接的超时时间（毫秒）
 * @return 返回连接操作的结果，成功返回 0，失败返回 -1
 */
extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);

}

#endif

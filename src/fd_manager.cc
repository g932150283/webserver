#include "fd_manager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace webserver {

FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {
}

// 初始化文件描述符上下文信息
bool FdCtx::init() {
    // 如果已经初始化过，则直接返回 true
    if (m_isInit) {
        return true;
    }
    
    // 重置接收和发送超时时间为默认值 -1
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    // 用于存储文件描述符的状态信息
    struct stat fd_stat;
    // 获取文件描述符的状态信息
    if (-1 == fstat(m_fd, &fd_stat)) {
        // 如果获取失败，则标记为未初始化，不是套接字
        m_isInit = false;
        m_isSocket = false;
    } else {
        // 如果获取成功，则标记为已初始化，根据文件类型判断是否为套接字
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    // 如果是套接字
    if (m_isSocket) {
        // 获取文件描述符的标志位
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        // 如果文件描述符不是非阻塞模式，则设置为非阻塞模式
        if (!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        // 标记系统态为非阻塞
        m_sysNonblock = true;
    } else {
        // 如果不是套接字，则标记系统态为阻塞
        m_sysNonblock = false;
    }

    // 标记用户态非阻塞标志为 false
    m_userNonblock = false;
    // 标记文件描述符未关闭
    m_isClosed = false;
    // 返回初始化状态
    return m_isInit;
}


void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

// 获取文件描述符上下文信息
FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    // 如果文件描述符为 -1，直接返回空指针
    if (fd == -1) {
        return nullptr;
    }
    // 获取读锁以避免竞态条件
    RWMutexType::ReadLock lock(m_mutex);
    // 如果文件描述符超出当前数据容器大小，且不需要自动创建，则返回空指针
    if ((int)m_datas.size() <= fd) {
        if (auto_create == false) {
            return nullptr;
        }
    } else {
        // 否则，如果文件描述符对应的上下文已存在且不需要自动创建，则直接返回该上下文
        if (m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    // 释放读锁
    lock.unlock();

    // 获取写锁以进行数据修改
    RWMutexType::WriteLock lock2(m_mutex);
    // 创建文件描述符上下文对象
    FdCtx::ptr ctx(new FdCtx(fd));
    // 如果文件描述符大于当前数据容器大小，则扩展容器大小
    if (fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    // 存储文件描述符上下文到数据容器中
    m_datas[fd] = ctx;
    // 返回创建的文件描述符上下文对象
    return ctx;
}

// 删除文件描述符上下文信息
void FdManager::del(int fd) {
    // 获取写锁以进行数据修改
    RWMutexType::WriteLock lock(m_mutex);
    // 如果文件描述符超出当前数据容器大小，直接返回
    if ((int)m_datas.size() <= fd) {
        return;
    }
    // 释放文件描述符对应的上下文对象
    m_datas[fd].reset();
}


}

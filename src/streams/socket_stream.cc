#include "socket_stream.h"
#include "src/util.h"

namespace webserver {

/**
 * 类名：SocketStream
 * 功能：实现对Socket的流操作，包括读取和写入数据，获取远程和本地地址等功能。
 */

/**
 * 构造函数
 * 功能：初始化SocketStream对象
 * 参数：
 *   - sock: Socket类的智能指针，表示要进行流操作的套接字
 *   - owner: bool类型，表示是否拥有该套接字的所有权
 */
SocketStream::SocketStream(Socket::ptr sock, bool owner)
    :m_socket(sock) // 初始化m_socket成员变量为提供的套接字指针
    ,m_owner(owner) { // 初始化m_owner成员变量为提供的布尔值
}

/**
 * 析构函数
 * 功能：清理SocketStream对象
 * 说明：如果对象拥有套接字并且套接字不为空，则关闭套接字。
 */
SocketStream::~SocketStream() {
    if(m_owner && m_socket) { // 如果拥有套接字且套接字不为空
        m_socket->close(); // 关闭套接字
    }
}

/**
 * 检查套接字是否已连接
 * 返回值：bool类型，表示套接字是否已连接
 */
bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected(); // 检查套接字是否不为空且已连接
}

/**
 * 从套接字读取数据
 * 参数：
 *   - buffer: void指针，表示数据缓冲区
 *   - length: size_t类型，表示要读取的数据长度
 * 返回值：int类型，表示读取的字节数，-1表示读取失败
 * 说明：如果套接字未连接，则返回-1；否则从套接字读取数据到缓冲区。
 */
int SocketStream::read(void* buffer, size_t length) {
    if(!isConnected()) { // 如果套接字未连接
        return -1; // 返回读取失败
    }
    return m_socket->recv(buffer, length); // 从套接字读取数据到缓冲区
}

/**
 * 从套接字读取数据到ByteArray对象
 * 参数：
 *   - ba: ByteArray类的智能指针，表示要读取数据的ByteArray对象
 *   - length: size_t类型，表示要读取的数据长度
 * 返回值：int类型，表示读取的字节数，-1表示读取失败
 * 说明：如果套接字未连接，则返回-1；否则从套接字读取数据到ByteArray对象。
 */
int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) { // 如果套接字未连接
        return -1; // 返回读取失败
    }
    std::vector<iovec> iovs; // 创建iovec结构的向量
    ba->getWriteBuffers(iovs, length); // 获取ByteArray对象的写入缓冲区
    int rt = m_socket->recv(&iovs[0], iovs.size()); // 从套接字读取数据到缓冲区
    if(rt > 0) { // 如果成功读取数据
        ba->setPosition(ba->getPosition() + rt); // 更新ByteArray对象的位置
    }
    return rt; // 返回读取的字节数
}

/**
 * 向套接字写入数据
 * 参数：
 *   - buffer: const void指针，表示数据缓冲区
 *   - length: size_t类型，表示要写入的数据长度
 * 返回值：int类型，表示写入的字节数，-1表示写入失败
 * 说明：如果套接字未连接，则返回-1；否则将数据从缓冲区写入到套接字。
 */
int SocketStream::write(const void* buffer, size_t length) {
    if(!isConnected()) { // 如果套接字未连接
        return -1; // 返回写入失败
    }
    return m_socket->send(buffer, length); // 将数据从缓冲区写入到套接字
}

/**
 * 向套接字写入ByteArray对象的数据
 * 参数：
 *   - ba: ByteArray类的智能指针，表示要写入数据的ByteArray对象
 *   - length: size_t类型，表示要写入的数据长度
 * 返回值：int类型，表示写入的字节数，-1表示写入失败
 * 说明：如果套接字未连接，则返回-1；否则将数据从ByteArray对象的读取缓冲区写入到套接字。
 */
int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) { // 如果套接字未连接
        return -1; // 返回写入失败
    }
    std::vector<iovec> iovs; // 创建iovec结构的向量
    ba->getReadBuffers(iovs, length); // 获取ByteArray对象的读取缓冲区
    int rt = m_socket->send(&iovs[0], iovs.size()); // 将数据从缓冲区写入到套接字
    if(rt > 0) { // 如果成功写入数据
        ba->setPosition(ba->getPosition() + rt); // 更新ByteArray对象的位置
    }
    return rt; // 返回写入的字节数
}

/**
 * 关闭套接字
 * 说明：如果套接字不为空，则关闭套接字。
 */
void SocketStream::close() {
    if(m_socket) { // 如果套接字不为空
        m_socket->close(); // 关闭套接字
    }
}

/**
 * 获取远程地址
 * 返回值：Address类的智能指针，表示远程地址，如果套接字为空则返回空指针
 */
Address::ptr SocketStream::getRemoteAddress() {
    if(m_socket) { // 如果套接字不为空
        return m_socket->getRemoteAddress(); // 返回套接字的远程地址
    }
    return nullptr; // 返回空指针
}

/**
 * 获取本地地址
 * 返回值：Address类的智能指针，表示本地地址，如果套接字为空则返回空指针
 */
Address::ptr SocketStream::getLocalAddress() {
    if(m_socket) { // 如果套接字不为空
        return m_socket->getLocalAddress(); // 返回套接字的本地地址
    }
    return nullptr; // 返回空指针
}

/**
 * 获取远程地址的字符串表示形式
 * 返回值：string类型，表示远程地址的字符串表示形式，如果套接字为空则返回空字符串
 */
std::string SocketStream::getRemoteAddressString() {
    auto addr = getRemoteAddress(); // 获取远程地址
    if(addr) { // 如果地址不为空
        return addr->toString(); // 返回远程地址的字符串表示形式
    }
    return ""; // 返回空字符串
}

/**
 * 获取本地地址的字符串表示形式
 * 返回值：string类型，表示本地地址的字符串表示形式，如果套接字为空则返回空字符串
 */
std::string SocketStream::getLocalAddressString() {
    auto addr = getLocalAddress(); // 获取本地地址
    if(addr) { // 如果地址不为空
        return addr->toString(); // 返回本地地址的字符串表示形式
    }
    return ""; // 返回空字符串
}


}

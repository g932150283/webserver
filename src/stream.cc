#include "stream.h"

namespace webserver {

/**
 * @brief 从流中读取固定大小的数据
 * 
 * @param buffer 存放读取数据的缓冲区指针
 * @param length 要读取的数据长度
 * @return int 返回实际读取的数据长度，若出错或到达文件结尾返回负值
 */
int Stream::readFixSize(void* buffer, size_t length) {
    size_t offset = 0; // 已读取数据的偏移量
    int64_t left = length; // 剩余要读取的数据长度
    while(left > 0) { // 循环读取，直到读取完指定长度的数据
        int64_t len = read((char*)buffer + offset, left); // 从流中读取数据
        if(len <= 0) { // 若出错或到达文件结尾，返回
            return len;
        }
        offset += len; // 更新已读取数据的偏移量
        left -= len; // 更新剩余要读取的数据长度
    }
    return length; // 返回实际读取的数据长度
}

/**
 * @brief 从流中读取固定大小的数据，并将数据写入字节数组中
 * 
 * @param ba 要写入数据的字节数组指针
 * @param length 要读取的数据长度
 * @return int 返回实际读取的数据长度，若出错或到达文件结尾返回负值
 */
int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length; // 剩余要读取的数据长度
    while(left > 0) { // 循环读取，直到读取完指定长度的数据
        int64_t len = read(ba, left); // 从流中读取数据，并将数据写入字节数组中
        if(len <= 0) { // 若出错或到达文件结尾，返回
            return len;
        }
        left -= len; // 更新剩余要读取的数据长度
    }
    return length; // 返回实际读取的数据长度
}

/**
 * @brief 向流中写入固定大小的数据
 * 
 * @param buffer 存放要写入数据的缓冲区指针
 * @param length 要写入的数据长度
 * @return int 返回实际写入的数据长度，若出错返回负值
 */
int Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0; // 已写入数据的偏移量
    int64_t left = length; // 剩余要写入的数据长度
    while(left > 0) { // 循环写入，直到写入完指定长度的数据
        int64_t len = write((const char*)buffer + offset, left); // 向流中写入数据
        if(len <= 0) { // 若出错，返回
            return len;
        }
        offset += len; // 更新已写入数据的偏移量
        left -= len; // 更新剩余要写入的数据长度
    }
    return length; // 返回实际写入的数据长度
}

/**
 * @brief 向流中写入固定大小的数据，数据来自字节数组
 * 
 * @param ba 存放要写入数据的字节数组指针
 * @param length 要写入的数据长度
 * @return int 返回实际写入的数据长度，若出错返回负值
 */
int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length; // 剩余要写入的数据长度
    while(left > 0) { // 循环写入，直到写入完指定长度的数据
        int64_t len = write(ba, left); // 向流中写入数据
        if(len <= 0) { // 若出错，返回
            return len;
        }
        left -= len; // 更新剩余要写入的数据长度
    }
    return length; // 返回实际写入的数据长度
}


}

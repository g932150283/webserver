#include "bytearray.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

#include "endian.h"
#include "log.h"

namespace webserver {

static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * Node类的构造函数，根据给定大小创建内存块。
 *
 * @param s 内存块大小
 */
ByteArray::Node::Node(size_t s)
    : ptr(new char[s]), // 使用new分配内存块
      next(nullptr),    // 下一个节点为空
      size(s) {          // 记录内存块大小
}

/**
 * Node类的默认构造函数，初始化为空节点。
 */
ByteArray::Node::Node()
    : ptr(nullptr),     // 内存指针为空
      next(nullptr),    // 下一个节点为空
      size(0) {          // 内存块大小为0
}

/**
 * Node类的析构函数，释放内存块。
 */
ByteArray::Node::~Node() {
    if (ptr) {            // 如果内存指针不为空
        delete[] ptr;     // 释放内存块
    }
}

/**
 * ByteArray类的构造函数，初始化字节数组。
 *
 * @param base_size 基本大小，用于初始化第一个节点的内存块大小
 */
ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size),     // 设置基本大小
      m_position(0),             // 初始化读写位置为0
      m_capacity(base_size),     // 初始容量为基本大小
      m_size(0),                 // 初始大小为0
      m_endian(WEBSERVER_BIG_ENDIAN),  // 设置默认字节序为大端序
      m_root(new Node(base_size)),     // 创建第一个节点
      m_cur(m_root) {                   // 当前节点指向根节点
}

/**
 * ByteArray类的析构函数，释放动态分配的内存。
 */
ByteArray::~ByteArray() {
    Node* tmp = m_root;           // 临时指针指向根节点
    while (tmp) {                 // 循环遍历节点链表
        m_cur = tmp;              // 当前节点指向临时节点
        tmp = tmp->next;          // 临时指针指向下一个节点
        delete m_cur;             // 删除当前节点，释放内存
    }
}


/**
 * 检查当前字节序是否为小端序。
 *
 * @return 如果当前字节序为小端序，则返回true；否则返回false。
 */
bool ByteArray::isLittleEndian() const {
    return m_endian == WEBSERVER_LITTLE_ENDIAN;
}

/**
 * 设置字节序为小端序或大端序。
 *
 * @param val 如果为true，则设置字节序为小端序；否则设置为大端序。
 */
void ByteArray::setIsLittleEndian(bool val) {
    m_endian = (val) ? WEBSERVER_LITTLE_ENDIAN : WEBSERVER_BIG_ENDIAN;
}

/**
 * 写入int8_t类型的数据。
 *
 * @param value 要写入的int8_t数据
 */
void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入uint8_t类型的数据。
 *
 * @param value 要写入的uint8_t数据
 */
void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入int16_t类型的数据。
 *
 * @param value 要写入的int16_t数据
 */
void ByteArray::writeFint16(int16_t value) {
    if (m_endian != WEBSERVER_BYTE_ORDER) {
        value = byteswap(value);  // 如果字节序不匹配，进行字节序转换
    }
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入uint16_t类型的数据。
 *
 * @param value 要写入的uint16_t数据
 */
void ByteArray::writeFuint16(uint16_t value) {
    if (m_endian != WEBSERVER_BYTE_ORDER) {
        value = byteswap(value);  // 如果字节序不匹配，进行字节序转换
    }
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入int32_t类型的数据。
 *
 * @param value 要写入的int32_t数据
 */
void ByteArray::writeFint32(int32_t value) {
    if (m_endian != WEBSERVER_BYTE_ORDER) {
        value = byteswap(value);  // 如果字节序不匹配，进行字节序转换
    }
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入uint32_t类型的数据。
 *
 * @param value 要写入的uint32_t数据
 */
void ByteArray::writeFuint32(uint32_t value) {
    if (m_endian != WEBSERVER_BYTE_ORDER) {
        value = byteswap(value);  // 如果字节序不匹配，进行字节序转换
    }
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入int64_t类型的数据。
 *
 * @param value 要写入的int64_t数据
 */
void ByteArray::writeFint64(int64_t value) {
    if (m_endian != WEBSERVER_BYTE_ORDER) {
        value = byteswap(value);  // 如果字节序不匹配，进行字节序转换
    }
    write(&value, sizeof(value));  // 调用write方法写入数据
}

/**
 * 写入uint64_t类型的数据。
 *
 * @param value 要写入的uint64_t数据
 */
void ByteArray::writeFuint64(uint64_t value) {
    if (m_endian != WEBSERVER_BYTE_ORDER) {
        value = byteswap(value);  // 如果字节序不匹配，进行字节序转换
    }
    write(&value, sizeof(value));  // 调用write方法写入数据
}


/**
 * 将int32_t类型的整数进行Zigzag编码。
 *
 * @param v 要编码的int32_t整数
 * @return 编码后的uint32_t整数
 */
static uint32_t EncodeZigzag32(const int32_t& v) {
    if (v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;  // 负数采用Zigzag编码方式
    } else {
        return v * 2;  // 正数采用Zigzag编码方式
    }
}

/**
 * 将int64_t类型的整数进行Zigzag编码。
 *
 * @param v 要编码的int64_t整数
 * @return 编码后的uint64_t整数
 */
static uint64_t EncodeZigzag64(const int64_t& v) {
    if (v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;  // 负数采用Zigzag编码方式
    } else {
        return v * 2;  // 正数采用Zigzag编码方式
    }
}

/**
 * 将uint32_t类型的整数进行Zigzag解码。
 *
 * @param v 要解码的uint32_t整数
 * @return 解码后的int32_t整数
 */
static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);  // 采用Zigzag解码方式
}

/**
 * 将uint64_t类型的整数进行Zigzag解码。
 *
 * @param v 要解码的uint64_t整数
 * @return 解码后的int64_t整数
 */
static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);  // 采用Zigzag解码方式
}


/**
 * 将int32_t类型的整数以Zigzag编码方式写入字节数组。
 *
 * @param value 要写入的int32_t整数
 */
void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));  // 将整数进行Zigzag编码后写入字节数组
}

/**
 * 将uint32_t类型的整数写入字节数组。
 *
 * @param value 要写入的uint32_t整数
 */
void ByteArray::writeUint32(uint32_t value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;  // 每次取7位数据写入，高位设为1表示还有后续数据
        value >>= 7;
    }
    tmp[i++] = value;  // 最后一个字节不设置高位标志位
    write(tmp, i);  // 将数据写入字节数组
}

/**
 * 将int64_t类型的整数以Zigzag编码方式写入字节数组。
 *
 * @param value 要写入的int64_t整数
 */
void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));  // 将整数进行Zigzag编码后写入字节数组
}

/**
 * 将uint64_t类型的整数写入字节数组。
 *
 * @param value 要写入的uint64_t整数
 */
void ByteArray::writeUint64(uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;  // 每次取7位数据写入，高位设为1表示还有后续数据
        value >>= 7;
    }
    tmp[i++] = value;  // 最后一个字节不设置高位标志位
    write(tmp, i);  // 将数据写入字节数组
}

/**
 * 将float类型的浮点数以uint32_t类型的整数形式写入字节数组。
 *
 * @param value 要写入的float浮点数
 */
void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));  // 将浮点数的字节表示转换为uint32_t整数
    writeFuint32(v);  // 将uint32_t整数以大端序写入字节数组
}

/**
 * 将double类型的浮点数以uint64_t类型的整数形式写入字节数组。
 *
 * @param value 要写入的double浮点数
 */
void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));  // 将浮点数的字节表示转换为uint64_t整数
    writeFuint64(v);  // 将uint64_t整数以大端序写入字节数组
}

/**
 * 将长度不超过65535字节的字符串以uint16_t类型的长度+内容的形式写入字节数组。
 *
 * @param value 要写入的字符串
 */
void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());  // 写入字符串长度
    write(value.c_str(), value.size());  // 写入字符串内容
}

/**
 * 将长度不超过4294967295字节的字符串以uint32_t类型的长度+内容的形式写入字节数组。
 *
 * @param value 要写入的字符串
 */
void ByteArray::writeStringF32(const std::string& value) {
    writeFuint32(value.size());  // 写入字符串长度
    write(value.c_str(), value.size());  // 写入字符串内容
}

/**
 * 将长度不超过18446744073709551615字节的字符串以uint64_t类型的长度+内容的形式写入字节数组。
 *
 * @param value 要写入的字符串
 */
void ByteArray::writeStringF64(const std::string& value) {
    writeFuint64(value.size());  // 写入字符串长度
    write(value.c_str(), value.size());  // 写入字符串内容
}

/**
 * 将长度不限的字符串以uint64_t类型的长度+内容的形式写入字节数组。
 *
 * @param value 要写入的字符串
 */
void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());  // 写入字符串长度
    write(value.c_str(), value.size());  // 写入字符串内容
}

/**
 * 将字符串内容直接写入字节数组，不附加长度信息。
 *
 * @param value 要写入的字符串
 */
void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());  // 写入字符串内容
}


/**
 * 从字节数组中读取一个int8_t类型的数据。
 *
 * @return 读取的int8_t数据
 */
int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

/**
 * 从字节数组中读取一个uint8_t类型的数据。
 *
 * @return 读取的uint8_t数据
 */
uint8_t ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

/**
 * 宏定义，用于简化读取不同类型数据的函数实现。
 */
#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == WEBSERVER_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

/**
 * 从字节数组中读取一个int16_t类型的数据。
 *
 * @return 读取的int16_t数据
 */
int16_t ByteArray::readFint16() {
    XX(int16_t);
}

/**
 * 从字节数组中读取一个uint16_t类型的数据。
 *
 * @return 读取的uint16_t数据
 */
uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

/**
 * 从字节数组中读取一个int32_t类型的数据。
 *
 * @return 读取的int32_t数据
 */
int32_t ByteArray::readFint32() {
    XX(int32_t);
}

/**
 * 从字节数组中读取一个uint32_t类型的数据。
 *
 * @return 读取的uint32_t数据
 */
uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

/**
 * 从字节数组中读取一个int64_t类型的数据。
 *
 * @return 读取的int64_t数据
 */
int64_t ByteArray::readFint64() {
    XX(int64_t);
}

/**
 * 从字节数组中读取一个uint64_t类型的数据。
 *
 * @return 读取的uint64_t数据
 */
uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

/**
 * 从字节数组中读取一个int32_t类型的Zigzag编码数据，并解码为原始数据。
 *
 * @return 解码后的int32_t数据
 */
int32_t ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

/**
 * 从字节数组中读取一个uint32_t类型的长度编码数据。
 *
 * @return 解码后的uint32_t数据
 */
uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

/**
 * 从字节数组中读取一个int64_t类型的Zigzag编码数据，并解码为原始数据。
 *
 * @return 解码后的int64_t数据
 */
int64_t ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

/**
 * 从字节数组中读取一个uint64_t类型的长度编码数据。
 *
 * @return 解码后的uint64_t数据
 */
uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

/**
 * 从字节数组中读取一个float类型的数据。
 *
 * @return 读取的float数据
 */
float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/**
 * 从字节数组中读取一个double类型的数据。
 *
 * @return 读取的double数据
 */
double ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/**
 * 从字节数组中读取一个长度不超过65535字节的字符串。
 *
 * @return 读取的字符串
 */
std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * 从字节数组中读取一个长度不超过4294967295字节的字符串。
 *
 * @return 读取的字符串
 */
std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * 从字节数组中读取一个长度不超过18446744073709551615字节的字符串。
 *
 * @return 读取的字符串
 */
std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

/**
 * 从字节数组中读取一个长度不超过18446744073709551615字节的字符串。
 *
 * @return 读取的字符串
 */
std::string ByteArray::readStringVint() {
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}


/**
 * @brief 清空字节数组的内容
 */
void ByteArray::clear() {
    // 重置位置和大小
    m_position = m_size = 0;
    // 重置容量为基础大小
    m_capacity = m_baseSize;
    // 释放链表中的节点内存
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    // 重置当前节点指针和根节点的下一个指针
    m_cur = m_root;
    m_root->next = NULL;
}

/**
 * @brief 向字节数组中写入数据
 * 
 * @param buf 要写入的数据缓冲区的指针
 * @param size 要写入的数据大小
 */
void ByteArray::write(const void* buf, size_t size) {
    // 如果写入大小为0，则直接返回
    if(size == 0) {
        return;
    }
    // 添加足够的容量以容纳要写入的数据
    addCapacity(size);

    // 计算当前节点内的偏移量、剩余容量和写入缓冲区的偏移量
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;

    // 循环写入数据
    while(size > 0) {
        if(ncap >= size) {
            // 当前节点容量足够，直接写入数据
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            // 如果写入后当前节点满了，则切换到下一个节点
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            // 更新位置、缓冲区偏移量和剩余大小
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            // 当前节点容量不够，先写入当前节点剩余空间，然后切换到下一个节点
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }

    // 更新实际大小
    if(m_position > m_size) {
        m_size = m_position;
    }
}


/**
 * @brief 从字节数组中读取数据
 * 
 * @param buf 目标缓冲区的指针，用于存储读取的数据
 * @param size 要读取的数据大小
 */
void ByteArray::read(void* buf, size_t size) {
    // 如果要读取的大小大于可读取的大小，则抛出异常
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    // 计算当前节点内的偏移量、剩余容量和目标缓冲区的偏移量
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;

    // 循环读取数据
    while(size > 0) {
        if(ncap >= size) {
            // 当前节点容量足够，直接读取数据
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            // 如果读取后当前节点满了，则切换到下一个节点
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            // 更新位置、缓冲区偏移量和剩余大小
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            // 当前节点容量不够，先读取当前节点剩余空间，然后切换到下一个节点
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

/**
 * @brief 从字节数组中指定位置读取数据
 * 
 * @param buf 目标缓冲区的指针，用于存储读取的数据
 * @param size 要读取的数据大小
 * @param position 读取的起始位置
 */
void ByteArray::read(void* buf, size_t size, size_t position) const {
    // 如果要读取的大小大于可读取的剩余大小，则抛出异常
    if(size > (m_size - position)) {
        throw std::out_of_range("not enough len");
    }

    // 计算指定位置的节点内的偏移量、剩余容量和目标缓冲区的偏移量
    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    // 保存当前节点的指针，防止修改 m_cur
    Node* cur = m_cur;

    // 循环读取数据
    while(size > 0) {
        if(ncap >= size) {
            // 当前节点容量足够，直接读取数据
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            // 如果读取后当前节点满了，则切换到下一个节点
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            // 更新位置、缓冲区偏移量和剩余大小
            position += size;
            bpos += size;
            size = 0;
        } else {
            // 当前节点容量不够，先读取当前节点剩余空间，然后切换到下一个节点
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

/**
 * @brief 设置字节数组的当前位置
 * 
 * @param v 要设置的位置值
 */
void ByteArray::setPosition(size_t v) {
    // 如果设置的位置超出了容量范围，则抛出异常
    if(v > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    // 设置当前位置值
    m_position = v;
    // 如果当前位置超过了实际大小，则更新实际大小
    if(m_position > m_size) {
        m_size = m_position;
    }
    // 将当前节点指针重置为根节点
    m_cur = m_root;
    // 找到包含设置位置的节点
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    // 如果设置位置正好是当前节点的末尾，则切换到下一个节点
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}


/**
 * @brief 将字节数组的内容写入文件
 * 
 * @param name 文件名
 * @return 写入成功返回 true，否则返回 false
 */
bool ByteArray::writeToFile(const std::string& name) const {
    // 打开文件并设置为二进制写入模式
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    // 如果打开文件失败，则记录日志并返回 false
    if(!ofs) {
        WEBSERVER_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 获取可读取的数据大小和当前位置
    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_cur;

    // 循环写入数据直到全部写入完毕
    while(read_size > 0) {
        int diff = pos % m_baseSize;
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }

    return true;
}

/**
 * @brief 从文件读取内容到字节数组中
 * 
 * @param name 文件名
 * @return 读取成功返回 true，否则返回 false
 */
bool ByteArray::readFromFile(const std::string& name) {
    // 打开文件并设置为二进制读取模式
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    // 如果打开文件失败，则记录日志并返回 false
    if(!ifs) {
        WEBSERVER_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 创建临时缓冲区，使用智能指针管理内存
    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
    // 循环读取文件内容并写入字节数组中
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

/**
 * @brief 增加字节数组的容量
 * 
 * @param size 要增加的容量大小
 */
void ByteArray::addCapacity(size_t size) {
    // 如果要增加的大小为0，则直接返回
    if(size == 0) {
        return;
    }
    // 获取当前容量
    size_t old_cap = getCapacity();
    // 如果当前容量已经大于等于要增加的大小，则无需增加，直接返回
    if(old_cap >= size) {
        return;
    }

    // 计算实际需要增加的大小
    size = size - old_cap;
    // 计算需要增加的节点数量
    size_t count = ceil(1.0 * size / m_baseSize);
    // 找到链表的末尾节点
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }

    // 创建并连接新的节点，并更新容量
    Node* first = NULL;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }

    // 如果原始容量为0，则将当前节点指针指向第一个新节点
    if(old_cap == 0) {
        m_cur = first;
    }
}


/**
 * @brief 将字节数组转换为字符串
 * 
 * @return 转换后的字符串
 */
std::string ByteArray::toString() const {
    // 创建字符串，并设置大小为可读取的数据大小
    std::string str;
    str.resize(getReadSize());
    // 如果字符串为空，则直接返回空字符串
    if(str.empty()) {
        return str;
    }
    // 从当前位置开始读取数据到字符串中
    read(&str[0], str.size(), m_position);
    return str;
}

/**
 * @brief 将字节数组转换为十六进制字符串
 * 
 * @return 转换后的十六进制字符串
 */
std::string ByteArray::toHexString() const {
    // 将字节数组转换为字符串
    std::string str = toString();
    // 创建字符串流
    std::stringstream ss;

    // 循环遍历字符串，将每个字节转换为十六进制并拼接到字符串流中
    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str(); // 返回拼接好的十六进制字符串
}



/**
 * @brief 获取可读取的数据缓冲区列表
 * 
 * @param buffers 存储可读取的数据缓冲区列表的向量
 * @param len 要获取的数据长度
 * @return 实际获取的数据长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    // 确保要获取的数据长度不超过可读取的数据大小
    len = len > getReadSize() ? getReadSize() : len;
    // 如果要获取的长度为0，则直接返回0
    if(len == 0) {
        return 0;
    }

    // 保存要返回的数据长度
    uint64_t size = len;

    // 计算当前节点内的偏移量和剩余容量
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    // 循环生成可读取的数据缓冲区列表
    while(len > 0) {
        if(ncap >= len) {
            // 当前节点容量足够，设置缓冲区的基地址和长度
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            // 当前节点容量不够，设置缓冲区的基地址和长度，并移动到下一个节点
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov); // 将生成的缓冲区添加到向量中
    }
    return size; // 返回实际获取的数据长度
}

/**
 * @brief 获取可读取的数据缓冲区列表
 * 
 * @param buffers 存储可读取的数据缓冲区列表的向量
 * @param len 要获取的数据长度
 * @param position 要读取数据的起始位置
 * @return 实际获取的数据长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const {
    // 确保要获取的数据长度不超过可读取的数据大小
    len = len > getReadSize() ? getReadSize() : len;
    // 如果要获取的长度为0，则直接返回0
    if(len == 0) {
        return 0;
    }

    // 保存要返回的数据长度
    uint64_t size = len;

    // 计算指定位置的节点内的偏移量和节点数量
    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    // 计算指定位置的节点内的剩余容量
    size_t ncap = cur->size - npos;
    struct iovec iov;
    // 循环生成可读取的数据缓冲区列表
    while(len > 0) {
        if(ncap >= len) {
            // 当前节点容量足够，设置缓冲区的基地址和长度
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            // 当前节点容量不够，设置缓冲区的基地址和长度，并移动到下一个节点
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov); // 将生成的缓冲区添加到向量中
    }
    return size; // 返回实际获取的数据长度
}

/**
 * @brief 获取可写入的数据缓冲区列表
 * 
 * @param buffers 存储可写入的数据缓冲区列表的向量
 * @param len 要获取的数据长度
 * @return 实际获取的数据长度
 */
uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    // 如果要获取的长度为0，则直接返回0
    if(len == 0) {
        return 0;
    }
    // 增加字节数组的容量以容纳要写入的数据
    addCapacity(len);
    // 保存要返回的数据长度
    uint64_t size = len;

    // 计算当前节点内的偏移量和剩余容量
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    // 循环生成可写入的数据缓冲区列表
    while(len > 0) {
        if(ncap >= len) {
            // 当前节点容量足够，设置缓冲区的基地址和长度
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            // 当前节点容量不够，设置缓冲区的基地址和长度，并移动到下一个节点
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov); // 将生成的缓冲区添加到向量中
    }
    return size; // 返回实际获取的数据长度
}


}

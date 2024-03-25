// #include "src/bytearray.h"
// #include "src/webserver.h"

// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();
// void test() {
// #define XX(type, len, write_fun, read_fun, base_len) {\
//     std::vector<type> vec; \
//     for(int i = 0; i < len; ++i) { \
//         vec.push_back(rand()); \
//     } \
//     webserver::ByteArray::ptr ba(new webserver::ByteArray(base_len)); \
//     for(auto& i : vec) { \
//         ba->write_fun(i); \
//     } \
//     ba->setPosition(0); \
//     for(size_t i = 0; i < vec.size(); ++i) { \
//         type v = ba->read_fun(); \
//         WEBSERVER_ASSERT(v == vec[i]); \
//     } \
//     WEBSERVER_ASSERT(ba->getReadSize() == 0); \
//     WEBSERVER_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
//                     " (" #type " ) len=" << len \
//                     << " base_len=" << base_len \
//                     << " size=" << ba->getSize(); \
// }

//     XX(int8_t,  100, writeFint8, readFint8, 1);
//     XX(uint8_t, 100, writeFuint8, readFuint8, 1);
//     XX(int16_t,  100, writeFint16,  readFint16, 1);
//     XX(uint16_t, 100, writeFuint16, readFuint16, 1);
//     XX(int32_t,  100, writeFint32,  readFint32, 1);
//     XX(uint32_t, 100, writeFuint32, readFuint32, 1);
//     XX(int64_t,  100, writeFint64,  readFint64, 1);
//     XX(uint64_t, 100, writeFuint64, readFuint64, 1);

//     XX(int32_t,  100, writeInt32,  readInt32, 1);
//     XX(uint32_t, 100, writeUint32, readUint32, 1);
//     XX(int64_t,  100, writeInt64,  readInt64, 1);
//     XX(uint64_t, 100, writeUint64, readUint64, 1);
// #undef XX

// #define XX(type, len, write_fun, read_fun, base_len) {\
//     std::vector<type> vec; \
//     for(int i = 0; i < len; ++i) { \
//         vec.push_back(rand()); \
//     } \
//     webserver::ByteArray::ptr ba(new webserver::ByteArray(base_len)); \
//     for(auto& i : vec) { \
//         ba->write_fun(i); \
//     } \
//     ba->setPosition(0); \
//     for(size_t i = 0; i < vec.size(); ++i) { \
//         type v = ba->read_fun(); \
//         WEBSERVER_ASSERT(v == vec[i]); \
//     } \
//     WEBSERVER_ASSERT(ba->getReadSize() == 0); \
//     WEBSERVER_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
//                     " (" #type " ) len=" << len \
//                     << " base_len=" << base_len \
//                     << " size=" << ba->getSize(); \
//     ba->setPosition(0); \
//     WEBSERVER_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
//     webserver::ByteArray::ptr ba2(new webserver::ByteArray(base_len * 2)); \
//     WEBSERVER_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
//     ba2->setPosition(0); \
//     WEBSERVER_ASSERT(ba->toString() == ba2->toString()); \
//     WEBSERVER_ASSERT(ba->getPosition() == 0); \
//     WEBSERVER_ASSERT(ba2->getPosition() == 0); \
// }
//     XX(int8_t,  100, writeFint8, readFint8, 1);
//     XX(uint8_t, 100, writeFuint8, readFuint8, 1);
//     XX(int16_t,  100, writeFint16,  readFint16, 1);
//     XX(uint16_t, 100, writeFuint16, readFuint16, 1);
//     XX(int32_t,  100, writeFint32,  readFint32, 1);
//     XX(uint32_t, 100, writeFuint32, readFuint32, 1);
//     XX(int64_t,  100, writeFint64,  readFint64, 1);
//     XX(uint64_t, 100, writeFuint64, readFuint64, 1);

//     XX(int32_t,  100, writeInt32,  readInt32, 1);
//     XX(uint32_t, 100, writeUint32, readUint32, 1);
//     XX(int64_t,  100, writeInt64,  readInt64, 1);
//     XX(uint64_t, 100, writeUint64, readUint64, 1);

// #undef XX
// }

// int main(int argc, char** argv) {
//     test();
//     return 0;
// }


#include "src/bytearray.h"
#include "src/webserver.h"

// 定义全局日志对象
static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// 测试函数
void test() {
    // 定义宏来简化测试代码
#define XX(type, len, write_fun, read_fun, base_len) \
    { \
        std::vector<type> vec; \
        for(int i = 0; i < len; ++i) { \
            vec.push_back(rand()); \
        } \
        webserver::ByteArray::ptr ba(new webserver::ByteArray(base_len)); \
        for(auto& i : vec) { \
            ba->write_fun(i); \
        } \
        ba->setPosition(0); \
        for(size_t i = 0; i < vec.size(); ++i) { \
            type v = ba->read_fun(); \
            WEBSERVER_ASSERT(v == vec[i]); \
        } \
        WEBSERVER_ASSERT(ba->getReadSize() == 0); \
        WEBSERVER_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                        " (" #type " ) len=" << len \
                        << " base_len=" << base_len \
                        << " size=" << ba->getSize(); \
    }

    // 测试不同数据类型的写入和读取操作
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

    // 测试写入和读取数据到文件，并比较文件内容
#undef XX
#define XX(type, len, write_fun, read_fun, base_len) \
    { \
        std::vector<type> vec; \
        for(int i = 0; i < len; ++i) { \
            vec.push_back(rand()); \
        } \
        webserver::ByteArray::ptr ba(new webserver::ByteArray(base_len)); \
        for(auto& i : vec) { \
            ba->write_fun(i); \
        } \
        ba->setPosition(0); \
        for(size_t i = 0; i < vec.size(); ++i) { \
            type v = ba->read_fun(); \
            WEBSERVER_ASSERT(v == vec[i]); \
        } \
        WEBSERVER_ASSERT(ba->getReadSize() == 0); \
        WEBSERVER_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                        " (" #type " ) len=" << len \
                        << " base_len=" << base_len \
                        << " size=" << ba->getSize(); \
        ba->setPosition(0); \
        WEBSERVER_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
        webserver::ByteArray::ptr ba2(new webserver::ByteArray(base_len * 2)); \
        WEBSERVER_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
        ba2->setPosition(0); \
        WEBSERVER_ASSERT(ba->toString() == ba2->toString()); \
        WEBSERVER_ASSERT(ba->getPosition() == 0); \
        WEBSERVER_ASSERT(ba2->getPosition() == 0); \
    }

    // 测试不同数据类型的写入和读取操作
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX
}

// 主函数
int main(int argc, char** argv) {
    test();  // 执行测试函数
    return 0;  // 返回成功状态
}


// #include "src/bytearray.h"
// #include "src/webserver.h"

// // 定义全局日志对象
// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_ROOT();

// // 测试函数
// void test() {
//     // 定义宏来简化测试代码
// #define XX(type, len, write_fun, read_fun, base_len) {\
//     std::vector<type> vec; \  
//     // 创建存储测试数据的向量
//     for(int i = 0; i < len; ++i) { \  
//     // 循环生成测试数据
//         vec.push_back(rand()); \  // 将随机数添加到向量中
//     } \
//     webserver::ByteArray::ptr ba(new webserver::ByteArray(base_len)); \  // 创建 ByteArray 对象指针
//     for(auto& i : vec) { \  // 遍历向量中的数据
//         ba->write_fun(i); \  // 调用 ByteArray 对象的写入函数
//     } \
//     ba->setPosition(0); \  // 将当前读取位置设置为起始位置
//     for(size_t i = 0; i < vec.size(); ++i) { \  // 遍历向量中的数据
//         type v = ba->read_fun(); \  // 从 ByteArray 对象中读取数据
//         WEBSERVER_ASSERT(v == vec[i]); \  // 断言读取的数据与原始数据相等
//     } \
//     WEBSERVER_ASSERT(ba->getReadSize() == 0); \  // 断言读取大小为零
//     WEBSERVER_LOG_INFO(g_logger) << #write_fun "/" #read_fun \  // 记录日志
//                     " (" #type " ) len=" << len \
//                     << " base_len=" << base_len \
//                     << " size=" << ba->getSize(); \
// }

//     // 测试不同数据类型的写入和读取操作
//     XX(int8_t,  100, writeFint8, readFint8, 1);
//     XX(uint8_t, 100, writeFuint8, readFuint8, 1);
//     XX(int16_t,  100, writeFint16,  readFint16, 1);
//     XX(uint16_t, 100, writeFuint16, readFuint16, 1);
//     XX(int32_t,  100, writeFint32,  readFint32, 1);
//     XX(uint32_t, 100, writeFuint32, readFuint32, 1);
//     XX(int64_t,  100, writeFint64,  readFint64, 1);
//     XX(uint64_t, 100, writeFuint64, readFuint64, 1);

//     XX(int32_t,  100, writeInt32,  readInt32, 1);
//     XX(uint32_t, 100, writeUint32, readUint32, 1);
//     XX(int64_t,  100, writeInt64,  readInt64, 1);
//     XX(uint64_t, 100, writeUint64, readUint64, 1);

//     // 测试写入和读取数据到文件，并比较文件内容
// #undef XX
// #define XX(type, len, write_fun, read_fun, base_len) {\
//     std::vector<type> vec; \  // 创建存储测试数据的向量
//     for(int i = 0; i < len; ++i) { \  // 循环生成测试数据
//         vec.push_back(rand()); \  // 将随机数添加到向量中
//     } \
//     webserver::ByteArray::ptr ba(new webserver::ByteArray(base_len)); \  // 创建 ByteArray 对象指针
//     for(auto& i : vec) { \  // 遍历向量中的数据
//         ba->write_fun(i); \  // 调用 ByteArray 对象的写入函数
//     } \
//     ba->setPosition(0); \  // 将当前读取位置设置为起始位置
//     for(size_t i = 0; i < vec.size(); ++i) { \  // 遍历向量中的数据
//         type v = ba->read_fun(); \  // 从 ByteArray 对象中读取数据
//         WEBSERVER_ASSERT(v == vec[i]); \  // 断言读取的数据与原始数据相等
//     } \
//     WEBSERVER_ASSERT(ba->getReadSize() == 0); \  // 断言读取大小为零
//     WEBSERVER_LOG_INFO(g_logger) << #write_fun "/" #read_fun \  // 记录日志
//                     " (" #type " ) len=" << len \
//                     << " base_len=" << base_len \
//                     << " size=" << ba->getSize(); \
//     ba->setPosition(0); \  // 将当前读取位置设置为起始位置
//     WEBSERVER_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \  // 将数据写入文件
//     webserver::ByteArray::ptr ba2(new webserver::ByteArray(base_len * 2)); \  // 创建新的 ByteArray 对象指针
//     WEBSERVER_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \  // 从文件中读取数据
//     ba2->setPosition(0); \  // 将当前读取位置设置为起始位置
//     WEBSERVER_ASSERT(ba->toString() == ba2->toString()); \  // 断言两个 ByteArray 对象的字符串表示相等
//     WEBSERVER_ASSERT(ba->getPosition() == 0); \  // 断言当前读取位置为零
//     WEBSERVER_ASSERT(ba2->getPosition() == 0); \  // 断言当前读取位置为零
// }
//     // 测试不同数据类型的写入和读取操作
//     XX(int8_t,  100, writeFint8, readFint8, 1);
//     XX(uint8_t, 100, writeFuint8, readFuint8, 1);
//     XX(int16_t,  100, writeFint16,  readFint16, 1);
//     XX(uint16_t, 100, writeFuint16, readFuint16, 1);
//     XX(int32_t,  100, writeFint32,  readFint32, 1);
//     XX(uint32_t, 100, writeFuint32, readFuint32, 1);
//     XX(int64_t,  100, writeFint64,  readFint64, 1);
//     XX(uint64_t, 100, writeFuint64, readFuint64, 1);

//     XX(int32_t,  100, writeInt32,  readInt32, 1);
//     XX(uint32_t, 100, writeUint32, readUint32, 1);
//     XX(int64_t,  100, writeInt64,  readInt64, 1);
//     XX(uint64_t, 100, writeUint64, readUint64, 1);

// #undef XX
// }

// // 主函数
// int main(int argc, char** argv) {
//     test();  // 执行测试函数
//     return 0;  // 返回成功状态
// }

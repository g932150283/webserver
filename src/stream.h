/**
 * @file stream.h
 * @brief 流接口
 * @author webserver.yin
 * @email 564628276@qq.com
 * @date 2019-06-06
 * @copyright Copyright (c) 2019年 webserver.yin All rights reserved (www.webserver.top)
 */
#ifndef __WEBSERVER_STREAM_H__
#define __WEBSERVER_STREAM_H__

#include <memory>
#include "bytearray.h"

namespace webserver {

/*
Socket流接口模块概述
基于序列化模块封装了读和写操作，但是socket的API并不保证一定能够写或读到规定的字节数，
所以封装了readFixSize、writeFixSize保证一定操作规定字节的数据。
该模块主要有以下几类：

class Stream：基类，抽象类。
class SocketStream：内部封装一个socket。

*/

/**
 * @brief 流结构
 */
class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;
    /**
     * @brief 析构函数
     */
    virtual ~Stream() {}

    /**
     * @brief 读数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @return
     *      @retval >0 返回接收到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int read(void* buffer, size_t length) = 0;

    /**
     * @brief 读数据
     * @param[out] ba 接收数据的ByteArray
     * @param[in] length 接收数据的内存大小
     * @return
     *      @retval >0 返回接收到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    /**
     * @brief 读固定长度的数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @return
     *      @retval >0 返回接收到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int readFixSize(void* buffer, size_t length);

    /**
     * @brief 读固定长度的数据
     * @param[out] ba 接收数据的ByteArray
     * @param[in] length 接收数据的内存大小
     * @return
     *      @retval >0 返回接收到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    /**
     * @brief 写数据
     * @param[in] buffer 写数据的内存
     * @param[in] length 写入数据的内存大小
     * @return
     *      @retval >0 返回写入到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int write(const void* buffer, size_t length) = 0;

    /**
     * @brief 写数据
     * @param[in] ba 写数据的ByteArray
     * @param[in] length 写入数据的内存大小
     * @return
     *      @retval >0 返回写入到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    /**
     * @brief 写固定长度的数据
     * @param[in] buffer 写数据的内存
     * @param[in] length 写入数据的内存大小
     * @return
     *      @retval >0 返回写入到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int writeFixSize(const void* buffer, size_t length);

    /**
     * @brief 写固定长度的数据
     * @param[in] ba 写数据的ByteArray
     * @param[in] length 写入数据的内存大小
     * @return
     *      @retval >0 返回写入到的数据的实际大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    /**
     * @brief 关闭流
     */
    virtual void close() = 0;
};

}

#endif

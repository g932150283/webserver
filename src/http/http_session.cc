#include "http_session.h"
#include "http_parser.h"

namespace webserver {
namespace http {

/**
 * 类名：HttpSession
 * 功能：实现HTTP会话管理，包括接收HTTP请求和发送HTTP响应等操作。
 * 详细描述：
 *  - 通过继承自SocketStream类，实现对套接字的流操作。
 *  - 提供recvRequest()函数用于接收HTTP请求，通过HttpRequestParser解析HTTP请求数据。
 *  - 提供sendResponse()函数用于发送HTTP响应，将HttpResponse对象转换为字符串后发送。
 */

/**
 * 构造函数
 * 功能：创建HttpSession对象并初始化。
 * 参数：
 *   - sock: 指向Socket对象的智能指针，表示当前会话的套接字。
 *   - owner: bool类型，表示当前对象是否拥有套接字的所有权。
 */
HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) { // 调用基类SocketStream的构造函数，初始化套接字流
}

/**
 * 接收HTTP请求
 * 返回值：指向HttpRequest对象的智能指针，表示接收到的HTTP请求；如果接收失败或解析错误，则返回空指针。
 * 详细描述：
 *  - 创建HttpRequestParser对象用于解析HTTP请求。
 *  - 读取套接字数据并填充缓冲区，调用HttpRequestParser的execute()方法解析HTTP请求。
 *  - 如果解析成功且请求完整，则返回解析得到的HttpRequest对象；否则返回空指针。
 *  - 如果请求中包含消息体，则继续读取消息体数据并填充HttpRequest对象。
 */
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser); // 创建HttpRequestParser对象
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize(); // 获取HTTP请求缓冲区大小
    std::shared_ptr<char> buffer( // 创建共享指针buffer，指向char数组
            new char[buff_size], [](char* ptr){ // 自定义删除器，释放buffer的内存
                delete[] ptr;
            });
    char* data = buffer.get(); // 获取buffer指向的内存地址
    int offset = 0; // 数据偏移量

    do {
        int len = read(data + offset, buff_size - offset); // 从套接字读取数据到缓冲区
        if(len <= 0) { // 如果读取失败或连接断开
            close(); // 关闭套接字
            return nullptr; // 返回空指针
        }
        len += offset; // 更新已读取数据的长度
        size_t nparse = parser->execute(data, len); // 解析HTTP请求
        if(parser->hasError()) { // 如果解析出错
            close(); // 关闭套接字
            return nullptr; // 返回空指针
        }
        offset = len - nparse; // 更新数据偏移量
        if(offset == (int)buff_size) { // 如果数据偏移量达到缓冲区大小
            close(); // 关闭套接字
            return nullptr; // 返回空指针
        }
        if(parser->isFinished()) { // 如果解析完成
            break; // 退出循环
        }
    } while(true);

    int64_t length = parser->getContentLength(); // 获取HTTP请求内容长度
    if(length > 0) { // 如果内容长度大于0
        std::string body; // 创建字符串body
        body.resize(length); // 调整body大小为内容长度

        int len = 0;
        if(length >= offset) { // 如果内容长度大于等于已读取数据长度
            memcpy(&body[0], data, offset); // 将已读取数据复制到body
            len = offset; // 更新已读取数据长度
        } else { // 否则
            memcpy(&body[0], data, length); // 将部分已读取数据复制到body
            len = length; // 更新已读取数据长度
        }
        length -= offset; // 更新剩余内容长度
        if(length > 0) { // 如果剩余内容长度大于0
            if(readFixSize(&body[len], length) <= 0) { // 从套接字继续读取剩余内容
                close(); // 关闭套接字
                return nullptr; // 返回空指针
            }
        }
        parser->getData()->setBody(body); // 设置HTTP请求的消息体
    }

    parser->getData()->init(); // 初始化HTTP请求数据
    return parser->getData(); // 返回HTTP请求数据
}

/**
 * 发送HTTP响应
 * 参数：
 *   - rsp: 指向HttpResponse对象的智能指针，表示要发送的HTTP响应。
 * 返回值：int类型，表示发送的字节数；-1表示发送失败。
 * 详细描述：
 *  - 将HttpResponse对象转换为字符串形式，并通过套接字发送。
 */
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss; // 创建字符串流ss
    ss << *rsp; // 将HTTP响应写入字符串流
    std::string data = ss.str(); // 获取字符串流的字符串表示形式
    return writeFixSize(data.c_str(), data.size()); // 向套接字写入数据并返回发送的字节数
}


}
}

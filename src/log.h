#ifndef _WEB_LOG_H_
#define _WEB_LOG_H_

#include<string>
#include<stdint.h>
#include<memory>

namespace webserver{

// 每个日志生成时定义为一个LogEvent,字段属性都在其中
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent();
private:
    const char* m_file = nullptr; // 文件名
    int32_t m_line = 0;           // 行号
    uint32_t m_threadId = 0;      // 线程ID
    uint32_t m_fiberId = 0;       // 协程ID
    uint64_t m_time = 0;          // 时间戳
    std::string m_concent;        // 消息

};

// 日志级别
class LogLevel{
public:
    enum Level {

    };
};
    
// 每个目的地格式不一致
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;

};

// 日志输出地
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {} // 定义为虚函数，多个输出继承，如果不定义为虚函数，子类使用时可能会出问题
    void log(LogLevel::Level Level, const LogEvent::ptr event);
private:
    LogLevel::Level m_level;
};



// 日志器
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr; // 智能指针方便内存管理
    
    Logger(const std::string & m_name = "root");

    void log(LogLevel::Level Level, const LogEvent::ptr event);
private:
    std::string m_name;
    LogLevel::Level m_level;
    LogAppender::ptr m_formatter;
};




}

#endif
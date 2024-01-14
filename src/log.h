#ifndef _WEB_LOG_H_
#define _WEB_LOG_H_

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
namespace webserver{

// 日志事件
// 每个日志生成时定义为一个LogEvent,字段属性都在其中
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent();
private:
    const char* m_file = nullptr; // 文件名
    int32_t m_line = 0;           // 行号
    uint32_t m_elapse = 0;        // 程序启动开始到现在的毫秒数
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

// 日志格式器
// 每个目的地格式不一致
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    // 返回格式化日志文本
    std::string format(LogEvent::ptr event); // 提供给appender输出
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
// 
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr; // 智能指针方便内存管理
    
    Logger(const std::string & m_name = "root");

    void log(LogLevel::Level Level, const LogEvent::ptr event);

    void debug(LogEvent::ptr event); // 写debug级别日志
    void info(LogEvent::ptr event);  // 写info级别日志
    void warn(LogEvent::ptr event);  // 写warn级别日志
    void error(LogEvent::ptr event); // 写error级别日志
    void fatal(LogEvent::ptr event); // 写fatal级别日志
private:
    std::string m_name;                       // 日志名称
    LogLevel::Level m_level;                  // 满足日志级别的才会被输出
    std::list<LogAppender::ptr> m_appenders;  // 输出的目的地集合
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender {


};

// 输出到文件的Appender
class FileLogAppender : public LogAppender {

};

} // 命名空间
#endif
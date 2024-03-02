#ifndef _WEBSERVER_LOG_H_
#define _WEBSERVER_LOG_H_


#include<string>
#include<stdint.h>
#include<memory>
#include<list>

// 文件
#include <sstream>
#include<fstream>
#include <vector>
#include <map>
#include"singleton.h"

#include"util.h"



// 获取root日志器
#define WEBSERVER_LOG_ROOT() webserver::LoggerMgr::GetInstance()->getRoot()

// 使用流式方式将日志级别level的日志写入到logger
#define WEBSERVER_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        webserver::LogEventWrap(webserver::LogEvent::ptr(new webserver::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, webserver::GetThreadId(),\
                webserver::GetFiberId(), time(0)))).getSS()
// 使用流式方式将日志级别debug的日志写入到logger
#define WEBSERVER_LOG_DEBUG(logger) WEBSERVER_LOG_LEVEL(logger, webserver::LogLevel::DEBUG)
// 使用流式方式将日志级别info的日志写入到logger
#define WEBSERVER_LOG_INFO(logger) WEBSERVER_LOG_LEVEL(logger, webserver::LogLevel::INFO)
// 使用流式方式将日志级别warn的日志写入到logger
#define WEBSERVER_LOG_WARN(logger) WEBSERVER_LOG_LEVEL(logger, webserver::LogLevel::WARN)
// 使用流式方式将日志级别error的日志写入到logger
#define WEBSERVER_LOG_ERROR(logger) WEBSERVER_LOG_LEVEL(logger, webserver::LogLevel::ERROR)
// 使用流式方式将日志级别fatal的日志写入到logger
#define WEBSERVER_LOG_FATAL(logger) WEBSERVER_LOG_LEVEL(logger, webserver::LogLevel::FATAL)

// 使用格式化方式将日志级别level的日志写入到logger
#define WEBSERVER_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        webserver::LogEventWrap(webserver::LogEvent::ptr(new webserver::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, webserver::GetThreadId(), webserver::GetFiberId(), \
                        time(0)))).getEvent()->format(fmt, __VA_ARGS__)

// 使用格式化方式将日志级别debug的日志写入到logger
#define WEBSERVER_LOG_FMT_DEBUG(logger, fmt, ...) WEBSERVER_LOG_FMT_LEVEL(logger, webserver::LogLevel::DEBUG, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别info的日志写入到logger
#define WEBSERVER_LOG_FMT_INFO(logger, fmt, ...)  WEBSERVER_LOG_FMT_LEVEL(logger, webserver::LogLevel::INFO, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别warn的日志写入到logger
#define WEBSERVER_LOG_FMT_WARN(logger, fmt, ...)  WEBSERVER_LOG_FMT_LEVEL(logger, webserver::LogLevel::WARN, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别error的日志写入到logger
#define WEBSERVER_LOG_FMT_ERROR(logger, fmt, ...) WEBSERVER_LOG_FMT_LEVEL(logger, webserver::LogLevel::ERROR, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别fatal的日志写入到logger
#define WEBSERVER_LOG_FMT_FATAL(logger, fmt, ...) WEBSERVER_LOG_FMT_LEVEL(logger, webserver::LogLevel::FATAL, fmt, __VA_ARGS__)

// 获取name的日志器
#define WEBSERVER_LOG_NAME(name) webserver::LoggerMgr::GetInstance()->getLogger(name)

namespace webserver{

class Logger;
class LoggerManager;

// 日志级别
class LogLevel{
public:
    enum Level {
        UNKNOW = 0,// UNKNOW 级别
        DEBUG = 1, // DEBUG 级别
        INFO = 2,  // INFO 级别
        WARN = 3,  // WARN 级别
        ERROR = 4, // ERROR 级别
        FATAL = 5  // FATAL 级别
    };

    // 将日志级别转成文本输出
    static const char* ToString(LogLevel::Level level);
    
    // 将文本转换成日志级别
    static LogLevel::Level FromString(const std::string& str);

};


// 日志事件
// 每个日志生成时定义为一个LogEvent,字段属性都在其中
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    // 返回文件名
    const char* getFile() const { return m_file;}
    // 返回行号
    int32_t getLine() const { return m_line;}
    // 返回耗时
    uint32_t getElapse() const { return m_elapse;}
    // 返回线程ID
    uint32_t getThreadId() const { return m_threadId;}
    // 返回协程ID
    uint32_t getFiberId() const { return m_fiberId;}
    // 返回时间
    uint64_t getTime() const { return m_time;}
    // 返回日志内容
    std::string getContent() const { return m_ss.str();}
    // 返回日志内容字符串流
    std::stringstream& getSS() { return m_ss;}
    // 返回日志器
    std::shared_ptr<Logger>& getLogger() { return m_logger;}
    // 返回日志级别
    LogLevel::Level getLevel() { return m_level;}
    // 格式化写入日志内容
    void format(const char* fmt, ...);
    // 格式化写入日志内容
    void format(const char* fmt, va_list al);
private:
    const char* m_file = nullptr; // 文件名
    int32_t m_line = 0;           // 行号
    uint32_t m_elapse = 0;        // 程序启动开始到现在的毫秒数
    uint32_t m_threadId = 0;      // 线程ID
    uint32_t m_fiberId = 0;       // 协程ID
    uint64_t m_time = 0;          // 时间戳
    std::stringstream m_ss;       // 日志内容流

    std::shared_ptr<Logger> m_logger; // 日志器
    LogLevel::Level m_level;          // 日志级别
};


// 日志事件包装器
class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    // 获取日志事件
    LogEvent::ptr getEvent() const { return m_event;}
    // 获取日志内容流
    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};



// 日志格式器
// 每个目的地格式不一致
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    // 根据pattern赋值解析信息
    LogFormatter(const std::string & pattern);
    // 返回格式化日志文本
    // 提供给appender输出
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event); 

    // 做pattern解析
    void init();

    // 日志解析子模块
    // 日志内容项格式化
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){}
        // 格式化日志到流， 使用ostream做输出
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    // 是否有错误
    bool isError() const { return m_error;}

    // 返回日志模板
    const std::string getPattern() const { return m_pattern;}

private:
    
    // 日志格式模板
    std::string m_pattern;
    // 日志格式解析后格式
    std::vector<FormatItem::ptr> m_items;
    // 是否有错误
    bool m_error = false;

};

// 日志输出地
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;

    // 定义为虚函数，多个输出继承，如果不定义为虚函数，子类使用时可能会出问题
    virtual ~LogAppender() {} 

    // 纯虚函数，子类必须实现该方法
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level Level, const LogEvent::ptr event) = 0; 

    // 将日志输出目标的配置转成YAML String
    virtual std::string toYamlString() = 0;
    
    // 设置日志格式器
    void setFormatter(LogFormatter::ptr val) {m_formatter = val;} 

    // 返回日志格式器
    LogFormatter::ptr getFormatter() const {return m_formatter;}  

    // 设置日志等级
    void setLevel(LogLevel::Level val) {m_level = val;} 

    // 返回日志等级
    LogLevel::Level getLevel() const {return m_level;}  
protected:
    // 虚基类，用到父类属性，修改为protected
    LogLevel::Level m_level = LogLevel::DEBUG;
    // Appender中还需要Format对象，定义输出格式
    LogFormatter::ptr m_formatter;
};



// 日志器
// 
class Logger: public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    // 智能指针方便内存管理
    typedef std::shared_ptr<Logger> ptr; 
    
    Logger(const std::string & name = "root");

    // 写日志
    void log(LogLevel::Level level, const LogEvent::ptr event); 

    // 写debug级别日志
    void debug(LogEvent::ptr event); 

    // 写info级别日志
    void info(LogEvent::ptr event);  

    // 写warn级别日志
    void warn(LogEvent::ptr event);  

    // 写error级别日志
    void error(LogEvent::ptr event); 

    // 写fatal级别日志
    void fatal(LogEvent::ptr event); 

    // 添加日志目标
    void addAppender(LogAppender::ptr appender); 

    // 删除日志目标
    void delAppender(LogAppender::ptr appender); 

    // 清空日志目标
    void clearAppenders();

    // 返回日志级别
    LogLevel::Level getLevel() const {return m_level;}  

    // 设置日志级别
    void setLevel(LogLevel::Level val) {m_level = val;} 

    // 返回日志名称
    const std::string& getName() const { return m_name;}

    // 设置日志格式器
    void setFormatter(LogFormatter::ptr val);

    // 设置日志格式模板
    void setFormatter(const std::string& val);

    // 获取日志格式器
    LogFormatter::ptr getFormatter();

    // 将日志器的配置转成YAML String
    std::string toYamlString();


private:
    // 日志名称
    std::string m_name;            
    // 满足日志级别的才会被输出           
    LogLevel::Level m_level;      
    // 输出的目的地Appender集合            
    std::list<LogAppender::ptr> m_appenders;  
    // 日志格式器
    LogFormatter::ptr m_formatter;
    // 
    Logger::ptr m_root;
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:

    // 智能指针方便内存管理
    typedef std::shared_ptr<StdoutLogAppender> ptr; 
    // 在成员函数声明或定义中，override 说明符确保该函数为虚函数并覆盖某个基类中的虚函数
    // 纯虚函数，子类必须实现该方法
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) override; 
    // 将日志输出目标的配置转成YAML String
    std::string toYamlString() override;
private:
    

};

// 输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    // 智能指针方便内存管理
    typedef std::shared_ptr<FileLogAppender> ptr; 
    FileLogAppender(const std::string& filename);
    // 在成员函数声明或定义中，override 说明符确保该函数为虚函数并覆盖某个基类中的虚函数
    // 纯虚函数，子类必须实现该方法
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) override; 
    // 将日志输出目标的配置转成YAML String
    std::string toYamlString() override;
    //重新打开日志文件，文件的打开成功返回true
    bool reopen(); 
private:
    // 文件以流的方式实现
    // 文件路径
    std::string m_filename;      
    // 文件流    
    std::ofstream m_filestream;  

};



class LoggerManager{
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    // 获取root日志器，等效于getLogger("root")
    Logger::ptr getRoot() { return m_root; }
    // 将日志输出目标的配置转成YAML String
    std::string toYamlString();
private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef webserver::Singleton<LoggerManager> LoggerMgr;

} // 命名空间
#endif
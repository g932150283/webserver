#include "log.h"
#include <iostream>
#include <map>
#include <functional>
#include <time.h>
#include <string.h>
#include <cstdarg>
#include "config.h"
namespace webserver{

// 将日志级别转成文本输出
const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level) {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

// 将文本转换成日志级别
LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}


LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :m_event(e){
    
}

LogEventWrap::~LogEventWrap(){
    // 把自己写进去
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}


std::stringstream& LogEventWrap::getSS(){
    return m_event->getSS();
}

// 格式化写入日志内容
void LogEvent::format(const char* fmt, ...){
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);    
}

// 格式化写入日志内容
void LogEvent::format(const char* fmt, va_list al){
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}


// 消息
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};
// 日志级别
class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};
// 程序启动开始到现在的毫秒数
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};
// 日志名称
class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
        // os << logger->getName();
    }
};
// 线程号
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};
// 协程号
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

// 线程名
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};


// 日志时间
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};
// 文件名
class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};
// 行号
class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};
// 换行
class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};
// 日志内容流
class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};
// Tab
class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string m_string;
};


LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberId(fiber_id)
    ,m_time(time)
    ,m_threadName(thread_name)
    ,m_logger(logger)
    ,m_level(level) {
}

Logger::Logger(const std::string & name) 
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

}


// 添加日志目标
void Logger::addAppender(LogAppender::ptr appender){
    // MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()){
        // MutexType::Lock ll(appender->m_mutex);
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

// 删除日志目标
void Logger::delAppender(LogAppender::ptr appender){
    // MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin(); it != m_appenders.end(); it++){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

// 清空日志目标
void Logger::clearAppenders() {
    // MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

// 设置日志格式器
void Logger::setFormatter(LogFormatter::ptr val) {
    // MutexType::Lock lock(m_mutex);
    m_formatter = val;

    for(auto& i : m_appenders){
        // MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter){
            i->m_formatter = m_formatter;
        }
    }
}

// 设置日志格式模板
void Logger::setFormatter(const std::string& val) {
    webserver::LogFormatter::ptr new_val(new webserver::LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }
    //m_formatter = new_val;
    setFormatter(new_val);
}

// 将日志输出目标的配置转成YAML String
std::string Logger::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

// 获取日志格式器
LogFormatter::ptr Logger::getFormatter() {
    // MutexType::Lock lock(m_mutex);
    return m_formatter;
}

// 写日志
void Logger::log(LogLevel::Level level, const LogEvent::ptr event){
    if(level >= m_level){
        auto self = shared_from_this();
        // MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root) {
            m_root->log(level, event);
        }
    }
}

// 写debug级别日志
void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
} 

// 写info级别日志
void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}  

// 写warn级别日志
void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}  

// 写error级别日志
void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
} 

// 写fatal级别日志
void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
}

// 设置日志格式器
void LogAppender::setFormatter(LogFormatter::ptr val) {
    // MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if(m_formatter){
        m_hasFormatter = true;
    }else{
        m_hasFormatter = false;
    }    
}

// 返回日志格式器
LogFormatter::ptr LogAppender::getFormatter() {
    // MutexType::Lock lock(m_mutex);
    return m_formatter;
}

// 在成员函数声明或定义中，override 说明符确保该函数为虚函数并覆盖某个基类中的虚函数
// 纯虚函数，子类必须实现该方法
void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) {
    if(level >= m_level){
        // MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format(logger, level, event);
    }
}

// 将日志输出目标的配置转成YAML String
std::string StdoutLogAppender::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


FileLogAppender::FileLogAppender(const std::string& filename) : m_filename(filename){
    reopen();
}

// 在成员函数声明或定义中，override 说明符确保该函数为虚函数并覆盖某个基类中的虚函数 
// 纯虚函数，子类必须实现该方法   
void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) {
        if(level >= m_level) {
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        // MutexType::Lock lock(m_mutex);
        if(!(m_filestream << m_formatter->format(logger, level, event))) {
        // if(!m_formatter->format(m_filestream, logger, level, event)) {
            std::cout << "error" << std::endl;
        }
    }
}

// 将日志输出目标的配置转成YAML String
std::string FileLogAppender::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

// 重新打开日志文件，文件的打开成功返回true
bool FileLogAppender::reopen(){
    // MutexType::Lock lock(m_mutex);
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);  // 以追加模式打开文件
    return !!m_filestream;
}

// 根据pattern赋值解析信息
LogFormatter::LogFormatter(const std::string & pattern) : m_pattern(pattern){
    init();
}
// 返回格式化日志文本
// 提供给appender输出
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    std::stringstream ss;
    for(auto& i : m_items){
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

// 做pattern解析
//%xxx %xxx{xxx} %%
void LogFormatter::init(){
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        // 视频中注释下一行
        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager(){
    m_root.reset(new Logger); // 使用 reset 重新分配关联资源
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->m_name] = m_root;

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name){
    // MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}


struct LogAppenderDefine{
    int type = 0; //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine{
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }



};

/*
webserver::ConfigVar 看起来是一个模板类，它接受一个模板参数。这种模板类通常用于创建可配置的变量或对象。
在这种情况下，模板参数是 std::set<LogDefine>，这意味着 webserver::ConfigVar 被实例化为一个存储 std::set<LogDefine> 类型数据的类。
<std::set<LogDefine>> 表示这是一个模板特化，即 webserver::ConfigVar 类被特化为存储 std::set<LogDefine> 类型数据的类。
模板特化允许你为特定类型提供定制化的实现。
g_log_defines 是一个指向 webserver::ConfigVar<std::set<LogDefine>> 类型对象的指针。
webserver::Config::Lookup 看起来是一个静态函数，用于查找名为 "logs" 的配置项，
并返回一个指向 webserver::ConfigVar<std::set<LogDefine>> 类型对象的指针。
该函数可能会在内部使用单例模式或其他方法来确保只有一个实例被创建。
综上所述，这段代码通过模板特化创建了一个存储 std::set<LogDefine> 类型数据的配置变量，
并使用 webserver::Config::Lookup 函数来获取该配置变量的指针。
*/
webserver::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
    webserver::Config::Lookup("logs", std::set<LogDefine>(), "logs config");


template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if(n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }

        if(n["appenders"].IsDefined()) {
            //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for(size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKNOW) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if(!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }

        for(auto& a : i.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if(a.level != LogLevel::UNKNOW) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if(!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};


struct LogIniter {
    /**
 * @brief 构造函数，用于初始化日志系统的配置
 * 
 * 此函数将添加一个监听器，以便在日志配置发生变化时进行相应的处理。
 * 
 * 监听器接受两个参数：旧的日志定义集合和新的日志定义集合，用于比较和处理配置变化。
 * 
 * 当有新的日志定义被添加或者已有的日志定义被修改时，将会根据变化进行相应的操作，包括：
 * - 添加新的 logger
 * - 修改已存在的 logger 的配置
 * - 删除不再存在的 logger
 * 
 * 每个 logger 的配置信息包括：
 * - 日志级别
 * - 日志格式器
 * - 日志输出目标（文件或控制台）
 * 
 * @note 此函数用于初始化日志系统的配置，应在日志系统启动之前调用。
 */
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                    const std::set<LogDefine>& new_value){
            // WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "on_logger_conf_changed";
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                webserver::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增logger
                    logger = WEBSERVER_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = WEBSERVER_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                //std::cout << "** " << i.name << " level=" << i.level
                //<< "  " << logger << std::endl;
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    webserver::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new StdoutLogAppender);
                        // if(!webserver::EnvMgr::GetInstance()->has("d")) {
                        //     ap.reset(new StdoutLogAppender);
                        // } else {
                        //     continue;
                        // }
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = WEBSERVER_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};


//全局对象在main函数之前构造，所以一定触发构造事件
static LogIniter __log_init;

std::string LoggerManager::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


void LoggerManager::init(){
    
}

}
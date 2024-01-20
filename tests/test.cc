#include"../src/log.h"
#include"../src/util.h"
int main(){
    webserver::Logger::ptr logger(new webserver::Logger);
    logger->addAppender(webserver::LogAppender::ptr(new webserver::StdoutLogAppender));

    webserver::FileLogAppender::ptr file_appender(new webserver::FileLogAppender("./log.txt"));

    webserver::LogFormatter::ptr fmt(new webserver::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);

    // 只记录error级别日志
    file_appender->setLevel(webserver::LogLevel::ERROR);

    logger->addAppender(file_appender);


    // webserver::LogEvent::ptr event(
    //     new webserver::LogEvent(logger, webserver::LogLevel::DEBUG, __FILE__, __LINE__,  
    //     0, webserver::GetThreadId(), webserver::GetFiberId(), time(0)));
    // logger->log(webserver::LogLevel::DEBUG, event);

    WEBSERVER_LOG_INFO(logger) << "test macro";
    WEBSERVER_LOG_ERROR(logger) << "test macro error";
    WEBSERVER_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    auto l = webserver::LoggerMgr::GetInstance()->getLogger("xx");
    WEBSERVER_LOG_INFO(l) << "xxx";

    return 0;

}
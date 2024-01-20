#include"../src/log.h"
#include"../src/config.h"

webserver::ConfigVar<int>::ptr g_int_value_config = 
    webserver::Config::Lookup("system.port", (int)8080, "system port");


int main(){

    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << g_int_value_config->getValue();
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << g_int_value_config->toString();

    return 0;
}
#include"src/log.h"
#include"src/config.h"
#include<yaml-cpp/yaml.h>
webserver::ConfigVar<int>::ptr g_int_value_config = 
    webserver::Config::Lookup("system.port", (int)8080, "system port");

webserver::ConfigVar<float>::ptr g_float_value_config = 
    webserver::Config::Lookup("system.value", (float)8080.08, "system value");


void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << std::string(level * 4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}


void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/user/wsl-code/webserver/bin/conf/log.yml");
    print_yaml(root, 0);

    // WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << root;

}


int main(){

    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << g_int_value_config->getValue();
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << g_float_value_config->toString();
    test_yaml();
    return 0;
}
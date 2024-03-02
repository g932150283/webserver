#include"src/log.h"
#include"src/config.h"
#include<yaml-cpp/yaml.h>
#include <iostream>

webserver::ConfigVar<int>::ptr g_int_value_config = 
    webserver::Config::Lookup("system.port", (int)8080, "system port");

webserver::ConfigVar<float>::ptr g_int_valuex_config = 
    webserver::Config::Lookup("system.port", (float)8080, "system port");

webserver::ConfigVar<float>::ptr g_float_value_config = 
    webserver::Config::Lookup("system.value", (float)8080.08, "system value");

webserver::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config =
    webserver::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vec");

webserver::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    webserver::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int list");

webserver::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    webserver::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int set");

webserver::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    webserver::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

webserver::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    webserver::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

webserver::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    webserver::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");


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
    YAML::Node root = YAML::LoadFile("/home/user/wsl-code/webserver/bin/conf/test.yml");
    print_yaml(root, 0);

    // WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << root;

}

void test_config() {
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/user/wsl-code/webserver/bin/conf/test.yml");
    webserver::Config::LoadFromYaml(root);


    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}


class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace webserver {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}


webserver::ConfigVar<Person>::ptr g_person =
    webserver::Config::Lookup("class.person", Person(), "system person");

webserver::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    webserver::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

webserver::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    webserver::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class() {
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "before: " << g_person->getValue().toString() 
                                             << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

    // g_person->addListener(10, [](const Person& old_value, const Person& new_value){
    //     WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "old_value = " << old_value.toString() 
    //         << " new_value = " << new_value.toString();
    // });

    XX_PM(g_person_map, "class.map before");
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/user/wsl-code/webserver/bin/conf/test.yml");
    webserver::Config::LoadFromYaml(root);

    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "after: " << g_person->getValue().toString() 
                                             << " - " << g_person->toString();

    XX_PM(g_person_map, "class.map after");
    WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}


void test_log() {
    // static webserver::Logger::ptr system_log = WEBSERVER_LOG_NAME("system");
    // WEBSERVER_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << webserver::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/user/wsl-code/webserver/bin/conf/log.yml"); ///home/user/wsl-code/webserver/bin/conf/log.yml
    webserver::Config::LoadFromYaml(root);
    std::cout << "=============" << std::endl;
    std::cout << webserver::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    // WEBSERVER_LOG_INFO(system_log) << "hello system" << std::endl;

    // system_log->setFormatter("%d - %m%n");
    // WEBSERVER_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main(){

    // WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << g_int_value_config->getValue();
    // WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << g_float_value_config->toString();
    // test_yaml();
    // test_config();
    
    /*
    2024-01-22 09:27:20     10639           0       [INFO]  [root]  /home/user/wsl-code/webserver/tests/test_config.cc:44   before: 8080
    2024-01-22 09:27:20     10639           0       [INFO]  [root]  /home/user/wsl-code/webserver/tests/test_config.cc:45   before: 8080.08008
    2024-01-22 09:27:20     10639           0       [INFO]  [root]  /home/user/wsl-code/webserver/tests/test_config.cc:51   after: 9900
    2024-01-22 09:27:20     10639           0       [INFO]  [root]  /home/user/wsl-code/webserver/tests/test_config.cc:52   after: 15
    */

    // test_class();
    test_log();
    return 0;
}
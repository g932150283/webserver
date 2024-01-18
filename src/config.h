#ifndef _WEBSERVER_CONFIG_H_
#define _WEBSERVER_CONFIG_H_

#include<memory>
#include <sstream>
#include<string>
// 类型转换
#include <boost/lexical_cast.hpp>

namespace webserver{

/**
 * @brief 配置基类，公用属性
 * 
 * 
 */
class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name)
        ,m_description(description){
    }

    virtual ~ConfigVarBase(){}

    // 返回配置名称
    const std::string& getName() const {return m_name;}
    // 设置配置名称
    void setName(const std::string& val) {m_name = val;}
    // 返回配置描述
    const std::string& getDescription() const {return m_description;}
    // 设置配置描述
    void setDescription(const std::string& val) {m_description = val;}

    // 转换为明文 调试、输出
    virtual std::string toString() = 0;

    // 解析配置，初始化成员信息
    virtual bool fromString(const std::string& val) = 0;

protected:
    // 配置名称
    std::string m_name;
    // 配置描述
    std::string m_description;

private:




};


}

#endif
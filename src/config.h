#ifndef _WEBSERVER_CONFIG_H_
#define _WEBSERVER_CONFIG_H_

#include<memory>
#include <sstream>
#include<string>
// 类型转换
#include <boost/lexical_cast.hpp>
#include"log.h"
#include<yaml-cpp/yaml.h>
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
        // 转换为小写
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
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

/**
 * @brief 类型转换模板类(F 源类型, T 目标类型)
 */
template<class F, class T>
class LexicalCast {
public:
    /**
     * @brief 类型转换
     * @param[in] v 源类型值
     * @return 返回v转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator()(const F& v) {
        return boost::lexical_cast<T>(v);
    }
};


/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
 */
template<class T>
class LexicalCast<std::string, std::vector<T> > {
public:
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 配置参数模板子类,保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */
template <class T, class FromStr = LexicalCast<std::string, T>, 
                    class ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

    /**
     * @brief 通过参数名,参数值,描述构造ConfigVar
     * @param[in] name 参数名称有效字符为[0-9a-z_.]
     * @param[in] default_value 参数的默认值
     * @param[in] description 参数的描述
     */
    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
        : ConfigVarBase(name, description)
        , m_val(default_value) {
    }

    /**
     * @brief 将参数值转换成YAML String
     * @exception 当转换失败抛出异常
     */
    std::string toString() override {
        try {
            // return boost::lexical_cast<std::string>(m_val);
            return ToStr()(m_val);
        } catch (std::exception &e) {
            WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "ConfigVar::toString exception "
                << e.what() << " convert: " <<  typeid(m_val).name() << " to string" ;
                // << e.what() << " convert: " << TypeToName<T>() << " to string" << " name=" << m_name;
        }
        return "";
    }

    /**
     * @brief 从YAML String 转成参数的值
     * @exception 当转换失败抛出异常
     */
    bool fromString(const std::string &val) override {
        try {
            // m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        } catch (std::exception &e) {
            // SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
            //                                   << e.what() << " convert: string to " << TypeToName<T>()
            //                                   << " name=" << m_name
            //                                   << " - " << val;
            WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }

    /**
     * @brief 获取当前参数的值
     */
    const T getValue() { return m_val;}

    // /**
    //  * @brief 获取当前参数的值
    //  */
    // const T getValue() {
    //     RWMutexType::ReadLock lock(m_mutex);
    //     return m_val;
    // }


    /**
     * @brief 设置当前参数的值
     * @details 如果参数的值有发生变化,则通知对应的注册回调函数
     */
    void setValue(const T &v) {m_val = v;}

    // /**
    //  * @brief 设置当前参数的值
    //  * @details 如果参数的值有发生变化,则通知对应的注册回调函数
    //  */
    // void setValue(const T &v) {
    //     {
    //         RWMutexType::ReadLock lock(m_mutex);
    //         if (v == m_val) {
    //             return;
    //         }
    //         for (auto &i : m_cbs) {
    //             i.second(m_val, v);
    //         }
    //     }
    //     RWMutexType::WriteLock lock(m_mutex);
    //     m_val = v;
    // }

    // /**
    //  * @brief 返回参数值的类型名称(typeinfo)
    //  */
    // std::string getTypeName() const override { return TypeToName<T>(); }

    // /**
    //  * @brief 添加变化回调函数
    //  * @return 返回该回调函数对应的唯一id,用于删除回调
    //  */
    // uint64_t addListener(on_change_cb cb) {
    //     static uint64_t s_fun_id = 0;
    //     RWMutexType::WriteLock lock(m_mutex);
    //     ++s_fun_id;
    //     m_cbs[s_fun_id] = cb;
    //     return s_fun_id;
    // }

    // /**
    //  * @brief 删除回调函数
    //  * @param[in] key 回调函数的唯一id
    //  */
    // void delListener(uint64_t key) {
    //     RWMutexType::WriteLock lock(m_mutex);
    //     m_cbs.erase(key);
    // }

    // /**
    //  * @brief 获取回调函数
    //  * @param[in] key 回调函数的唯一id
    //  * @return 如果存在返回对应的回调函数,否则返回nullptr
    //  */
    // on_change_cb getListener(uint64_t key) {
    //     RWMutexType::ReadLock lock(m_mutex);
    //     auto it = m_cbs.find(key);
    //     return it == m_cbs.end() ? nullptr : it->second;
    // }

    // /**
    //  * @brief 清理所有的回调函数
    //  */
    // void clearListener() {
    //     RWMutexType::WriteLock lock(m_mutex);
    //     m_cbs.clear();
    // }

private:
    T m_val;
    //变更回调函数组, uint64_t key,要求唯一，一般可以用hash
    // std::map<uint64_t, on_change_cb> m_cbs;
};

/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar
 */
class Config {
public:
    // typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    // typedef RWMutex RWMutexType;

    /**
     * @brief 获取/创建对应参数名的配置参数,定义时赋值
     * @param[in] name 配置参数名称
     * @param[in] default_value 参数默认值
     * @param[in] description 参数描述
     * @details 获取参数名为name的配置参数,如果存在直接返回
     *          如果不存在,创建参数配置并用default_value赋值
     * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
     * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
     */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name,
        const T &default_value, const std::string &description = "") {
        auto tmp = Lookup<T>(name);
        if (tmp) {
            WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "Lookup name=" << name << " exists";
            return tmp;
        }

        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
            WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = v;
        return v;


        // if (it != s_datas.end()) {
        //     auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        //     if (tmp) {
        //         WEBSERVER_LOG_INFO(WEBSERVER_LOG_ROOT()) << "Lookup name=" << name << " exists";
        //         return tmp;
        //     } else {
        //         SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
        //                                           << TypeToName<T>() << " real_type=" << it->second->getTypeName()
        //                                           << " " << it->second->toString();
        //         return nullptr;
        //     }
        // }

        // if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
        //     SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
        //     throw std::invalid_argument(name);
        // }

        // typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        // GetDatas()[name] = v;
        // return v;
    }

    /**
     * @brief 查找配置参数
     * @param[in] name 配置参数名称
     * @return 返回配置参数名为name的配置参数
     */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        auto it = s_datas.find(name);
        if (it == s_datas.end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 使用YAML::Node初始化配置模块
     */
    static void LoadFromYaml(const YAML::Node& root);

    // /**
    //  * @brief 加载path文件夹里面的配置文件
    //  */
    // static void LoadFromConfDir(const std::string &path, bool force = false);

    /**
     * @brief 查找配置参数,返回配置参数的基类
     * @param[in] name 配置参数名称
     */
    static ConfigVarBase::ptr LookupBase(const std::string &name);

    // /**
    //  * @brief 遍历配置模块里面所有配置项
    //  * @param[in] cb 配置项回调函数
    //  */
    // static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:

    static ConfigVarMap s_datas;

    // /**
    //  * @brief 返回所有的配置项
    //  */
    // static ConfigVarMap &GetDatas() {
    //     static ConfigVarMap s_datas;
    //     return s_datas;
    // }

    // /**
    //  * @brief 配置项的RWMutex
    //  */
    // static RWMutexType &GetMutex() {
    //     static RWMutexType s_mutex;
    //     return s_mutex;
    // }
};


}

#endif
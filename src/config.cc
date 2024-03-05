#include"config.h"

namespace webserver{

// Config::ConfigVarMap Config::s_datas;
// static webserver::Logger::ptr g_logger = WEBSERVER_LOG_NAME("system");

/**
 * @brief ConfigVarBase类指针的声明，命名为ptr
 * 
 * @param name 配置参数名称
 * @return ConfigVarBase::ptr 
 */
ConfigVarBase::ptr Config::LookupBase(const std::string& name) {

    RWMutexType::ReadLock lock(GetMutex());
    // 在映射中查找具有给定名称的ConfigVarBase
    auto it = GetDatas().find(name);

    // 如果未找到ConfigVarBase，则返回nullptr
    // 否则返回找到的ConfigVarBase
    return it == GetDatas().end() ? nullptr : it->second;
}


//"A.B", 10
//A:
//  B: 10
//  C: str
/**
 * @brief 列出YAML节点的所有成员以及它们的前缀
 * 
 * @param prefix YAML节点的当前前缀
 * @param node 正在处理的YAML节点
 * @param output 存储前缀和相应节点的对的列表
 */
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) {
    // 检查前缀是否包含无效字符
    if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
        // 如果前缀包含无效字符，则记录错误
        // WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "配置无效名称：" << prefix << " : " << node;
        WEBSERVER_LOG_ERROR(WEBSERVER_LOG_ROOT()) << "config invalid name: " << prefix << " : " << node;
        return;
    }

    // 将当前节点及其前缀添加到输出列表
    output.push_back(std::make_pair(prefix, node));

    // 如果当前节点是一个映射，遍历其键值对
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            // 对每个键值对递归调用该函数
            // 如果前缀为空，则使用键作为新前缀；否则，将前缀和键拼接在一起
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}


/**
 * @brief 从YAML节点加载配置信息
 * 
 * @param root 
 */
void Config::LoadFromYaml(const YAML::Node& root) {
    // 存储所有节点及其前缀的列表
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    
    // 列出YAML节点的所有成员及其前缀
    ListAllMember("", root, all_nodes);

    // 遍历所有节点
    for (auto& i : all_nodes) {
        std::string key = i.first;

        // 跳过空键
        if (key.empty()) {
            continue;
        }

        // 将键转换为小写
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // 查找配置变量
        ConfigVarBase::ptr var = LookupBase(key);

        // 如果找到配置变量
        if (var) {
            // 如果节点是标量，则使用标量值设置配置变量
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } 
            // 如果节点不是标量，则将节点的内容转换为字符串并设置配置变量
            else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

// void Config::LoadFromConfDir(const std::string& path, bool force) {
//     std::string absoulte_path = webserver::EnvMgr::GetInstance()->getAbsolutePath(path);
//     std::vector<std::string> files;
//     FSUtil::ListAllFile(files, absoulte_path, ".yml");

//     for(auto& i : files) {
//         {
//             struct stat st;
//             lstat(i.c_str(), &st);
//             webserver::Mutex::Lock lock(s_mutex);
//             if(!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
//                 continue;
//             }
//             s_file2modifytime[i] = st.st_mtime;
//         }
//         try {
//             YAML::Node root = YAML::LoadFile(i);
//             LoadFromYaml(root);
//             WEBSERVER_LOG_INFO(g_logger) << "LoadConfFile file="
//                 << i << " ok";
//         } catch (...) {
//             WEBSERVER_LOG_ERROR(g_logger) << "LoadConfFile file="
//                 << i << " failed";
//         }
//     }
// }

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin();
            it != m.end(); ++it) {
        cb(it->second);
    }

}


}
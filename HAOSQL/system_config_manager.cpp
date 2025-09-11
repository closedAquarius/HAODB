#include "system_config_manager.h"
#include <iostream>
#include <fstream>
#include <cstring>

SystemConfigManager::SystemConfigManager(const std::string& file_path)
    : config_file_path(file_path) {
}

bool SystemConfigManager::LoadConfig() 
{
    std::ifstream file(config_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cout << "配置文件不存在，创建默认配置" << std::endl;
        return CreateDefaultConfig();
    }

    file.read(reinterpret_cast<char*>(&config), sizeof(SystemConfig));

    // 验证文件格式
    if (strncmp(config.magic, "HAOSYSCF", 8) != 0) 
    {
        std::cerr << "无效的配置文件格式" << std::endl;
        return false;
    }

    file.close();
    std::cout << "系统配置加载成功" << std::endl;
    return true;
}

bool SystemConfigManager::SaveConfig() 
{
    std::ofstream file(config_file_path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "无法写入配置文件: " << config_file_path << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&config), sizeof(SystemConfig));
    file.close();

    std::cout << "系统配置保存成功" << std::endl;
    return true;
}

void SystemConfigManager::SetMaxDatabases(uint32_t max_db) 
{
    config.max_databases = max_db;
    std::cout << "最大数据库数量设置为: " << max_db << std::endl;
}

void SystemConfigManager::SetMaxConnections(uint32_t max_connections) 
{
    config.max_connections_total = max_connections;
    std::cout << "最大连接数设置为: " << max_connections << std::endl;
}

void SystemConfigManager::SetGlobalBufferSize(uint32_t buffer_size)
{
    config.global_buffer_size = buffer_size;
    std::cout << "全局缓冲区大小设置为: " << buffer_size << " bytes" << std::endl;
}

void SystemConfigManager::SetCheckpointInterval(uint32_t interval) 
{
    config.checkpoint_interval = interval;
    std::cout << "检查点间隔设置为: " << interval << " 秒" << std::endl;
}

void SystemConfigManager::SetSessionTimeout(uint32_t timeout)
{
    config.session_timeout = timeout;
    std::cout << "会话超时设置为: " << timeout << " 秒" << std::endl;
}

void SystemConfigManager::SetSystemPaths(const std::string& log_path,
    const std::string& temp_path,
    const std::string& backup_path)
{
    strcpy(config.system_log_path, log_path.c_str());
    strcpy(config.temp_path, temp_path.c_str());
    strcpy(config.backup_path, backup_path.c_str());
    std::cout << "系统路径配置更新" << std::endl;
}

void SystemConfigManager::EnableQueryCache(bool enable) 
{
    config.enable_query_cache = enable ? 1 : 0;
    std::cout << "查询缓存" << (enable ? "启用" : "禁用") << std::endl;
}

void SystemConfigManager::EnableAuthentication(bool enable) 
{
    config.enable_authentication = enable ? 1 : 0;
    std::cout << "身份验证" << (enable ? "启用" : "禁用") << std::endl;
}

void SystemConfigManager::EnableAuditLog(bool enable) 
{
    config.enable_audit_log = enable ? 1 : 0;
    std::cout << "审计日志" << (enable ? "启用" : "禁用") << std::endl;
}

void SystemConfigManager::ShowConfig() 
{
    std::cout << "\n=== 系统配置 ===" << std::endl;
    std::cout << "版本: " << config.version << std::endl;
    std::cout << "最大数据库数: " << config.max_databases << std::endl;
    std::cout << "最大连接数: " << config.max_connections_total << std::endl;
    std::cout << "默认页面大小: " << config.default_page_size << " bytes" << std::endl;
    std::cout << "默认字符集: " << config.default_charset << std::endl;
    std::cout << "全局缓冲区: " << config.global_buffer_size << " bytes" << std::endl;
    std::cout << "检查点间隔: " << config.checkpoint_interval << " 秒" << std::endl;
    std::cout << "查询缓存: " << (config.enable_query_cache ? "启用" : "禁用") << std::endl;
    std::cout << "身份验证: " << (config.enable_authentication ? "启用" : "禁用") << std::endl;
}

bool SystemConfigManager::CreateDefaultConfig() 
{
    config = SystemConfig();  // 使用默认构造函数
    // 设置默认路径
    strcpy(config.system_log_path, "C:\\HAODB\\system\\logs\\");
    strcpy(config.temp_path, "C:\\HAODB\\temp\\");
    strcpy(config.backup_path, "C:\\HAODB\\backup\\");
    return SaveConfig();
}
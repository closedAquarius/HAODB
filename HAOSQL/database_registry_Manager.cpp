#define _CRT_SECURE_NO_WARNINGS
#include "database_registry_manager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>

DatabaseRegistryManager::DatabaseRegistryManager(const std::string& file_path)
    : registry_file_path(file_path) {
}

bool DatabaseRegistryManager::LoadRegistry() 
{
    std::ifstream file(registry_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cout << "注册表文件不存在，创建新的注册表" << std::endl;
        return CreateNewRegistry();
    }

    // 读取头部信息
    file.read(reinterpret_cast<char*>(&registry), sizeof(DatabaseRegistry));

    // 验证文件格式
    if (strncmp(registry.magic, "HAODBSYS", 8) != 0) 
    {
        std::cerr << "无效的注册表文件格式" << std::endl;
        return false;
    }

    // 读取数据库条目
    databases.clear();
    databases.resize(registry.database_count);

    for (uint32_t i = 0; i < registry.database_count; i++)
    {
        file.read(reinterpret_cast<char*>(&databases[i]), sizeof(DatabaseEntry));
    }

    file.close();
    std::cout << "成功加载 " << registry.database_count << " 个数据库注册信息" << std::endl;
    return true;
}

bool DatabaseRegistryManager::SaveRegistry()
{
    std::ofstream file(registry_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "无法写入注册表文件: " << registry_file_path << std::endl;
        return false;
    }

    // 更新头部信息
    registry.database_count = databases.size();
    registry.last_update = static_cast<uint64_t>(time(nullptr));

    // 写入头部
    file.write(reinterpret_cast<const char*>(&registry), sizeof(DatabaseRegistry));

    // 写入数据库条目
    for (const auto& db : databases) 
    {
        file.write(reinterpret_cast<const char*>(&db), sizeof(DatabaseEntry));
    }

    file.close();
    std::cout << "注册表保存成功" << std::endl;
    return true;
}

bool DatabaseRegistryManager::RegisterDatabase(const std::string& db_name,
    const std::string& db_path,
    uint32_t db_id,
    const std::string& owner) 
{
    // 检查是否已存在
    for (const auto& db : databases) 
    {
        if (strcmp(db.db_name, db_name.c_str()) == 0) 
        {
            std::cerr << "数据库 " << db_name << " 已存在" << std::endl;
            return false;
        }
    }

    // 创建新条目
    DatabaseEntry new_entry;
    strcpy(new_entry.db_name, db_name.c_str());
    strcpy(new_entry.db_path, db_path.c_str());
    new_entry.db_id = db_id;
    new_entry.status = 1;  // 在线
    new_entry.create_time = static_cast<uint64_t>(time(nullptr));
    new_entry.last_access = new_entry.create_time;
    strcpy(new_entry.owner, owner.c_str());
    new_entry.auto_start = 1;

    databases.push_back(new_entry);

    std::cout << "数据库 " << db_name << " 注册成功" << std::endl;
    return SaveRegistry();
}

bool DatabaseRegistryManager::UnregisterDatabase(const std::string& db_name)
{
    auto it = std::find_if(databases.begin(), databases.end(),
        [&](const DatabaseEntry& db) 
        {
            return strcmp(db.db_name, db_name.c_str()) == 0;
        });

    if (it != databases.end()) 
    {
        databases.erase(it);
        std::cout << "数据库 " << db_name << " 注销成功" << std::endl;
        return SaveRegistry();
    }

    std::cerr << "未找到数据库: " << db_name << std::endl;
    return false;
}

bool DatabaseRegistryManager::UpdateDatabaseStatus(const std::string& db_name, uint8_t status) {
    for (auto& db : databases) 
    {
        if (strcmp(db.db_name, db_name.c_str()) == 0)
        {
            db.status = status;
            db.last_access = static_cast<uint64_t>(time(nullptr));
            return SaveRegistry();
        }
    }
    return false;
}

DatabaseEntry* DatabaseRegistryManager::GetDatabaseInfo(const std::string& db_name) {
    for (auto& db : databases) 
    {
        if (strcmp(db.db_name, db_name.c_str()) == 0)
        {
            return &db;
        }
    }
    return nullptr;
}

void DatabaseRegistryManager::ListDatabases() 
{
    std::cout << "\n=== 数据库注册表 ===" << std::endl;
    std::cout << "总数据库数量: " << databases.size() << std::endl;

    for (const auto& db : databases) 
    {
        std::cout << "ID: " << db.db_id
            << ", 名称: " << db.db_name
            << ", 状态: " << (db.status == 1 ? "在线" : "离线")
            << ", 所有者: " << db.owner << std::endl;
    }
}

bool DatabaseRegistryManager::CreateNewRegistry() 
{
    registry = DatabaseRegistry();  // 使用默认构造函数
    databases.clear();
    return SaveRegistry();
}
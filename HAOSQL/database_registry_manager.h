#pragma once
#include "metadata_structures.h"
#include <string>
#include <vector>

class DatabaseRegistryManager 
{
private:
    std::string registry_file_path;
    DatabaseRegistry registry;
    std::vector<DatabaseEntry> databases;

    bool CreateNewRegistry();

public:
    explicit DatabaseRegistryManager(const std::string& file_path);

    // 文件操作
    bool LoadRegistry();
    bool SaveRegistry();

    // 数据库管理
    bool RegisterDatabase(const std::string& db_name, const std::string& db_path,
        uint32_t db_id, const std::string& owner);
    bool UnregisterDatabase(const std::string& db_name);
    bool UpdateDatabaseStatus(const std::string& db_name, uint8_t status);

    // 查询接口
    DatabaseEntry* GetDatabaseInfo(const std::string& db_name);
    void ListDatabases();

    // 获取注册表信息
    const DatabaseRegistry& GetRegistry() const { return registry; }
    const std::vector<DatabaseEntry>& GetDatabases() const { return databases; }
};

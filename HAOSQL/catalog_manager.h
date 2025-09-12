#pragma once
#include "system_config_manager.h"
#include "database_registry_manager.h"
#include "database_metadata_manager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// 列定义结构
struct ColumnDefinition {
    std::string column_name;
    std::string data_type;    // "INT", "VARCHAR", "DECIMAL", "DATETIME"
    uint32_t max_length;
    bool is_primary_key;
    bool is_nullable;
    bool is_unique;
    std::string default_value;
    uint32_t column_offset;

    ColumnDefinition(const std::string& name, const std::string& type, uint32_t length)
        : column_name(name), data_type(type), max_length(length),
        is_primary_key(false), is_nullable(true), is_unique(false),column_offset(0) {
    }
};

// 数据库信息结构
struct DatabaseInfo {
    std::string db_name;
    std::string db_path;
    uint32_t db_id;
    std::string status;
    std::string owner;
    uint64_t create_time;
    uint64_t last_access;
    uint64_t table_count;
    uint64_t total_size;
};

// 表信息结构
struct TableInfo {
    std::string table_name;
    uint32_t table_id;
    uint64_t row_count;
    uint32_t column_count;
    std::vector<ColumnDefinition> columns;
    uint64_t create_time;
    uint32_t data_file_offset;
};

// 索引信息结构
struct IndexInfo {
    std::string index_name;
    uint32_t index_id;
    std::string table_name;
    bool is_unique;
    std::vector<std::string> column_names;
    uint64_t root_page_id;
};

// 统计信息结构
struct DatabaseStatistics {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t total_size;
    uint32_t transaction_count;
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint32_t active_connections;
    double cache_hit_ratio;
    double storage_utilization;
};

// 表格页偏移更新
struct TableOffset {
    std::string db_name;
    std::string table_name;
    uint32_t page_id;

    TableOffset(const std::string& db, const std::string& table, uint32_t id)
        : db_name(db), table_name(table), page_id(id) {
    }
};

class CatalogManager {
private:
    std::string haodb_root_path;
    std::unique_ptr<SystemConfigManager> system_config;
    std::unique_ptr<DatabaseRegistryManager> registry_manager;
    std::map<std::string, std::unique_ptr<DatabaseMetadataManager>> database_managers;

    // 辅助方法
    bool CreateDirectoryStructure(const std::string& db_name);
    bool RemoveDirectoryStructure(const std::string& db_name);
    uint32_t GenerateNewDatabaseId();
    uint32_t GenerateNewTableId(const std::string& db_name);
    uint32_t GenerateNewIndexId(const std::string& db_name);
    uint8_t ParseDataType(const std::string& type_str);
    std::string DataTypeToString(uint8_t type);
    bool LoadDatabaseManager(const std::string& db_name);
    bool ValidateColumnDefinitions(const std::vector<ColumnDefinition>& columns);
    bool ValidateDatabaseName(const std::string& db_name);
    bool ValidateTableName(const std::string& table_name);
    bool ValidateColumnName(const std::string& column_name);
    bool UpdateColumnOffset(std::vector<ColumnMeta>& columns);

public:
    bool UpdateTableOffset(const std::vector<TableOffset>& tables);
    bool UpdateIndexRoot(const std::string& db_name, const std::string& index_name, uint64_t root_page_id);

public:
    explicit CatalogManager(const std::string& haodb_path = "HAODB");
    ~CatalogManager();

    // 初始化和清理
    bool Initialize();
    bool Shutdown();

    // ==================== 数据库管理 ====================

    // (1) 创建数据库
    bool CreateDatabase(const std::string& db_name, const std::string& owner);

    // (2) 查看数据库信息
    std::vector<DatabaseInfo> ListDatabases(const std::string& owner = "");

    // (3) 删除数据库
    bool DropDatabase(const std::string& db_name);

    // 其他数据库操作
    bool DatabaseExists(const std::string& db_name);
    DatabaseInfo GetDatabaseInfo(const std::string& db_name);

    // ==================== 数据表管理 ====================

    // (4) 创建数据表
    bool CreateTable(const std::string& db_name, const std::string& table_name,
        const std::vector<std::vector<std::string>>& column_specs, const int pageId);

    // (5) 设定/取消主键
    bool SetPrimaryKey(const std::string& db_name, const std::string& table_name,
        const std::string& column_name, bool is_primary);

    // (6) 设定/取消可空属性
    bool SetNullable(const std::string& db_name, const std::string& table_name,
        const std::string& column_name, bool is_nullable);

    // (7) 设定/取消唯一属性
    bool SetUnique(const std::string& db_name, const std::string& table_name,
        const std::string& column_name, bool is_unique);

    // (8) 新增列
    bool AddColumns(const std::string& db_name, const std::string& table_name,
        const std::vector<std::vector<std::string>>& column_specs);

    // (9) 删除列
    bool DropColumns(const std::string& db_name, const std::string& table_name,
        const std::vector<std::string>& column_names);

    // (10) 删除表
    bool DropTable(const std::string& db_name, const std::string& table_name);

    // (11) 展示数据库中所有表信息
    std::vector<TableInfo> ListTables(const std::string& db_name);

    // (12) 展示表结构信息
    TableInfo GetTableInfo(const std::string& db_name, const std::string& table_name);

    // 其他表操作
    bool TableExists(const std::string& db_name, const std::string& table_name);
    bool UpdateTableRowCount(const std::string& db_name, const std::string& table_name, uint64_t row_count);

    // ==================== 索引管理 ====================

    // (13) 创建索引
    bool CreateIndex(const std::string& db_name, const std::string& table_name,
        const std::string& index_name, const std::vector<std::string>& column_names,
        bool is_unique = false,  uint64_t root_page_id=0);

    // (14) 删除索引
    bool DropIndex(const std::string& db_name, const std::string& index_name);

    // 其他索引操作
    std::vector<IndexInfo> ListIndexes(const std::string& db_name, const std::string& table_name = "");
    bool IndexExists(const std::string& db_name, const std::string& index_name);
    std::vector<IndexInfo> FindIndexesWithColumns(
        const std::string& db_name,
        const std::string& table_name,
        const std::vector<std::string>& column_names);

    // ==================== 统计信息管理 ====================

    // (15) 更新统计信息
    bool UpdateTotalPages(const std::string& db_name, uint64_t increment = 1);
    bool UpdateFreePages(const std::string& db_name, uint64_t increment = 1);
    bool UpdateDatabaseSize(const std::string& db_name, uint64_t increment = 1);
    bool UpdateTransactionCount(const std::string& db_name, uint64_t increment = 1);
    bool UpdateReadCount(const std::string& db_name, uint64_t increment = 1);
    bool UpdateWriteCount(const std::string& db_name, uint64_t increment = 1);
    bool UpdateCacheHits(const std::string& db_name, uint64_t increment = 1);
    bool UpdateCacheMisses(const std::string& db_name, uint64_t increment = 1);
    bool UpdateActiveConnections(const std::string& db_name, uint32_t count);

    // 获取统计信息
    DatabaseStatistics GetDatabaseStatistics(const std::string& db_name);

    // 重置统计信息
    bool ResetStatistics(const std::string& db_name);

    // ==================== 系统信息 ====================

    // 显示系统信息
    void ShowSystemInfo();
    void ShowDatabaseList();
    void ShowTableList(const std::string& db_name);
    void ShowTableStructure(const std::string& db_name, const std::string& table_name);
    void ShowIndexList(const std::string& db_name);
    void ShowStatistics(const std::string& db_name);

    // 性能分析
    void GenerateSystemReport();
    void GenerateDatabaseReport(const std::string& db_name);

    // ==================== 配置管理 ====================

    // 系统配置
    bool SetMaxDatabases(uint32_t max_db);
    bool SetMaxConnections(uint32_t max_connections);
    bool SetGlobalBufferSize(uint32_t buffer_size);
    bool EnableQueryCache(bool enable);
    bool EnableAuthentication(bool enable);

    // 获取配置信息
    const SystemConfig& GetSystemConfig();
};
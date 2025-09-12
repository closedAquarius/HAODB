#pragma once
#include "system_config_manager.h"
#include "database_registry_manager.h"
#include "database_metadata_manager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// �ж���ṹ
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

// ���ݿ���Ϣ�ṹ
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

// ����Ϣ�ṹ
struct TableInfo {
    std::string table_name;
    uint32_t table_id;
    uint64_t row_count;
    uint32_t column_count;
    std::vector<ColumnDefinition> columns;
    uint64_t create_time;
    uint32_t data_file_offset;
};

// ������Ϣ�ṹ
struct IndexInfo {
    std::string index_name;
    uint32_t index_id;
    std::string table_name;
    bool is_unique;
    std::vector<std::string> column_names;
    uint64_t root_page_id;
};

// ͳ����Ϣ�ṹ
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

// ���ҳƫ�Ƹ���
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

    // ��������
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

    // ��ʼ��������
    bool Initialize();
    bool Shutdown();

    // ==================== ���ݿ���� ====================

    // (1) �������ݿ�
    bool CreateDatabase(const std::string& db_name, const std::string& owner);

    // (2) �鿴���ݿ���Ϣ
    std::vector<DatabaseInfo> ListDatabases(const std::string& owner = "");

    // (3) ɾ�����ݿ�
    bool DropDatabase(const std::string& db_name);

    // �������ݿ����
    bool DatabaseExists(const std::string& db_name);
    DatabaseInfo GetDatabaseInfo(const std::string& db_name);

    // ==================== ���ݱ���� ====================

    // (4) �������ݱ�
    bool CreateTable(const std::string& db_name, const std::string& table_name,
        const std::vector<std::vector<std::string>>& column_specs, const int pageId);

    // (5) �趨/ȡ������
    bool SetPrimaryKey(const std::string& db_name, const std::string& table_name,
        const std::string& column_name, bool is_primary);

    // (6) �趨/ȡ���ɿ�����
    bool SetNullable(const std::string& db_name, const std::string& table_name,
        const std::string& column_name, bool is_nullable);

    // (7) �趨/ȡ��Ψһ����
    bool SetUnique(const std::string& db_name, const std::string& table_name,
        const std::string& column_name, bool is_unique);

    // (8) ������
    bool AddColumns(const std::string& db_name, const std::string& table_name,
        const std::vector<std::vector<std::string>>& column_specs);

    // (9) ɾ����
    bool DropColumns(const std::string& db_name, const std::string& table_name,
        const std::vector<std::string>& column_names);

    // (10) ɾ����
    bool DropTable(const std::string& db_name, const std::string& table_name);

    // (11) չʾ���ݿ������б���Ϣ
    std::vector<TableInfo> ListTables(const std::string& db_name);

    // (12) չʾ��ṹ��Ϣ
    TableInfo GetTableInfo(const std::string& db_name, const std::string& table_name);

    // ���������
    bool TableExists(const std::string& db_name, const std::string& table_name);
    bool UpdateTableRowCount(const std::string& db_name, const std::string& table_name, uint64_t row_count);

    // ==================== �������� ====================

    // (13) ��������
    bool CreateIndex(const std::string& db_name, const std::string& table_name,
        const std::string& index_name, const std::vector<std::string>& column_names,
        bool is_unique = false,  uint64_t root_page_id=0);

    // (14) ɾ������
    bool DropIndex(const std::string& db_name, const std::string& index_name);

    // ������������
    std::vector<IndexInfo> ListIndexes(const std::string& db_name, const std::string& table_name = "");
    bool IndexExists(const std::string& db_name, const std::string& index_name);
    std::vector<IndexInfo> FindIndexesWithColumns(
        const std::string& db_name,
        const std::string& table_name,
        const std::vector<std::string>& column_names);

    // ==================== ͳ����Ϣ���� ====================

    // (15) ����ͳ����Ϣ
    bool UpdateTotalPages(const std::string& db_name, uint64_t increment = 1);
    bool UpdateFreePages(const std::string& db_name, uint64_t increment = 1);
    bool UpdateDatabaseSize(const std::string& db_name, uint64_t increment = 1);
    bool UpdateTransactionCount(const std::string& db_name, uint64_t increment = 1);
    bool UpdateReadCount(const std::string& db_name, uint64_t increment = 1);
    bool UpdateWriteCount(const std::string& db_name, uint64_t increment = 1);
    bool UpdateCacheHits(const std::string& db_name, uint64_t increment = 1);
    bool UpdateCacheMisses(const std::string& db_name, uint64_t increment = 1);
    bool UpdateActiveConnections(const std::string& db_name, uint32_t count);

    // ��ȡͳ����Ϣ
    DatabaseStatistics GetDatabaseStatistics(const std::string& db_name);

    // ����ͳ����Ϣ
    bool ResetStatistics(const std::string& db_name);

    // ==================== ϵͳ��Ϣ ====================

    // ��ʾϵͳ��Ϣ
    void ShowSystemInfo();
    void ShowDatabaseList();
    void ShowTableList(const std::string& db_name);
    void ShowTableStructure(const std::string& db_name, const std::string& table_name);
    void ShowIndexList(const std::string& db_name);
    void ShowStatistics(const std::string& db_name);

    // ���ܷ���
    void GenerateSystemReport();
    void GenerateDatabaseReport(const std::string& db_name);

    // ==================== ���ù��� ====================

    // ϵͳ����
    bool SetMaxDatabases(uint32_t max_db);
    bool SetMaxConnections(uint32_t max_connections);
    bool SetGlobalBufferSize(uint32_t buffer_size);
    bool EnableQueryCache(bool enable);
    bool EnableAuthentication(bool enable);

    // ��ȡ������Ϣ
    const SystemConfig& GetSystemConfig();
};
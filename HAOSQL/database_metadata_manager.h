#pragma once
#include "metadata_structures.h"
#include <string>
#include <vector>

// ���ݿ�Ԫ���ݹ�����
class DatabaseMetadataManager 
{
private:
    std::string metadata_file_path;
    DatabaseHeader db_header;
    StorageConfig storage_config;
    LogConfig log_config; 
    DatabaseStats db_stats;
    std::vector<TableMeta> tables;
    std::vector<IndexMeta> indexes;

public:
    explicit DatabaseMetadataManager(const std::string& file_path);

    // �ļ�����
    bool LoadMetadata();
    bool SaveMetadata();
    bool CreateNewDatabase(const std::string& db_name, uint32_t db_id);

    // �����
    bool CreateTable(const std::string& table_name, uint32_t table_id,
        const std::vector<ColumnMeta>& columns, uint64_t data_offset);
    bool DropTable(const std::string& table_name);
    bool UpdateTableRowCount(const std::string& table_name, uint64_t row_count);
    TableMeta* GetTableInfo(const std::string& table_name);
    TableMeta* GetTableInfo(uint32_t table_id);

    // ��������
    bool CreateIndex(const std::string& index_name, uint32_t index_id,
        uint32_t table_id, const std::vector<uint32_t>& column_ids,
        bool is_unique, uint64_t root_page_id);
    bool DropIndex(const std::string& index_name);
    IndexMeta* GetIndexInfo(const std::string& index_name);
    std::vector<IndexMeta*> GetTableIndexes(uint32_t table_id);

    // �洢���ù���
    const StorageConfig& GetStorageConfig() const { return storage_config; }
    bool SetDataFilePath(const std::string& path);
    bool SetLogFilePath(const std::string& path);
    bool SetIndexFilePath(const std::string& path);
    bool SetBufferPoolSize(uint32_t size);
    bool SetMaxConnections(uint32_t max_conn);
    bool SetLockTimeout(uint32_t timeout);
    bool EnableAutoCheckpoint(bool enable);
    bool SetPageCacheSize(uint32_t size);
    bool EnableCompression(bool enable);
    void ShowStorageConfig();

    // ��־���ù���
    const LogConfig& GetLogConfig() const { return log_config; }
    bool SetLogLevel(uint8_t level);
    bool SetLogFileSize(uint32_t size_mb);
    bool SetLogFileCount(uint32_t count);
    bool EnableWAL(bool enable);
    bool SetWALBufferSize(uint32_t size);
    bool SetSyncMode(uint8_t mode);
    bool EnableLogRotation(bool enable);
    void ShowLogConfig();

    // ���ݿ�ͳ����Ϣ����
    const DatabaseStats& GetDatabaseStats() const { return db_stats; }
    bool UpdateTotalPages(uint64_t total, uint64_t free);
    bool UpdateDatabaseSize(uint64_t size);
    bool SetLastBackupTime(uint64_t timestamp);
    bool IncrementTransactionCount();
    bool UpdateIOStats(uint64_t reads, uint64_t writes);
    bool UpdateCacheStats(uint64_t hits, uint64_t misses);
    bool SetActiveConnections(uint32_t count);
    void ShowDatabaseStats();
    void ResetStatistics();

    // �й���
    bool AddColumn(const std::string& table_name, const ColumnMeta& column);
    bool DropColumn(const std::string& table_name, const std::string& column_name);
    ColumnMeta* GetColumnInfo(const std::string& table_name, const std::string& column_name);

    // ��ѯ�ӿ�
    const DatabaseHeader& GetDatabaseHeader() const { return db_header; }
    const std::vector<TableMeta>& GetTables() const { return tables; }
    const std::vector<IndexMeta>& GetIndexes() const { return indexes; }

    // ��ʾ��Ϣ
    void ShowDatabaseStructure();
    void ShowTableStructure(const std::string& table_name);
    void ShowIndexInfo();
    void ShowCompleteInfo();  // ��ʾ������Ϣ

    // ���ݿ������޸�
    bool SetDatabaseName(const std::string& new_name);
    bool SetPageSize(uint32_t page_size);

    // ͳ����Ϣ
    size_t GetTableCount() const { return tables.size(); }
    size_t GetIndexCount() const { return indexes.size(); }
    uint64_t GetTotalRowCount() const;

    // ���ܷ���
    double GetCacheHitRatio() const;
    uint64_t GetAverageTransactionSize() const;
    void GeneratePerformanceReport();
};

// �洢���ù�����
class StorageConfigManager
{
private:
    StorageConfig& config;

public:
    explicit StorageConfigManager(StorageConfig& cfg) : config(cfg) {}

    // ·������
    bool SetDataPath(const std::string& path);
    bool SetLogPath(const std::string& path);
    bool SetIndexPath(const std::string& path);

    // �������
    bool SetBufferPoolSize(uint32_t size_mb);
    bool SetPageCacheSize(uint32_t size_mb);

    // ���ӹ���
    bool SetMaxConnections(uint32_t max_conn);
    bool SetLockTimeout(uint32_t timeout_sec);

    // ���Կ���
    bool EnableAutoCheckpoint(bool enable);
    bool EnableCompression(bool enable);

    // ��֤����
    bool ValidateConfig();
    void PrintConfig();
};

// ��־���ù�����
class LogConfigManager 
{
private:
    LogConfig& config;

public:
    explicit LogConfigManager(LogConfig& cfg) : config(cfg) {}

    // ��־�������
    bool SetLogLevel(const std::string& level);  // "ERROR", "WARN", "INFO", "DEBUG"
    bool SetLogLevel(uint8_t level);

    // �ļ�����
    bool SetLogFileSize(uint32_t size_mb);
    bool SetLogFileCount(uint32_t count);
    bool EnableLogRotation(bool enable);

    // WAL����
    bool EnableWAL(bool enable);
    bool SetWALBufferSize(uint32_t size_mb);
    bool SetWALCheckpointSize(uint32_t size_mb);

    // ͬ��ģʽ
    bool SetSyncMode(const std::string& mode);  // "async", "sync", "full_sync"
    bool SetSyncMode(uint8_t mode);

    // ��֤����
    bool ValidateConfig();
    void PrintConfig();
    std::string GetLogLevelString() const;
    std::string GetSyncModeString() const;
};

// ͳ����Ϣ������
class DatabaseStatsManager 
{
private:
    DatabaseStats& stats;

public:
    explicit DatabaseStatsManager(DatabaseStats& st) : stats(st) {}

    // �洢ͳ��
    bool UpdateStorageStats(uint64_t total_pages, uint64_t free_pages, uint64_t total_size);
    bool IncrementTransactionCount(uint32_t count = 1);

    // I/Oͳ��
    bool RecordRead(uint64_t count = 1);
    bool RecordWrite(uint64_t count = 1);
    bool RecordCacheHit(uint64_t count = 1);
    bool RecordCacheMiss(uint64_t count = 1);

    // ����ͳ��
    bool SetActiveConnections(uint32_t count);
    bool IncrementActiveConnections();
    bool DecrementActiveConnections();

    // ������Ϣ
    bool UpdateBackupTime();
    bool SetBackupTime(uint64_t timestamp);

    // ͳ�Ƽ���
    double GetCacheHitRatio() const;
    double GetStorageUtilization() const;
    uint64_t GetTotalIO() const;

    // ���ú�����
    void ResetIOStats();
    void ResetCacheStats();
    void ResetAllStats();

    // ��������
    void PrintStats();
    void PrintPerformanceReport();
    void ExportStatsToFile(const std::string& filename);
};
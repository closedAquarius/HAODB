#pragma once
#include "metadata_structures.h"
#include <string>
#include <vector>

// 数据库元数据管理类
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

    // 文件操作
    bool LoadMetadata();
    bool SaveMetadata();
    bool CreateNewDatabase(const std::string& db_name, uint32_t db_id);

    // 表管理
    bool CreateTable(const std::string& table_name, uint32_t table_id,
        const std::vector<ColumnMeta>& columns, uint64_t data_offset);
    bool DropTable(const std::string& table_name);
    bool UpdateTableRowCount(const std::string& table_name, uint64_t row_count);
    TableMeta* GetTableInfo(const std::string& table_name);
    TableMeta* GetTableInfo(uint32_t table_id);

    // 索引管理
    bool CreateIndex(const std::string& index_name, uint32_t index_id,
        uint32_t table_id, const std::vector<uint32_t>& column_ids,
        bool is_unique, uint64_t root_page_id);
    bool DropIndex(const std::string& index_name);
    IndexMeta* GetIndexInfo(const std::string& index_name);
    std::vector<IndexMeta*> GetTableIndexes(uint32_t table_id);

    // 存储配置管理
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

    // 日志配置管理
    const LogConfig& GetLogConfig() const { return log_config; }
    bool SetLogLevel(uint8_t level);
    bool SetLogFileSize(uint32_t size_mb);
    bool SetLogFileCount(uint32_t count);
    bool EnableWAL(bool enable);
    bool SetWALBufferSize(uint32_t size);
    bool SetSyncMode(uint8_t mode);
    bool EnableLogRotation(bool enable);
    void ShowLogConfig();

    // 数据库统计信息管理
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

    // 列管理
    bool AddColumn(const std::string& table_name, const ColumnMeta& column);
    bool DropColumn(const std::string& table_name, const std::string& column_name);
    ColumnMeta* GetColumnInfo(const std::string& table_name, const std::string& column_name);

    // 查询接口
    const DatabaseHeader& GetDatabaseHeader() const { return db_header; }
    const std::vector<TableMeta>& GetTables() const { return tables; }
    const std::vector<IndexMeta>& GetIndexes() const { return indexes; }

    // 显示信息
    void ShowDatabaseStructure();
    void ShowTableStructure(const std::string& table_name);
    void ShowIndexInfo();
    void ShowCompleteInfo();  // 显示完整信息

    // 数据库属性修改
    bool SetDatabaseName(const std::string& new_name);
    bool SetPageSize(uint32_t page_size);

    // 统计信息
    size_t GetTableCount() const { return tables.size(); }
    size_t GetIndexCount() const { return indexes.size(); }
    uint64_t GetTotalRowCount() const;

    // 性能分析
    double GetCacheHitRatio() const;
    uint64_t GetAverageTransactionSize() const;
    void GeneratePerformanceReport();
};

// 存储配置管理器
class StorageConfigManager
{
private:
    StorageConfig& config;

public:
    explicit StorageConfigManager(StorageConfig& cfg) : config(cfg) {}

    // 路径管理
    bool SetDataPath(const std::string& path);
    bool SetLogPath(const std::string& path);
    bool SetIndexPath(const std::string& path);

    // 缓存管理
    bool SetBufferPoolSize(uint32_t size_mb);
    bool SetPageCacheSize(uint32_t size_mb);

    // 连接管理
    bool SetMaxConnections(uint32_t max_conn);
    bool SetLockTimeout(uint32_t timeout_sec);

    // 特性开关
    bool EnableAutoCheckpoint(bool enable);
    bool EnableCompression(bool enable);

    // 验证配置
    bool ValidateConfig();
    void PrintConfig();
};

// 日志配置管理器
class LogConfigManager 
{
private:
    LogConfig& config;

public:
    explicit LogConfigManager(LogConfig& cfg) : config(cfg) {}

    // 日志级别管理
    bool SetLogLevel(const std::string& level);  // "ERROR", "WARN", "INFO", "DEBUG"
    bool SetLogLevel(uint8_t level);

    // 文件管理
    bool SetLogFileSize(uint32_t size_mb);
    bool SetLogFileCount(uint32_t count);
    bool EnableLogRotation(bool enable);

    // WAL配置
    bool EnableWAL(bool enable);
    bool SetWALBufferSize(uint32_t size_mb);
    bool SetWALCheckpointSize(uint32_t size_mb);

    // 同步模式
    bool SetSyncMode(const std::string& mode);  // "async", "sync", "full_sync"
    bool SetSyncMode(uint8_t mode);

    // 验证配置
    bool ValidateConfig();
    void PrintConfig();
    std::string GetLogLevelString() const;
    std::string GetSyncModeString() const;
};

// 统计信息管理器
class DatabaseStatsManager 
{
private:
    DatabaseStats& stats;

public:
    explicit DatabaseStatsManager(DatabaseStats& st) : stats(st) {}

    // 存储统计
    bool UpdateStorageStats(uint64_t total_pages, uint64_t free_pages, uint64_t total_size);
    bool IncrementTransactionCount(uint32_t count = 1);

    // I/O统计
    bool RecordRead(uint64_t count = 1);
    bool RecordWrite(uint64_t count = 1);
    bool RecordCacheHit(uint64_t count = 1);
    bool RecordCacheMiss(uint64_t count = 1);

    // 连接统计
    bool SetActiveConnections(uint32_t count);
    bool IncrementActiveConnections();
    bool DecrementActiveConnections();

    // 备份信息
    bool UpdateBackupTime();
    bool SetBackupTime(uint64_t timestamp);

    // 统计计算
    double GetCacheHitRatio() const;
    double GetStorageUtilization() const;
    uint64_t GetTotalIO() const;

    // 重置和清理
    void ResetIOStats();
    void ResetCacheStats();
    void ResetAllStats();

    // 报告生成
    void PrintStats();
    void PrintPerformanceReport();
    void ExportStatsToFile(const std::string& filename);
};
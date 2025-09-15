#pragma once
#include "logger.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// 前向声明，避免循环依赖
class Operator;
class BufferPoolManager;
class CatalogManager;
class Page;
class DiskManager;
struct Quadruple;

// 定义基本类型（避免依赖executor.h）
using Row = std::map<std::string, std::string>;
using Table = std::vector<Row>;

struct WithdrawLog
{
    uint16_t page_id;                // 页面ID
    uint16_t slot_id;                // 槽位ID
    uint16_t length;                 // 数据长度
    int record_type;                 // 1-Delete, 2-Insert, 3-Update_Delete, 4-Update_Insert
};

// ========== 全局日志管理器 ==========
class GlobalLoggerManager {
private:
    static std::map<std::string, std::unique_ptr<DatabaseLogger>> loggers;
    static std::mutex manager_mutex;

public:
    static DatabaseLogger* GetLogger(const std::string& db_name, CatalogManager* catalog = nullptr);
    static void ShutdownAll();
    static DatabaseLogger* GetDefaultLogger();
};

// ========== 日志配置管理器 ==========
//class LogConfigManager {
//public:
//    static LogConfigInfo LoadConfig(const std::string& config_file);
//    static bool SaveConfig(const LogConfigInfo& config, const std::string& config_file);
//    static LogConfigInfo GetSystemDefaultConfig();
//    static bool UpdateLogLevel(const std::string& db_name, uint8_t new_level);
//    static bool UpdateSyncMode(const std::string& db_name, uint8_t new_mode);
//};

// ========== 简化的日志接口 ==========
class SimpleLogger {
private:
    DatabaseLogger* logger;
    std::string db_name;
    uint32_t current_transaction_id;

public:
    SimpleLogger(const std::string& db_name);
    ~SimpleLogger();

    void BeginTransaction();
    void CommitTransaction();
    void RollbackTransaction();

    // void LogSQL(const std::string& sql, const std::vector<OperationLogRecord::SimpleQuadruple>& quads, bool success = true);
    // void LogInsert(const std::string& table_name, const std::map<std::string, std::string>& data);
    //void LogDelete(const std::string& table_name, const std::string& condition);
    //void LogUpdate(const std::string& table_name, const std::string& condition, const std::map<std::string, std::string>& changes);
    void LogError(const std::string& error_msg);
    void LogInfo(const std::string& info_msg);

    bool UndoLastOperation();
    bool RecoverFromCrash();
};

// ========== 全局变量声明 ==========
extern std::string CURRENT_DB_NAME;
extern DatabaseLogger* CURRENT_LOGGER;

// ========== 日志宏定义 ==========
#define INIT_LOGGER(db_name, catalog) \
    do { \
        CURRENT_DB_NAME = db_name; \
        CURRENT_LOGGER = GlobalLoggerManager::GetLogger(db_name, catalog); \
        if (!CURRENT_LOGGER) { \
            throw std::runtime_error("Failed to initialize logger for " + std::string(db_name)); \
        } \
    } while(0)

#define LOG_SQL_EXECUTION(sql, quads, success) \
    do { \
        if (CURRENT_LOGGER) { \
            uint32_t txn_id = CURRENT_LOGGER->BeginTransaction(); \
            CURRENT_LOGGER->LogQuadrupleExecution(txn_id, sql, quads); \
            if (success) { \
                CURRENT_LOGGER->CommitTransaction(txn_id); \
            } else { \
                CURRENT_LOGGER->AbortTransaction(txn_id); \
            } \
        } \
    } while(0)

#define LOG_ERROR(msg) \
    do { \
        if (CURRENT_LOGGER) { \
            CURRENT_LOGGER->LogError(msg, __FUNCTION__); \
        } \
    } while(0)

#define BEGIN_TRANSACTION() \
    uint32_t __txn_id = 0; \
    do { \
        if (CURRENT_LOGGER) { \
            __txn_id = CURRENT_LOGGER->BeginTransaction(); \
        } \
    } while(0)

#define COMMIT_TRANSACTION() \
    do { \
        if (CURRENT_LOGGER && __txn_id != 0) { \
            CURRENT_LOGGER->CommitTransaction(__txn_id); \
        } \
    } while(0)

#define ROLLBACK_TRANSACTION() \
    do { \
        if (CURRENT_LOGGER && __txn_id != 0) { \
            CURRENT_LOGGER->AbortTransaction(__txn_id); \
        } \
    } while(0)

// ========== 基础算子接口（避免依赖executor.h）==========
class BaseOperator {
public:
    virtual ~BaseOperator() {}
    virtual std::vector<Row> execute() = 0;
};

// ========== 带日志的算子（组合模式）==========
class LoggedInsertComposition : public BaseOperator {
private:
    Table* table;
    Row row_data;
    DatabaseLogger* logger;
    uint32_t transaction_id;
    std::string table_name;

public:
    LoggedInsertComposition(Table* t, const Row& r, const std::string& tname,
        DatabaseLogger* log, uint32_t txn_id);
    std::vector<Row> execute() override;

private:
    std::vector<char> SerializeRow(const Row& row);
};

class LoggedDeleteComposition : public BaseOperator {
private:
    Table* table;
    std::function<bool(const Row&)> predicate;
    DatabaseLogger* logger;
    uint32_t transaction_id;
    std::string table_name;

public:
    LoggedDeleteComposition(Table* t, std::function<bool(const Row&)> pred,
        const std::string& tname, DatabaseLogger* log, uint32_t txn_id);
    std::vector<Row> execute() override;

private:
    std::vector<char> SerializeRow(const Row& row);
};

class LoggedUpdateComposition : public BaseOperator {
private:
    Table* table;
    std::function<void(Row&)> updater;
    std::function<bool(const Row&)> predicate;
    DatabaseLogger* logger;
    uint32_t transaction_id;
    std::string table_name;

public:
    LoggedUpdateComposition(Table* t, std::function<void(Row&)> u,
        const std::string& tname, DatabaseLogger* log, uint32_t txn_id,
        std::function<bool(const Row&)> p = nullptr);
    std::vector<Row> execute() override;

private:
    std::vector<char> SerializeRow(const Row& row);
};

// ========== 带日志算子工厂 ==========
class LoggedOperatorFactory {
public:
    static std::unique_ptr<BaseOperator> CreateLoggedInsert(
        Table* table, const Row& row, const std::string& table_name,
        DatabaseLogger* logger, uint32_t transaction_id);

    static std::unique_ptr<BaseOperator> CreateLoggedDelete(
        Table* table, std::function<bool(const Row&)> predicate, const std::string& table_name,
        DatabaseLogger* logger, uint32_t transaction_id);

    static std::unique_ptr<BaseOperator> CreateLoggedUpdate(
        Table* table, std::function<void(Row&)> updater,
        const std::string& table_name, DatabaseLogger* logger, uint32_t transaction_id,
        std::function<bool(const Row&)> predicate = nullptr);
};

// ========== 性能监控器 ==========
class LogPerformanceMonitor {
private:
    std::map<std::string, std::chrono::steady_clock::time_point> operation_start_times;
    std::map<std::string, uint64_t> operation_counts;
    std::map<std::string, uint64_t> total_execution_times;
    std::mutex monitor_mutex;

public:
    void StartOperation(const std::string& operation_name);
    void EndOperation(const std::string& operation_name);

    struct PerformanceStats {
        uint64_t total_operations;
        uint64_t total_time_ms;
        double average_time_ms;
    };

    PerformanceStats GetStats(const std::string& operation_name);
    void PrintAllStats();
    void ResetStats();
};

// ========== 日志文件查看器 ==========
class LogViewer {
private:
    std::string db_name;
    std::unique_ptr<LogFileManager> file_manager;

public:
    LogViewer(const std::string& db_name);
    ~LogViewer();

    std::vector<std::string> GetRecentOperations(int count = 10);
    std::vector<std::string> GetRecentErrors(int count = 10);
    std::vector<std::string> SearchLogs(const std::string& pattern,
        const std::string& log_type = "operation");
    bool ExportLogs(const std::string& output_file,
        const std::string& date_range = "");

    struct LogStats {
        uint64_t total_operations;
        uint64_t total_errors;
        uint64_t total_transactions;
        std::string oldest_log_date;
        std::string newest_log_date;
        uint64_t total_log_size_mb;
    };

    LogStats GetLogStatistics();

    // ========== 日志打印函数 ==========
    void PrintRecentOperations(int max_lines = 20);
    void PrintRecentErrors(int max_lines = 10);
    void PrintAllLogs(int max_lines = 50);
    void PrintLogsByType(const std::string& log_type, int max_lines = 20);
};

// ========== 增强的执行引擎 ==========
class EnhancedExecutor {
private:
    DatabaseLogger* logger;
    std::string db_name;
    std::string session_id;
    std::string user_name;

public:
    EnhancedExecutor(const std::string& db_name, const std::string& session = "", const std::string& user = "");
    ~EnhancedExecutor();
    bool Initialize();

    // 执行SQL语句（带完整日志记录）
    // bool ExecuteSQL(const std::string& sql, const std::vector<OperationLogRecord::SimpleQuadruple>& quads);

    // 数据变更操作（带WAL日志）
    bool InsertRecord(uint16_t page_id, uint16_t slot_id, uint16_t length, string sql, 
        vector<Quadruple> qua, string user, bool result, uint64_t duration, string message);
    bool DeleteRecord(uint16_t page_id, uint16_t slot_id, uint16_t length, string sql,
        vector<Quadruple> qua, string user, bool result, uint64_t duration, string message);
    bool UpdateRecord(uint16_t before_page_id, uint16_t before_slot_id, uint16_t before_length, uint16_t after_page_id,
        uint16_t after_slot_id, uint16_t after_length, string sql, vector<Quadruple> qua, string user, bool result, uint64_t duration, string message);

    // 恢复操作
    bool UndoLastDelete();
    bool UndoLastUpdate();
    bool UndoLastInsert();
    bool PerformCrashRecovery();
    vector<WithdrawLog> UndoLastOperation(int counts);
    void PrintAllWAL();

    // 日志状态查看
    void ShowLoggerStatus();

private:
    std::vector<char> SerializeRowData(const std::map<std::string, std::string>& row_data);
    uint64_t GetLastLogId();
};

// ========== 实用工具函数 ==========
namespace LoggerUtils {
    std::string FormatTimestamp(uint64_t timestamp);
    uint8_t ParseLogLevel(const std::string& level_name);
    std::string LogLevelToString(uint8_t level_code);
    uint8_t ParseSyncMode(const std::string& mode_name);
    std::string SyncModeToString(uint8_t mode_code);
    std::string FormatFileSize(uint64_t size_bytes);
    bool ValidateLogFileIntegrity(const std::string& log_file_path);
    bool MergeLogFiles(const std::vector<std::string>& input_files,
        const std::string& output_file);
    bool RebuildOperationLogFromWAL(const std::string& wal_file,
        const std::string& output_file);
}

// ========== 转换函数（用于与原有代码集成）==========
//std::vector<OperationLogRecord::SimpleQuadruple> ConvertQuadruples(const std::vector<Quadruple>& quads);
//std::vector<Quadruple> ConvertSimpleQuadruples(const std::vector<OperationLogRecord::SimpleQuadruple>& simple_quads);
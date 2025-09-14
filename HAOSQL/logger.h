#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <functional>
#include <cstring>
#include <ctime>
#include "dataType.h"
#include "catalog_manager.h"

using namespace std;

// ========== WAL日志记录结构 ==========
struct WALLogRecord {
    uint64_t lsn;                    // 日志序列号
    uint32_t transaction_id;         // 事务ID
    uint64_t timestamp;              // 时间戳
    int record_type;             // 记录类型
    uint32_t record_size;            // 记录总大小
    uint32_t checksum;               // 校验和
    int withdraw = 0;

    // 四元式信息
    //struct QuadrupleInfo {
    //    uint8_t op_code;             // 操作码
    //    std::string table_name;      // 表名
    //    std::string condition;       // 条件表达式
    //    uint32_t affected_rows;      // 影响行数
    //    std::string original_sql;    // 原始SQL

    //    QuadrupleInfo();
    //} quad_info;

    // 数据变更信息
    struct DataChange {
        uint32_t before_page_id;            // 页面ID
        uint32_t before_slot_id;            // 槽位ID
        uint32_t after_page_id;            // 页面ID
        uint32_t after_slot_id;            // 槽位ID
        uint32_t before_length;      // 修改前数据长度
        uint32_t after_length;       // 修改后数据长度
        // std::vector<char> before_image; // 修改前的完整记录
        // std::vector<char> after_image;  // 修改后的完整记录

        DataChange();
    };
    std::vector<DataChange> changes;

    // 记录类型枚举
    enum RecordType {
        TXN_BEGIN = 1,
        TXN_COMMIT = 2,
        TXN_ABORT = 3,
        INSERT_OP = 4,
        DELETE_OP = 5,
        UPDATE_OP = 6,
        CHECKPOINT = 7
    };

    WALLogRecord();
};

// ========== 操作日志记录结构 ==========
struct OperationLogRecord {
    uint64_t log_id;                 // 日志ID
    uint64_t timestamp;              // 时间戳
    std::string session_id;          // 会话ID
    std::string user_name;           // 用户名
    uint8_t log_level;               // 日志级别
    std::string original_sql;        // 原始SQL语句

    // 四元式序列
    struct SimpleQuadruple {
        std::string op;
        std::string arg1;
        std::string arg2;
        std::string result;

        SimpleQuadruple();
        SimpleQuadruple(const std::string& op, const std::string& arg1,
            const std::string& arg2, const std::string& result);
    };
    std::vector<SimpleQuadruple> quad_sequence;

    // 执行结果
    struct ExecutionResult {
        bool success;                // 是否成功
        uint32_t affected_rows;      // 影响行数
        uint64_t execution_time_ms;  // 执行时间(毫秒)
        std::string error_message;   // 错误信息

        ExecutionResult();
    } result;

    // 性能信息
    struct PerformanceInfo {
        uint32_t pages_read;         // 读取页面数
        uint32_t pages_written;      // 写入页面数
        uint32_t index_lookups;      // 索引查找次数

        PerformanceInfo();
    } perf_info;

    OperationLogRecord();
};

// ========== WAL缓冲区 ==========
class WALBuffer {
private:
    std::vector<char> buffer;
    size_t buffer_size;
    size_t current_pos;
    std::mutex buffer_mutex;

public:
    WALBuffer();
    ~WALBuffer();

    void Initialize(size_t size);
    bool WriteRecord(const WALLogRecord& record);
    void FlushToFile(std::ofstream& file);
    bool ShouldFlush() const;
    size_t GetCurrentSize() const;
    void Clear();

private:
    inline bool read_exact(std::istream& in, void* buf, std::size_t n);
    template<typename T> inline bool read_pod(std::istream& in, T& v);
    bool read_len_prefixed_string(std::istream& in, std::string& s, std::vector<char>& payload);
    bool read_bytes_exact(std::istream& in, std::vector<char>& out, uint32_t n, std::vector<char>& payload);
    uint32_t CalculateChecksum(const std::vector<char>& data);

public:
    std::vector<char> SerializeRecord(const WALLogRecord& record);
    bool DeserializeWALLogRecord(std::istream& in, WALLogRecord& rec);
};

// ========== 日志文件管理器 ==========
class LogFileManager {
private:
    LogConfigInfo config;
    std::string db_name;
    std::string log_directory;
    std::string current_wal_file;
    std::string current_op_file;
    std::string current_error_file;

public:
    LogFileManager(const LogConfigInfo& cfg, const std::string& db);
    ~LogFileManager();

    // 文件管理
    bool CreateLogDirectory();
    std::string GetWALFilePath();
    std::string GetOperationLogPath();
    std::string GetErrorLogPath();

    // 日志轮转
    void RotateLogsIfNeeded();
    void CleanupOldLogs();

    // 工具函数
    static std::string GetCurrentTimestamp();
    static uint64_t GetFileSize(const std::string& filepath);

private:
    void RotateFile(const std::string& filepath);
    std::vector<std::string> GetLogFiles(const std::string& pattern);
};

// ========== 事务管理器 ==========
class TransactionManager {
private:
    std::atomic<uint32_t> next_transaction_id;
    std::map<uint32_t, bool> active_transactions; // transaction_id -> is_committed
    std::mutex txn_mutex;

public:
    TransactionManager();

    uint32_t BeginTransaction();
    void CommitTransaction(uint32_t txn_id);
    void AbortTransaction(uint32_t txn_id);
    bool IsTransactionActive(uint32_t txn_id);
    std::vector<uint32_t> GetActiveTransactions();
};

// ========== 恢复管理器 ==========
class RecoveryManager {
private:
    std::string db_name;
    LogFileManager* file_manager;

public:
    RecoveryManager(const std::string& db, LogFileManager* fm);

    // 崩溃恢复
    bool PerformCrashRecovery();

    // 操作恢复
    bool UndoDelete(WALLogRecord record);
    bool UndoUpdate(uint64_t log_id);
    bool UndoInsert(uint64_t log_id);

    // 事务恢复
    bool UndoTransaction(uint32_t transaction_id);
    bool RedoTransaction(uint32_t transaction_id);

    // 获取最新WAL操作
    WALLogRecord FindLatestWALRecord();

private:
    std::vector<WALLogRecord> ReadWALLog();
    std::vector<WALLogRecord> FindWALRecordsByLogId(uint64_t log_id);
    std::vector<WALLogRecord> FindWALRecordsByTransaction(uint32_t txn_id);
    WALLogRecord DeserializeWALRecord(const std::vector<char>& data);

    // 物理恢复操作
    bool RestoreRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,
        uint32_t after_page_id, uint32_t after_slot_id, uint32_t after_length);
    bool DeleteRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,
        uint32_t after_page_id, uint32_t after_slot_id, uint32_t after_length);
};

// ========== 数据库日志管理器（主类）==========
class DatabaseLogger {
private:
    LogConfigInfo config;
    std::string db_name;
    std::unique_ptr<LogFileManager> file_manager;
    std::unique_ptr<WALBuffer> wal_buffer;
    std::unique_ptr<TransactionManager> txn_manager;
    std::unique_ptr<RecoveryManager> recovery_manager;

    // 文件流
    std::ofstream wal_file;
    std::ofstream operation_file;
    std::ofstream error_file;

    // 线程安全
    std::mutex log_mutex;
    std::atomic<uint64_t> next_lsn;
    std::atomic<uint64_t> next_log_id;

    // 后台线程
    std::thread background_thread;
    std::atomic<bool> stop_background;
    bool background_running;

public:
    DatabaseLogger(const LogConfigInfo& cfg, const std::string& db_name);
    ~DatabaseLogger();

    // 初始化和关闭
    bool Initialize();
    void Shutdown();

    // 事务管理
    uint32_t BeginTransaction();
    // void CommitTransaction(uint32_t txn_id, int type = WALLogRecord::TXN_COMMIT);
    void CommitTransaction(uint32_t txn_id, WALLogRecord& record);
    void AbortTransaction(uint32_t txn_id);

    // 主要日志接口
    uint64_t LogQuadrupleExecution(
        uint32_t transaction_id,
        const std::string& original_sql,
        const std::vector<OperationLogRecord::SimpleQuadruple>& quads,
        const std::string& session_id = "",
        const std::string& user_name = "");

    void LogDataChange(
        uint32_t transaction_id,
        uint8_t operation_type,
        uint32_t before_page_id,
        uint32_t before_slot_id,
        uint32_t after_page_id,
        uint32_t after_slot_id,
        uint32_t before_length,
        uint32_t after_length);

    void LogError(const std::string& error_message,
        const std::string& context = "");

    // 恢复接口
    RecoveryManager* GetRecoveryManager() { return recovery_manager.get(); }

    // 配置访问
    const LogConfigInfo& GetConfig() const { return config; }

private:
    // 内部方法
    void WriteWALLog(const WALLogRecord& record);
    void WriteOperationLog(const OperationLogRecord& record);
    void FlushWALBuffer();

    // 后台线程函数
    void BackgroundWorker();

    // 工具函数
    uint64_t GenerateLSN();
    uint64_t GenerateLogId();
    uint64_t GetCurrentTimestamp();

    // 序列化
    std::string SerializeOperationLog(const OperationLogRecord& record);
};

// ========== 日志管理器工厂 ==========
class LoggerFactory {
public:
    static std::unique_ptr<DatabaseLogger> CreateLogger(
        const std::string& db_name,
        CatalogManager* catalog = nullptr);

    static LogConfigInfo GetDefaultConfig();
};
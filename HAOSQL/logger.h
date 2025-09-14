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

// ========== WAL��־��¼�ṹ ==========
struct WALLogRecord {
    uint64_t lsn;                    // ��־���к�
    uint32_t transaction_id;         // ����ID
    uint64_t timestamp;              // ʱ���
    int record_type;             // ��¼����
    uint32_t record_size;            // ��¼�ܴ�С
    uint32_t checksum;               // У���
    int withdraw = 0;

    // ��Ԫʽ��Ϣ
    //struct QuadrupleInfo {
    //    uint8_t op_code;             // ������
    //    std::string table_name;      // ����
    //    std::string condition;       // �������ʽ
    //    uint32_t affected_rows;      // Ӱ������
    //    std::string original_sql;    // ԭʼSQL

    //    QuadrupleInfo();
    //} quad_info;

    // ���ݱ����Ϣ
    struct DataChange {
        uint32_t before_page_id;            // ҳ��ID
        uint32_t before_slot_id;            // ��λID
        uint32_t after_page_id;            // ҳ��ID
        uint32_t after_slot_id;            // ��λID
        uint32_t before_length;      // �޸�ǰ���ݳ���
        uint32_t after_length;       // �޸ĺ����ݳ���
        // std::vector<char> before_image; // �޸�ǰ��������¼
        // std::vector<char> after_image;  // �޸ĺ��������¼

        DataChange();
    };
    std::vector<DataChange> changes;

    // ��¼����ö��
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

// ========== ������־��¼�ṹ ==========
struct OperationLogRecord {
    uint64_t log_id;                 // ��־ID
    uint64_t timestamp;              // ʱ���
    std::string session_id;          // �ỰID
    std::string user_name;           // �û���
    uint8_t log_level;               // ��־����
    std::string original_sql;        // ԭʼSQL���

    // ��Ԫʽ����
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

    // ִ�н��
    struct ExecutionResult {
        bool success;                // �Ƿ�ɹ�
        uint32_t affected_rows;      // Ӱ������
        uint64_t execution_time_ms;  // ִ��ʱ��(����)
        std::string error_message;   // ������Ϣ

        ExecutionResult();
    } result;

    // ������Ϣ
    struct PerformanceInfo {
        uint32_t pages_read;         // ��ȡҳ����
        uint32_t pages_written;      // д��ҳ����
        uint32_t index_lookups;      // �������Ҵ���

        PerformanceInfo();
    } perf_info;

    OperationLogRecord();
};

// ========== WAL������ ==========
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

// ========== ��־�ļ������� ==========
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

    // �ļ�����
    bool CreateLogDirectory();
    std::string GetWALFilePath();
    std::string GetOperationLogPath();
    std::string GetErrorLogPath();

    // ��־��ת
    void RotateLogsIfNeeded();
    void CleanupOldLogs();

    // ���ߺ���
    static std::string GetCurrentTimestamp();
    static uint64_t GetFileSize(const std::string& filepath);

private:
    void RotateFile(const std::string& filepath);
    std::vector<std::string> GetLogFiles(const std::string& pattern);
};

// ========== ��������� ==========
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

// ========== �ָ������� ==========
class RecoveryManager {
private:
    std::string db_name;
    LogFileManager* file_manager;

public:
    RecoveryManager(const std::string& db, LogFileManager* fm);

    // �����ָ�
    bool PerformCrashRecovery();

    // �����ָ�
    bool UndoDelete(WALLogRecord record);
    bool UndoUpdate(uint64_t log_id);
    bool UndoInsert(uint64_t log_id);

    // ����ָ�
    bool UndoTransaction(uint32_t transaction_id);
    bool RedoTransaction(uint32_t transaction_id);

    // ��ȡ����WAL����
    WALLogRecord FindLatestWALRecord();

private:
    std::vector<WALLogRecord> ReadWALLog();
    std::vector<WALLogRecord> FindWALRecordsByLogId(uint64_t log_id);
    std::vector<WALLogRecord> FindWALRecordsByTransaction(uint32_t txn_id);
    WALLogRecord DeserializeWALRecord(const std::vector<char>& data);

    // ����ָ�����
    bool RestoreRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,
        uint32_t after_page_id, uint32_t after_slot_id, uint32_t after_length);
    bool DeleteRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,
        uint32_t after_page_id, uint32_t after_slot_id, uint32_t after_length);
};

// ========== ���ݿ���־�����������ࣩ==========
class DatabaseLogger {
private:
    LogConfigInfo config;
    std::string db_name;
    std::unique_ptr<LogFileManager> file_manager;
    std::unique_ptr<WALBuffer> wal_buffer;
    std::unique_ptr<TransactionManager> txn_manager;
    std::unique_ptr<RecoveryManager> recovery_manager;

    // �ļ���
    std::ofstream wal_file;
    std::ofstream operation_file;
    std::ofstream error_file;

    // �̰߳�ȫ
    std::mutex log_mutex;
    std::atomic<uint64_t> next_lsn;
    std::atomic<uint64_t> next_log_id;

    // ��̨�߳�
    std::thread background_thread;
    std::atomic<bool> stop_background;
    bool background_running;

public:
    DatabaseLogger(const LogConfigInfo& cfg, const std::string& db_name);
    ~DatabaseLogger();

    // ��ʼ���͹ر�
    bool Initialize();
    void Shutdown();

    // �������
    uint32_t BeginTransaction();
    // void CommitTransaction(uint32_t txn_id, int type = WALLogRecord::TXN_COMMIT);
    void CommitTransaction(uint32_t txn_id, WALLogRecord& record);
    void AbortTransaction(uint32_t txn_id);

    // ��Ҫ��־�ӿ�
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

    // �ָ��ӿ�
    RecoveryManager* GetRecoveryManager() { return recovery_manager.get(); }

    // ���÷���
    const LogConfigInfo& GetConfig() const { return config; }

private:
    // �ڲ�����
    void WriteWALLog(const WALLogRecord& record);
    void WriteOperationLog(const OperationLogRecord& record);
    void FlushWALBuffer();

    // ��̨�̺߳���
    void BackgroundWorker();

    // ���ߺ���
    uint64_t GenerateLSN();
    uint64_t GenerateLogId();
    uint64_t GetCurrentTimestamp();

    // ���л�
    std::string SerializeOperationLog(const OperationLogRecord& record);
};

// ========== ��־���������� ==========
class LoggerFactory {
public:
    static std::unique_ptr<DatabaseLogger> CreateLogger(
        const std::string& db_name,
        CatalogManager* catalog = nullptr);

    static LogConfigInfo GetDefaultConfig();
};
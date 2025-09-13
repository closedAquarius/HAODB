#define _CRT_SECURE_NO_WARNINGS
#include "logger.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

// ========== 结构体构造函数实现 ==========
//LogConfigInfo::LogConfigInfo() {
//    log_level_name = "INFO";
//    log_level_code = 2;
//    log_file_size_mb = 50;
//    log_file_count = 5;
//    enable_wal = true;
//    wal_buffer_size_mb = 8;
//    sync_mode_name = "同步";
//    sync_mode_code = 1;
//    wal_checkpoint_size_mb = 64;
//    enable_log_rotation = true;
//}

WALLogRecord::QuadrupleInfo::QuadrupleInfo() : op_code(0), affected_rows(0) {}

WALLogRecord::DataChange::DataChange() : page_id(0), slot_id(0), before_length(0), after_length(0) {}

WALLogRecord::WALLogRecord() : lsn(0), transaction_id(0), timestamp(0), record_type(0), record_size(0), checksum(0) {}

OperationLogRecord::SimpleQuadruple::SimpleQuadruple() {}

OperationLogRecord::SimpleQuadruple::SimpleQuadruple(const std::string& op, const std::string& arg1,
    const std::string& arg2, const std::string& result)
    : op(op), arg1(arg1), arg2(arg2), result(result) {
}

OperationLogRecord::ExecutionResult::ExecutionResult() : success(false), affected_rows(0), execution_time_ms(0) {}

OperationLogRecord::PerformanceInfo::PerformanceInfo() : pages_read(0), pages_written(0), index_lookups(0) {}

OperationLogRecord::OperationLogRecord() : log_id(0), timestamp(0), log_level(0) {}

// ========== WALBuffer 实现 ==========
WALBuffer::WALBuffer() : buffer_size(0), current_pos(0) {}

WALBuffer::~WALBuffer() {}

void WALBuffer::Initialize(size_t size) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    buffer_size = size;
    buffer.resize(buffer_size);
    current_pos = 0;
}

bool WALBuffer::WriteRecord(const WALLogRecord& record) {
    std::lock_guard<std::mutex> lock(buffer_mutex);

    auto serialized = SerializeRecord(record);
    if (current_pos + serialized.size() > buffer_size) {
        return false; // 缓冲区满
    }

    std::memcpy(buffer.data() + current_pos, serialized.data(), serialized.size());
    current_pos += serialized.size();
    return true;
}

void WALBuffer::FlushToFile(std::ofstream& file) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (current_pos > 0) {
        file.write(buffer.data(), current_pos);
        file.flush();
        current_pos = 0;
    }
}

bool WALBuffer::ShouldFlush() const {
    return current_pos > buffer_size * 0.8; // 80%满时刷新
}

size_t WALBuffer::GetCurrentSize() const {
    return current_pos;
}

void WALBuffer::Clear() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    current_pos = 0;
}

std::vector<char> WALBuffer::SerializeRecord(const WALLogRecord& record) {
    std::vector<char> data;

    // 写入固定长度的头部信息
    data.insert(data.end(), reinterpret_cast<const char*>(&record.lsn),
        reinterpret_cast<const char*>(&record.lsn) + sizeof(record.lsn));
    data.insert(data.end(), reinterpret_cast<const char*>(&record.transaction_id),
        reinterpret_cast<const char*>(&record.transaction_id) + sizeof(record.transaction_id));
    data.insert(data.end(), reinterpret_cast<const char*>(&record.timestamp),
        reinterpret_cast<const char*>(&record.timestamp) + sizeof(record.timestamp));
    data.insert(data.end(), reinterpret_cast<const char*>(&record.record_type),
        reinterpret_cast<const char*>(&record.record_type) + sizeof(record.record_type));

    // 写入四元式信息
    data.insert(data.end(), reinterpret_cast<const char*>(&record.quad_info.op_code),
        reinterpret_cast<const char*>(&record.quad_info.op_code) + sizeof(record.quad_info.op_code));

    // 写入字符串（长度+内容）
    uint32_t table_name_len = record.quad_info.table_name.length();
    data.insert(data.end(), reinterpret_cast<const char*>(&table_name_len),
        reinterpret_cast<const char*>(&table_name_len) + sizeof(table_name_len));
    data.insert(data.end(), record.quad_info.table_name.begin(), record.quad_info.table_name.end());

    uint32_t condition_len = record.quad_info.condition.length();
    data.insert(data.end(), reinterpret_cast<const char*>(&condition_len),
        reinterpret_cast<const char*>(&condition_len) + sizeof(condition_len));
    data.insert(data.end(), record.quad_info.condition.begin(), record.quad_info.condition.end());

    uint32_t sql_len = record.quad_info.original_sql.length();
    data.insert(data.end(), reinterpret_cast<const char*>(&sql_len),
        reinterpret_cast<const char*>(&sql_len) + sizeof(sql_len));
    data.insert(data.end(), record.quad_info.original_sql.begin(), record.quad_info.original_sql.end());

    data.insert(data.end(), reinterpret_cast<const char*>(&record.quad_info.affected_rows),
        reinterpret_cast<const char*>(&record.quad_info.affected_rows) + sizeof(record.quad_info.affected_rows));

    // 写入数据变更信息
    uint32_t changes_count = record.changes.size();
    data.insert(data.end(), reinterpret_cast<const char*>(&changes_count),
        reinterpret_cast<const char*>(&changes_count) + sizeof(changes_count));

    for (const auto& change : record.changes) {
        data.insert(data.end(), reinterpret_cast<const char*>(&change.page_id),
            reinterpret_cast<const char*>(&change.page_id) + sizeof(change.page_id));
        data.insert(data.end(), reinterpret_cast<const char*>(&change.slot_id),
            reinterpret_cast<const char*>(&change.slot_id) + sizeof(change.slot_id));
        data.insert(data.end(), reinterpret_cast<const char*>(&change.before_length),
            reinterpret_cast<const char*>(&change.before_length) + sizeof(change.before_length));
        data.insert(data.end(), reinterpret_cast<const char*>(&change.after_length),
            reinterpret_cast<const char*>(&change.after_length) + sizeof(change.after_length));

        data.insert(data.end(), change.before_image.begin(), change.before_image.end());
        data.insert(data.end(), change.after_image.begin(), change.after_image.end());
    }

    // 计算校验和
    uint32_t checksum = CalculateChecksum(data);
    data.insert(data.end(), reinterpret_cast<const char*>(&checksum),
        reinterpret_cast<const char*>(&checksum) + sizeof(checksum));

    return data;
}

uint32_t WALBuffer::CalculateChecksum(const std::vector<char>& data) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        checksum += static_cast<uint32_t>(data[i]) * (i + 1);
    }
    return checksum;
}

// ========== LogFileManager 实现 ==========
LogFileManager::LogFileManager(const LogConfigInfo& cfg, const std::string& db)
    : config(cfg), db_name(db) {
    log_directory = "HAODB/" + db_name + "/logs/";
    CreateLogDirectory();
}

LogFileManager::~LogFileManager() {}

bool LogFileManager::CreateLogDirectory() {
    try {
        if (!fs::exists(log_directory)) {
            fs::create_directories(log_directory);
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create log directory: " << e.what() << std::endl;
        return false;
    }
}

std::string LogFileManager::GetWALFilePath() {
    if (current_wal_file.empty()) {
        current_wal_file = log_directory + db_name + "_wal.log";
    }
    return current_wal_file;
}

std::string LogFileManager::GetOperationLogPath() {
    if (current_op_file.empty()) {
        std::string timestamp = GetCurrentTimestamp();
        current_op_file = log_directory + db_name + "_ops_" + timestamp + ".log";
    }
    return current_op_file;
}

std::string LogFileManager::GetErrorLogPath() {
    if (current_error_file.empty()) {
        std::string timestamp = GetCurrentTimestamp();
        current_error_file = log_directory + db_name + "_error_" + timestamp + ".log";
    }
    return current_error_file;
}

void LogFileManager::RotateLogsIfNeeded() {
    if (!config.enable_log_rotation) return;

    auto op_size = GetFileSize(GetOperationLogPath());
    auto error_size = GetFileSize(GetErrorLogPath());

    uint64_t max_size = static_cast<uint64_t>(config.log_file_size_mb) * 1024 * 1024;

    if (op_size >= max_size) {
        RotateFile(current_op_file);
        current_op_file.clear();
    }

    if (error_size >= max_size) {
        RotateFile(current_error_file);
        current_error_file.clear();
    }

    CleanupOldLogs();
}

void LogFileManager::CleanupOldLogs() {
    auto op_files = GetLogFiles(db_name + "_ops_");
    auto error_files = GetLogFiles(db_name + "_error_");

    if (op_files.size() > config.log_file_count) {
        std::sort(op_files.begin(), op_files.end());
        for (size_t i = 0; i < op_files.size() - config.log_file_count; ++i) {
            try {
                fs::remove(log_directory + op_files[i]);
            }
            catch (...) {}
        }
    }

    if (error_files.size() > config.log_file_count) {
        std::sort(error_files.begin(), error_files.end());
        for (size_t i = 0; i < error_files.size() - config.log_file_count; ++i) {
            try {
                fs::remove(log_directory + error_files[i]);
            }
            catch (...) {}
        }
    }
}

std::string LogFileManager::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

uint64_t LogFileManager::GetFileSize(const std::string& filepath) {
    try {
        if (fs::exists(filepath)) {
            return fs::file_size(filepath);
        }
    }
    catch (const std::exception&) {}
    return 0;
}

void LogFileManager::RotateFile(const std::string& filepath) {
    if (!fs::exists(filepath)) return;

    std::string timestamp = GetCurrentTimestamp();
    std::string new_path = filepath + "." + timestamp;

    try {
        fs::rename(filepath, new_path);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to rotate log file: " << e.what() << std::endl;
    }
}

std::vector<std::string> LogFileManager::GetLogFiles(const std::string& pattern) {
    std::vector<std::string> files;

    try {
        for (const auto& entry : fs::directory_iterator(log_directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(pattern) == 0) {
                    files.push_back(filename);
                }
            }
        }
    }
    catch (const std::exception&) {}

    return files;
}

// ========== TransactionManager 实现 ==========
TransactionManager::TransactionManager() : next_transaction_id(1) {}

uint32_t TransactionManager::BeginTransaction() {
    std::lock_guard<std::mutex> lock(txn_mutex);
    uint32_t txn_id = next_transaction_id.fetch_add(1);
    active_transactions[txn_id] = false;
    return txn_id;
}

void TransactionManager::CommitTransaction(uint32_t txn_id) {
    std::lock_guard<std::mutex> lock(txn_mutex);
    auto it = active_transactions.find(txn_id);
    if (it != active_transactions.end()) {
        it->second = true;
    }
}

void TransactionManager::AbortTransaction(uint32_t txn_id) {
    std::lock_guard<std::mutex> lock(txn_mutex);
    active_transactions.erase(txn_id);
}

bool TransactionManager::IsTransactionActive(uint32_t txn_id) {
    std::lock_guard<std::mutex> lock(txn_mutex);
    return active_transactions.find(txn_id) != active_transactions.end();
}

std::vector<uint32_t> TransactionManager::GetActiveTransactions() {
    std::lock_guard<std::mutex> lock(txn_mutex);
    std::vector<uint32_t> active_txns;
    for (const auto& pair : active_transactions) {
        if (!pair.second) {
            active_txns.push_back(pair.first);
        }
    }
    return active_txns;
}

// ========== RecoveryManager 实现 ==========
RecoveryManager::RecoveryManager(const std::string& db, LogFileManager* fm)
    : db_name(db), file_manager(fm) {
}

bool RecoveryManager::PerformCrashRecovery() {
    try {
        auto wal_records = ReadWALLog();

        std::map<uint32_t, bool> transaction_status;

        for (const auto& record : wal_records) {
            switch (record.record_type) {
            case WALLogRecord::TXN_BEGIN:
                transaction_status[record.transaction_id] = false;
                break;
            case WALLogRecord::TXN_COMMIT:
                transaction_status[record.transaction_id] = true;
                break;
            case WALLogRecord::TXN_ABORT:
                transaction_status.erase(record.transaction_id);
                break;
            }
        }

        for (const auto& record : wal_records) {
            auto it = transaction_status.find(record.transaction_id);
            if (it != transaction_status.end() && it->second) {
                if (record.record_type == WALLogRecord::INSERT_OP ||
                    record.record_type == WALLogRecord::UPDATE_OP) {
                    for (const auto& change : record.changes) {
                        RestoreRecord(record.quad_info.table_name,
                            change.page_id, change.slot_id,
                            change.after_image);
                    }
                }
                else if (record.record_type == WALLogRecord::DELETE_OP) {
                    for (const auto& change : record.changes) {
                        DeleteRecord(record.quad_info.table_name,
                            change.page_id, change.slot_id);
                    }
                }
            }
        }

        for (auto it = wal_records.rbegin(); it != wal_records.rend(); ++it) {
            const auto& record = *it;
            auto status_it = transaction_status.find(record.transaction_id);
            if (status_it != transaction_status.end() && !status_it->second) {
                if (record.record_type == WALLogRecord::INSERT_OP) {
                    for (const auto& change : record.changes) {
                        DeleteRecord(record.quad_info.table_name,
                            change.page_id, change.slot_id);
                    }
                }
                else if (record.record_type == WALLogRecord::DELETE_OP) {
                    for (const auto& change : record.changes) {
                        RestoreRecord(record.quad_info.table_name,
                            change.page_id, change.slot_id,
                            change.before_image);
                    }
                }
                else if (record.record_type == WALLogRecord::UPDATE_OP) {
                    for (const auto& change : record.changes) {
                        RestoreRecord(record.quad_info.table_name,
                            change.page_id, change.slot_id,
                            change.before_image);
                    }
                }
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Crash recovery failed: " << e.what() << std::endl;
        return false;
    }
}

bool RecoveryManager::UndoDelete(uint64_t log_id) {
    auto records = FindWALRecordsByLogId(log_id);

    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        const auto& record = *it;
        if (record.record_type == WALLogRecord::DELETE_OP) {
            for (const auto& change : record.changes) {
                if (!change.before_image.empty()) {
                    RestoreRecord(record.quad_info.table_name,
                        change.page_id, change.slot_id,
                        change.before_image);
                }
            }
            return true;
        }
    }

    return false;
}

bool RecoveryManager::UndoUpdate(uint64_t log_id) {
    auto records = FindWALRecordsByLogId(log_id);

    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        const auto& record = *it;
        if (record.record_type == WALLogRecord::UPDATE_OP) {
            for (const auto& change : record.changes) {
                if (!change.before_image.empty()) {
                    RestoreRecord(record.quad_info.table_name,
                        change.page_id, change.slot_id,
                        change.before_image);
                }
            }
            return true;
        }
    }

    return false;
}

bool RecoveryManager::UndoInsert(uint64_t log_id) {
    auto records = FindWALRecordsByLogId(log_id);

    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        const auto& record = *it;
        if (record.record_type == WALLogRecord::INSERT_OP) {
            for (const auto& change : record.changes) {
                DeleteRecord(record.quad_info.table_name,
                    change.page_id, change.slot_id);
            }
            return true;
        }
    }

    return false;
}

bool RecoveryManager::UndoTransaction(uint32_t transaction_id) {
    auto records = FindWALRecordsByTransaction(transaction_id);

    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        const auto& record = *it;
        // 实现事务撤销逻辑
    }

    return true;
}

bool RecoveryManager::RedoTransaction(uint32_t transaction_id) {
    auto records = FindWALRecordsByTransaction(transaction_id);

    for (const auto& record : records) {
        // 实现事务重做逻辑
    }

    return true;
}

std::vector<WALLogRecord> RecoveryManager::ReadWALLog() {
    std::vector<WALLogRecord> records;

    try {
        std::string wal_path = file_manager->GetWALFilePath();
        std::ifstream file(wal_path, std::ios::binary);

        if (!file.is_open()) {
            return records;
        }

        // 简化实现：实际需要完整的反序列化逻辑
        while (file.good()) {
            uint64_t lsn;
            if (!file.read(reinterpret_cast<char*>(&lsn), sizeof(lsn))) {
                break;
            }

            WALLogRecord record;
            record.lsn = lsn;
            records.push_back(record);
        }

        file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to read WAL log: " << e.what() << std::endl;
    }

    return records;
}

std::vector<WALLogRecord> RecoveryManager::FindWALRecordsByLogId(uint64_t log_id) {
    auto all_records = ReadWALLog();
    std::vector<WALLogRecord> result;

    for (const auto& record : all_records) {
        result.push_back(record);
    }

    return result;
}

std::vector<WALLogRecord> RecoveryManager::FindWALRecordsByTransaction(uint32_t txn_id) {
    auto all_records = ReadWALLog();
    std::vector<WALLogRecord> result;

    for (const auto& record : all_records) {
        if (record.transaction_id == txn_id) {
            result.push_back(record);
        }
    }

    return result;
}

WALLogRecord RecoveryManager::DeserializeWALRecord(const std::vector<char>& data) {
    WALLogRecord record;
    // 简化实现
    return record;
}

bool RecoveryManager::RestoreRecord(const std::string& table_name, uint32_t page_id,
    uint32_t slot_id, const std::vector<char>& data) {
    std::cout << "Restoring record in table " << table_name
        << " at page " << page_id << " slot " << slot_id << std::endl;
    return true;
}

bool RecoveryManager::DeleteRecord(const std::string& table_name, uint32_t page_id, uint32_t slot_id) {
    std::cout << "Deleting record in table " << table_name
        << " at page " << page_id << " slot " << slot_id << std::endl;
    return true;
}

// ========== DatabaseLogger 实现 ==========
DatabaseLogger::DatabaseLogger(const LogConfigInfo& cfg, const std::string& db)
    : config(cfg), db_name(db), stop_background(false), next_lsn(1), next_log_id(1), background_running(false) {

    file_manager = std::make_unique<LogFileManager>(config, db_name);
    wal_buffer = std::make_unique<WALBuffer>();
    txn_manager = std::make_unique<TransactionManager>();
    recovery_manager = std::make_unique<RecoveryManager>(db_name, file_manager.get());
}

DatabaseLogger::~DatabaseLogger() {
    Shutdown();
}

bool DatabaseLogger::Initialize() {
    try {
        wal_buffer->Initialize(config.wal_buffer_size_mb * 1024 * 1024);

        if (config.enable_wal) {
            wal_file.open(file_manager->GetWALFilePath(), std::ios::binary | std::ios::app);
            if (!wal_file.is_open()) {
                std::cerr << "Failed to open WAL file" << std::endl;
                return false;
            }
        }

        operation_file.open(file_manager->GetOperationLogPath(), std::ios::app);
        if (!operation_file.is_open()) {
            std::cerr << "Failed to open operation log file" << std::endl;
            return false;
        }

        error_file.open(file_manager->GetErrorLogPath(), std::ios::app);
        if (!error_file.is_open()) {
            std::cerr << "Failed to open error log file" << std::endl;
            return false;
        }

        stop_background = false;
        background_running = true;
        background_thread = std::thread(&DatabaseLogger::BackgroundWorker, this);

        std::cout << "DatabaseLogger initialized for database: " << db_name << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize DatabaseLogger: " << e.what() << std::endl;
        return false;
    }
}

void DatabaseLogger::Shutdown() {
    stop_background = true;

    if (background_running && background_thread.joinable()) {
        background_thread.join();
        background_running = false;
    }

    if (config.enable_wal) {
        FlushWALBuffer();
        if (wal_file.is_open()) {
            wal_file.close();
        }
    }

    if (operation_file.is_open()) {
        operation_file.close();
    }

    if (error_file.is_open()) {
        error_file.close();
    }

    std::cout << "DatabaseLogger shutdown for database: " << db_name << std::endl;
}

uint32_t DatabaseLogger::BeginTransaction() {
    uint32_t txn_id = txn_manager->BeginTransaction();

    if (config.enable_wal) {
        WALLogRecord record;
        record.lsn = GenerateLSN();
        record.transaction_id = txn_id;
        record.timestamp = GetCurrentTimestamp();
        record.record_type = WALLogRecord::TXN_BEGIN;

        WriteWALLog(record);
    }

    return txn_id;
}

void DatabaseLogger::CommitTransaction(uint32_t txn_id) {
    txn_manager->CommitTransaction(txn_id);

    if (config.enable_wal) {
        WALLogRecord record;
        record.lsn = GenerateLSN();
        record.transaction_id = txn_id;
        record.timestamp = GetCurrentTimestamp();
        record.record_type = WALLogRecord::TXN_COMMIT;

        WriteWALLog(record);
        FlushWALBuffer();
    }
}

void DatabaseLogger::AbortTransaction(uint32_t txn_id) {
    txn_manager->AbortTransaction(txn_id);

    if (config.enable_wal) {
        WALLogRecord record;
        record.lsn = GenerateLSN();
        record.transaction_id = txn_id;
        record.timestamp = GetCurrentTimestamp();
        record.record_type = WALLogRecord::TXN_ABORT;

        WriteWALLog(record);
    }
}

uint64_t DatabaseLogger::LogQuadrupleExecution(
    uint32_t transaction_id,
    const std::string& original_sql,
    const std::vector<OperationLogRecord::SimpleQuadruple>& quads,
    const std::string& session_id,
    const std::string& user_name) {

    std::lock_guard<std::mutex> lock(log_mutex);

    OperationLogRecord op_log;
    op_log.log_id = GenerateLogId();
    op_log.timestamp = GetCurrentTimestamp();
    op_log.session_id = session_id;
    op_log.user_name = user_name;
    op_log.log_level = config.log_level_code;
    op_log.original_sql = original_sql;
    op_log.quad_sequence = quads;

    WriteOperationLog(op_log);

    return op_log.log_id;
}

void DatabaseLogger::LogDataChange(
    uint32_t transaction_id,
    uint8_t operation_type,
    const std::string& table_name,
    uint32_t page_id,
    uint32_t slot_id,
    const std::vector<char>& before_data,
    const std::vector<char>& after_data) {

    if (!config.enable_wal) return;

    WALLogRecord record;
    record.lsn = GenerateLSN();
    record.transaction_id = transaction_id;
    record.timestamp = GetCurrentTimestamp();
    record.record_type = operation_type;

    record.quad_info.op_code = operation_type;
    record.quad_info.table_name = table_name;
    record.quad_info.affected_rows = 1;

    WALLogRecord::DataChange change;
    change.page_id = page_id;
    change.slot_id = slot_id;
    change.before_length = before_data.size();
    change.after_length = after_data.size();
    change.before_image = before_data;
    change.after_image = after_data;

    record.changes.push_back(change);

    WriteWALLog(record);

    if (config.sync_mode_code == 2) {
        FlushWALBuffer();
    }
    else if (config.sync_mode_code == 1) {
        if (wal_buffer->ShouldFlush()) {
            FlushWALBuffer();
        }
    }
}

void DatabaseLogger::LogError(const std::string& error_message, const std::string& context) {
    std::lock_guard<std::mutex> lock(log_mutex);

    auto timestamp = GetCurrentTimestamp();
    if (error_file.is_open()) {
        error_file << "[" << timestamp << "] ERROR: " << error_message;
        if (!context.empty()) {
            error_file << " (Context: " << context << ")";
        }
        error_file << std::endl;
        error_file.flush();
    }
}

void DatabaseLogger::WriteWALLog(const WALLogRecord& record) {
    if (!wal_buffer->WriteRecord(record)) {
        FlushWALBuffer();
        wal_buffer->WriteRecord(record);
    }
}

void DatabaseLogger::WriteOperationLog(const OperationLogRecord& record) {
    std::string log_line = SerializeOperationLog(record);
    if (operation_file.is_open()) {
        operation_file << log_line << std::endl;
        operation_file.flush();
    }
}

void DatabaseLogger::FlushWALBuffer() {
    if (config.enable_wal && wal_file.is_open()) {
        wal_buffer->FlushToFile(wal_file);
    }
}

void DatabaseLogger::BackgroundWorker() {
    while (!stop_background) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (config.sync_mode_code == 0 && wal_buffer->ShouldFlush()) {
            FlushWALBuffer();
        }

        file_manager->RotateLogsIfNeeded();

        if (wal_buffer->GetCurrentSize() >= config.wal_checkpoint_size_mb * 1024 * 1024) {
            FlushWALBuffer();
        }
    }
}

uint64_t DatabaseLogger::GenerateLSN() {
    return next_lsn.fetch_add(1);
}

uint64_t DatabaseLogger::GenerateLogId() {
    return next_log_id.fetch_add(1);
}

uint64_t DatabaseLogger::GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string DatabaseLogger::SerializeOperationLog(const OperationLogRecord& record) {
    std::stringstream ss;
    ss << "[" << record.timestamp << "] "
        << "[" << (int)record.log_level << "] "
        << "[TXN:" << record.log_id << "] "
        << "[User:" << record.user_name << "] "
        << "[Session:" << record.session_id << "] "
        << "SQL: " << record.original_sql << " | "
        << "Quads: ";

    for (size_t i = 0; i < record.quad_sequence.size(); ++i) {
        const auto& quad = record.quad_sequence[i];
        ss << "(" << quad.op << "," << quad.arg1 << "," << quad.arg2 << "," << quad.result << ")";
        if (i < record.quad_sequence.size() - 1) {
            ss << ",";
        }
    }

    ss << " | Result: " << (record.result.success ? "SUCCESS" : "FAILED")
        << " | Rows: " << record.result.affected_rows
        << " | Time: " << record.result.execution_time_ms << "ms";

    if (!record.result.error_message.empty()) {
        ss << " | Error: " << record.result.error_message;
    }

    return ss.str();
}

// ========== LoggerFactory 实现 ==========
std::unique_ptr<DatabaseLogger> LoggerFactory::CreateLogger(
    const std::string& db_name,
    CatalogManager* catalog) {

    LogConfigInfo config;
    if (catalog) {
        // 从catalog获取配置
        config = catalog->GetLogConfig(DBName);
        // config = GetDefaultConfig(); // 暂时使用默认配置
    }
    else {
        config = GetDefaultConfig();
    }

    auto logger = std::make_unique<DatabaseLogger>(config, db_name);
    if (!logger->Initialize()) {
        return nullptr;
    }

    return logger;
}

LogConfigInfo LoggerFactory::GetDefaultConfig() {
    return LogConfigInfo();
}
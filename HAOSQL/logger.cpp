#define _CRT_SECURE_NO_WARNINGS
#include "logger.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

// ========== 结构体构造函数实现 ==========
// WALLogRecord::QuadrupleInfo::QuadrupleInfo() : op_code(0), affected_rows(0) {}

WALLogRecord::DataChange::DataChange() : page_id(0), slot_id(0), length(0), record_type(0) {}

WALLogRecord::WALLogRecord() : lsn(0), transaction_id(0), timestamp(0), record_size(0), checksum(0) {}

//OperationLogRecord::SimpleQuadruple::SimpleQuadruple() {}
//
//OperationLogRecord::SimpleQuadruple::SimpleQuadruple(const std::string& op, const std::string& arg1,
//    const std::string& arg2, const std::string& result)
//    : op(op), arg1(arg1), arg2(arg2), result(result) {
//}

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

bool WALBuffer::read_exact(std::istream& in, void* buf, std::size_t n) {
    return static_cast<bool>(in.read(reinterpret_cast<char*>(buf),
        static_cast<std::streamsize>(n)));
}

template<typename T>
bool WALBuffer::read_pod(std::istream& in, T& v) {
    static_assert(std::is_trivially_copyable_v<T>, "T must be POD");
    return read_exact(in, &v, sizeof(T));
}

// 读取长度前缀为 uint32_t 的字符串，同时把“长度字段 + 内容字节”追加到 payload
bool WALBuffer::read_len_prefixed_string(std::istream& in, std::string& s, std::vector<char>& payload) {
    uint32_t n = 0;
    if (!read_pod(in, n)) return false;
    // 先把长度字节也计入 payload（与写入一致）
    payload.insert(payload.end(),
        reinterpret_cast<const char*>(&n),
        reinterpret_cast<const char*>(&n) + sizeof(n));

    // 防御：限制长度，避免损坏文件导致巨量分配
    if (n > (1u << 28)) return false; // 256MB 上限，可按需调整

    s.resize(n);
    if (n == 0) return true;

    if (!read_exact(in, s.data(), n)) return false;

    // 字面内容也加入 payload
    payload.insert(payload.end(), s.begin(), s.end());
    return true;
}

// 读取长度为 before_length/after_length 的原始字节数组，并把其内容也计入 payload
bool WALBuffer::read_bytes_exact(std::istream& in, std::vector<char>& out, uint32_t n, std::vector<char>& payload) {
    if (n > (1u << 28)) return false; // 防御：上限可调
    out.resize(n);
    if (n == 0) return true;
    if (!read_exact(in, out.data(), n)) return false;
    payload.insert(payload.end(), out.begin(), out.end());
    return true;
}

// 反序列化单条WALLogRecord（与写入顺序严格一致）
bool WALBuffer::DeserializeWALLogRecord(std::istream& in, WALLogRecord& rec) {
    std::vector<char> payload; //用来重算checksum，收集“除 checksum 外的所有字节”
    payload.reserve(256);      //预设小容量，避免频繁扩容

    auto append_pod_to_payload = [&](const auto& v) {
        const char* p = reinterpret_cast<const char*>(&v);
        payload.insert(payload.end(), p, p + sizeof(v));
        };

    // 固定头部
    if (!read_pod(in, rec.lsn))            return false; append_pod_to_payload(rec.lsn);
    if (!read_pod(in, rec.transaction_id)) return false; append_pod_to_payload(rec.transaction_id);
    if (!read_pod(in, rec.timestamp))      return false; append_pod_to_payload(rec.timestamp);
    if (!read_pod(in, rec.withdraw))       return false; append_pod_to_payload(rec.withdraw);
    if (!read_pod(in, rec.record_size))    return false; append_pod_to_payload(rec.record_size);

    // 四元式信息
    /*if (!read_pod(in, rec.quad_info.op_code)) return false; append_pod_to_payload(rec.quad_info.op_code);

    if (!read_len_prefixed_string(in, rec.quad_info.table_name, payload))   return false;
    if (!read_len_prefixed_string(in, rec.quad_info.condition, payload))  return false;
    if (!read_len_prefixed_string(in, rec.quad_info.original_sql, payload)) return false;

    if (!read_pod(in, rec.quad_info.affected_rows)) return false; append_pod_to_payload(rec.quad_info.affected_rows);*/

    // changes数组（先个数，再每条的字段 + 两段 image 原始字节）
    uint32_t changes_count = 0;
    if (!read_pod(in, changes_count)) return false; append_pod_to_payload(changes_count);

    // 限制条目数
    if (changes_count > (1u << 26)) return false; // ~67M，可按需调小
    rec.changes.clear();
    rec.changes.reserve(changes_count);

    for (uint32_t i = 0; i < changes_count; ++i) {
        WALLogRecord::DataChange c{};

        if (!read_pod(in, c.page_id))       return false; append_pod_to_payload(c.page_id);
        if (!read_pod(in, c.slot_id))       return false; append_pod_to_payload(c.slot_id);
        if (!read_pod(in, c.length)) return false; append_pod_to_payload(c.length);
        if (!read_pod(in, c.record_type))  return false; append_pod_to_payload(c.record_type);

        rec.changes.emplace_back(std::move(c));
    }

    // 末尾checksum（不计入payload）
    uint32_t checksum_on_disk = 0;
    if (!read_pod(in, checksum_on_disk)) return false;

    // 计算checksum验证
    const uint32_t checksum_calc = CalculateChecksum(payload);
    if (checksum_calc != checksum_on_disk) {
        return false; // 校验失败 -> 视为损坏记录
    }

    return true;
}

std::vector<char> WALBuffer::SerializeRecord(const WALLogRecord& record) {
    std::vector<char> data;
    data.reserve(256); // 按需调整，减少扩容

    auto append_pod = [&](const auto& v) {
        const char* p = reinterpret_cast<const char*>(&v);
        data.insert(data.end(), p, p + sizeof(v));
        };
    auto append_str_with_len = [&](const std::string& s) {
        uint32_t n = static_cast<uint32_t>(s.size());
        append_pod(n);
        data.insert(data.end(), s.begin(), s.end());
        };

    // 固定头
    append_pod(record.lsn);
    append_pod(record.transaction_id);
    append_pod(record.timestamp);
    append_pod(record.withdraw);

    // 预留record_size（4字节占位，稍后回填）
    const std::size_t record_size_pos = data.size();
    {
        uint32_t placeholder = 0;
        append_pod(placeholder);
    }

    // 四元式信息
    /*append_pod(record.quad_info.op_code);
    append_str_with_len(record.quad_info.table_name);
    append_str_with_len(record.quad_info.condition);
    append_str_with_len(record.quad_info.original_sql);
    append_pod(record.quad_info.affected_rows);*/

    // 数据变更信息
    {
        uint32_t changes_count = static_cast<uint32_t>(record.changes.size());
        append_pod(changes_count);

        for (const auto& change : record.changes) {
            append_pod(change.page_id);
            append_pod(change.slot_id);
            append_pod(change.length);
            append_pod(change.record_type);
        }
    }

    // 回填 record_size
    // 定义：record_size = （从record_size字段之后到末尾的总长度）= (当前data.size() - (record_size_pos + 4)) + sizeof(checksum)
    {
        const uint32_t record_size_val =
            static_cast<uint32_t>((data.size() - (record_size_pos + sizeof(uint32_t))) + sizeof(uint32_t));

        // 回写到占位的4字节
        std::memcpy(&data[record_size_pos], &record_size_val, sizeof(record_size_val));
    }

    // 计算并追加checksum（对“除 checksum 本身以外的所有字节”做校验）
    {
        uint32_t checksum = CalculateChecksum(data);
        append_pod(checksum);
    }

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

//bool RecoveryManager::PerformCrashRecovery() {
//    try {
//        auto wal_records = ReadWALLog();
//
//        std::map<uint32_t, bool> transaction_status;
//
//        for (const auto& record : wal_records) {
//            switch (record.record_type) {
//            case WALLogRecord::TXN_BEGIN:
//                transaction_status[record.transaction_id] = false;
//                break;
//            case WALLogRecord::TXN_COMMIT:
//                transaction_status[record.transaction_id] = true;
//                break;
//            case WALLogRecord::TXN_ABORT:
//                transaction_status.erase(record.transaction_id);
//                break;
//            }
//        }
//
//        for (const auto& record : wal_records) {
//            auto it = transaction_status.find(record.transaction_id);
//            if (it != transaction_status.end() && it->second) {
//                if (record.record_type == WALLogRecord::INSERT_OP ||
//                    record.record_type == WALLogRecord::UPDATE_OP) {
//                    for (const auto& change : record.changes) {
//                        RestoreRecord(change.before_page_id, change.before_slot_id, change.before_length,
//                            change.after_page_id, change.after_slot_id, change.after_length);
//                    }
//                }
//                else if (record.record_type == WALLogRecord::DELETE_OP) {
//                    for (const auto& change : record.changes) {
//                        DeleteRecord(change.before_page_id, change.before_slot_id, change.before_length,
//                            change.after_page_id, change.after_slot_id, change.after_length);
//                    }
//                }
//            }
//        }
//
//        for (auto it = wal_records.rbegin(); it != wal_records.rend(); ++it) {
//            const auto& record = *it;
//            auto status_it = transaction_status.find(record.transaction_id);
//            if (status_it != transaction_status.end() && !status_it->second) {
//                if (record.record_type == WALLogRecord::INSERT_OP) {
//                    for (const auto& change : record.changes) {
//                        DeleteRecord(change.before_page_id, change.before_slot_id, change.before_length,
//                            change.after_page_id, change.after_slot_id, change.after_length);
//                    }
//                }
//                else if (record.record_type == WALLogRecord::DELETE_OP) {
//                    for (const auto& change : record.changes) {
//                        RestoreRecord(change.before_page_id, change.before_slot_id, change.before_length,
//                            change.after_page_id, change.after_slot_id, change.after_length);
//                    }
//                }
//                else if (record.record_type == WALLogRecord::UPDATE_OP) {
//                    for (const auto& change : record.changes) {
//                        RestoreRecord(change.before_page_id, change.before_slot_id, change.before_length,
//                            change.after_page_id, change.after_slot_id, change.after_length);
//                    }
//                }
//            }
//        }
//
//        return true;
//    }
//    catch (const std::exception& e) {
//        std::cerr << "Crash recovery failed: " << e.what() << std::endl;
//        return false;
//    }
//}

bool RecoveryManager::UndoDelete(WALLogRecord record) {
    
    /*
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
    */

    cout << "撤销删除" << endl;
    cout << "删除前内容" << endl;
    
    /*for (auto& change : record.changes)
    {
        for (auto& before : change.before_image)
        {
            cout << before;
        }
        cout << endl;
    }*/
    return true;
}

bool RecoveryManager::UndoUpdate(uint64_t log_id) {
    auto records = FindWALRecordsByLogId(log_id);

    /*for (auto it = records.rbegin(); it != records.rend(); ++it) {
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
    }*/

    return false;
}

bool RecoveryManager::UndoInsert(uint64_t log_id) {
    auto records = FindWALRecordsByLogId(log_id);

    /*for (auto it = records.rbegin(); it != records.rend(); ++it) {
        const auto& record = *it;
        if (record.record_type == WALLogRecord::INSERT_OP) {
            for (const auto& change : record.changes) {
                DeleteRecord(record.quad_info.table_name,
                    change.page_id, change.slot_id);
            }
            return true;
        }
    }*/

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
        std::ifstream file(file_manager->GetWALFilePath(), std::ios::binary);
        if (!file.is_open()) return records;

        while (true) {
            // 小心处理 EOF：peek 一下，避免在 EOF 上报“损坏”
            file.peek();
            if (!file.good()) break;

            WALLogRecord rec;
            auto pos = file.tellg();
            WALBuffer wal;

            if (!wal.DeserializeWALLogRecord(file, rec)) {
                std::cerr << "Corrupted WAL record at offset "
                    << static_cast<long long>(pos) << "\n";
                break;
            }
            records.emplace_back(std::move(rec));
        }
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

WALLogRecord RecoveryManager::FindLatestWALRecord() {
    auto records = ReadWALLog();

    // 过滤掉已经撤回，withdraw==true的操作
    std::vector<WALLogRecord> valid;
    std::copy_if(records.begin(), records.end(), std::back_inserter(valid),
        [](const WALLogRecord& r) { return !r.withdraw; });

    if (valid.empty()) {
        throw std::runtime_error("No valid WAL records found");
    }

    auto it = std::max_element(valid.begin(), valid.end(),
        [](const WALLogRecord& a, const WALLogRecord& b) {
            //cout << "a,time" << a.timestamp<<endl;
            //cout << "b,time" << b.timestamp << endl;
            return a.lsn < b.lsn;
            
        });

    return *it;
}

std::vector<WALLogRecord> RecoveryManager::FindFilteredWALRecords() {
    // 读取所有 WAL 记录
    auto records = ReadWALLog();

    // 过滤条件(INSERT\DELETE\UPDATE)
    std::vector<WALLogRecord> valid;
    std::copy_if(records.begin(), records.end(), std::back_inserter(valid),
        [](const WALLogRecord& r) {
            if (r.withdraw) return false;
            if (r.changes.empty()) return false;

            // changes[0].record_type 在1~4
            uint8_t type = r.changes[0].record_type;
            return (type >= 1 && type <= 4);
        });

    // 按lsn从大到小排序
    std::sort(valid.begin(), valid.end(),
        [](const WALLogRecord& a, const WALLogRecord& b) {
            return a.lsn > b.lsn;  // 降序
        });

    return valid;
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

bool RecoveryManager::RestoreRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,
    uint32_t after_page_id, uint32_t after_slot_id, uint32_t after_length) {
    std::cout << "Restoring record in table at page " << before_page_id << " slot " << before_slot_id << std::endl;
    return true;
}

bool RecoveryManager::DeleteRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,
    uint32_t after_page_id, uint32_t after_slot_id, uint32_t after_length) {
    std::cout << "Deleting record in table at page " << before_page_id << " slot " << before_slot_id << std::endl;
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

        WALLogRecord::DataChange dataChange;
        dataChange.record_type = (int)WALLogRecord::TXN_BEGIN;
        record.changes.push_back(dataChange);

        WriteWALLog(record);
    }

    return txn_id;
}

void DatabaseLogger::CommitTransaction(uint32_t txn_id, WALLogRecord& record) {
    txn_manager->CommitTransaction(txn_id);

    if (config.enable_wal) {
        record.lsn = GenerateLSN();
        record.timestamp = GetCurrentTimestamp();

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
        
        WALLogRecord::DataChange dataChange;
        dataChange.record_type = (int)WALLogRecord::TXN_ABORT;
        record.changes.push_back(dataChange);

        WriteWALLog(record);
    }
}

uint64_t DatabaseLogger::LogQuadrupleExecution(
    uint32_t transaction_id,
    const std::string& original_sql,
    // const std::vector<OperationLogRecord::SimpleQuadruple>& quads,
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
    // op_log.quad_sequence = quads;

    WriteOperationLog(op_log);

    return op_log.log_id;
}

uint64_t DatabaseLogger::LogQuadrupleExecution(
    uint32_t transaction_id,
    const std::string& original_sql,
    const std::vector<Quadruple>& quads,
    const std::string& user_name,
    const bool success,
    const uint64_t execution_time_ms,
    const std::string error_message,
    const std::string& session_id) {

    std::lock_guard<std::mutex> lock(log_mutex);

    OperationLogRecord op_log;
    op_log.log_id = GenerateLogId();
    op_log.timestamp = GetCurrentTimestamp();
    op_log.session_id = session_id;
    op_log.user_name = user_name;
    op_log.log_level = config.log_level_code;
    op_log.original_sql = original_sql;
    op_log.quad_sequence = quads;

    op_log.result.success = success;
    op_log.result.error_message = error_message;
    op_log.result.execution_time_ms = execution_time_ms;


    WriteOperationLog(op_log);

    return op_log.log_id;
}

//void DatabaseLogger::LogDataChange(
//    uint32_t transaction_id,
//    uint8_t operation_type,
//    uint32_t before_page_id,
//    uint32_t before_slot_id,
//    uint32_t after_page_id,
//    uint32_t after_slot_id,
//    uint32_t before_length,
//    uint32_t after_length) {
//
//    if (!config.enable_wal) return;
//
//    WALLogRecord record;
//    record.lsn = GenerateLSN();
//    record.transaction_id = transaction_id;
//    record.timestamp = GetCurrentTimestamp();
//
//    WALLogRecord::DataChange change;
//    change.page_id = before_page_id;
//    change.slot_id = before_slot_id;
//    change.length = before_length;
//    change.after_length = after_length;
//
//    record.changes.push_back(change);
//
//    WriteWALLog(record);
//
//    if (config.sync_mode_code == 2) {
//        FlushWALBuffer();
//    }
//    else if (config.sync_mode_code == 1) {
//        if (wal_buffer->ShouldFlush()) {
//            FlushWALBuffer();
//        }
//    }
//}

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
        config = catalog->GetLogConfig(db_name);
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
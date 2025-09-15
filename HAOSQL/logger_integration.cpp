#define _CRT_SECURE_NO_WARNINGS
#include "logger_integration.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <ctime>

// ========== 全局变量实现 ==========
std::string CURRENT_DB_NAME = "";
DatabaseLogger* CURRENT_LOGGER = nullptr;

// ========== GlobalLoggerManager 实现 ==========
std::map<std::string, std::unique_ptr<DatabaseLogger>> GlobalLoggerManager::loggers;
std::mutex GlobalLoggerManager::manager_mutex;

DatabaseLogger* GlobalLoggerManager::GetLogger(const std::string& db_name, CatalogManager* catalog) {
    std::lock_guard<std::mutex> lock(manager_mutex);

    auto it = loggers.find(db_name);
    if (it != loggers.end()) {
        return it->second.get();
    }

    auto logger = LoggerFactory::CreateLogger(db_name, catalog);
    if (logger) {
        DatabaseLogger* logger_ptr = logger.get();
        loggers[db_name] = std::move(logger);
        return logger_ptr;
    }

    return nullptr;
}

void GlobalLoggerManager::ShutdownAll() {
    std::lock_guard<std::mutex> lock(manager_mutex);
    for (auto& pair : loggers) {
        pair.second->Shutdown();
    }
    loggers.clear();
    CURRENT_LOGGER = nullptr;
}

DatabaseLogger* GlobalLoggerManager::GetDefaultLogger() {
    if (!CURRENT_DB_NAME.empty()) {
        return GetLogger(CURRENT_DB_NAME);
    }
    return nullptr;
}

// ========== LogConfigManager 实现 ==========
//LogConfigInfo LogConfigManager::LoadConfig(const std::string& config_file) {
//    LogConfigInfo config;
//    std::ifstream file(config_file);
//
//    if (!file.is_open()) {
//        std::cerr << "Warning: Could not open config file " << config_file
//            << ", using default configuration." << std::endl;
//        return config;
//    }
//
//    std::string line;
//    std::string current_section;
//
//    while (std::getline(file, line)) {
//        size_t comment_pos = line.find('#');
//        if (comment_pos != std::string::npos) {
//            line = line.substr(0, comment_pos);
//        }
//
//        line.erase(0, line.find_first_not_of(" \t"));
//        line.erase(line.find_last_not_of(" \t") + 1);
//
//        if (line.empty()) continue;
//
//        if (line.front() == '[' && line.back() == ']') {
//            current_section = line.substr(1, line.length() - 2);
//            continue;
//        }
//
//        size_t eq_pos = line.find('=');
//        if (eq_pos == std::string::npos) continue;
//
//        std::string key = line.substr(0, eq_pos);
//        std::string value = line.substr(eq_pos + 1);
//
//        key.erase(0, key.find_first_not_of(" \t"));
//        key.erase(key.find_last_not_of(" \t") + 1);
//        value.erase(0, value.find_first_not_of(" \t"));
//        value.erase(value.find_last_not_of(" \t") + 1);
//
//        if (current_section == "logger") {
//            if (key == "log_level") {
//                config.log_level_name = value;
//                config.log_level_code = LoggerUtils::ParseLogLevel(value);
//            }
//            else if (key == "log_file_size_mb") {
//                config.log_file_size_mb = std::stoul(value);
//            }
//            else if (key == "log_file_count") {
//                config.log_file_count = std::stoul(value);
//            }
//            else if (key == "enable_log_rotation") {
//                config.enable_log_rotation = (value == "true" || value == "1");
//            }
//        }
//        else if (current_section == "wal") {
//            if (key == "enable_wal") {
//                config.enable_wal = (value == "true" || value == "1");
//            }
//            else if (key == "wal_buffer_size_mb") {
//                config.wal_buffer_size_mb = std::stoul(value);
//            }
//            else if (key == "sync_mode") {
//                config.sync_mode_name = value;
//                config.sync_mode_code = LoggerUtils::ParseSyncMode(value);
//            }
//            else if (key == "wal_checkpoint_size_mb") {
//                config.wal_checkpoint_size_mb = std::stoul(value);
//            }
//        }
//    }
//
//    file.close();
//    return config;
//}
//
//bool LogConfigManager::SaveConfig(const LogConfigInfo& config, const std::string& config_file) {
//    std::ofstream file(config_file);
//    if (!file.is_open()) {
//        return false;
//    }
//
//    file << "# HAODB Logger Configuration\n";
//    file << "# Auto-generated configuration file\n\n";
//
//    file << "[logger]\n";
//    file << "log_level = " << config.log_level_name << "\n";
//    file << "log_file_size_mb = " << config.log_file_size_mb << "\n";
//    file << "log_file_count = " << config.log_file_count << "\n";
//    file << "enable_log_rotation = " << (config.enable_log_rotation ? "true" : "false") << "\n\n";
//
//    file << "[wal]\n";
//    file << "enable_wal = " << (config.enable_wal ? "true" : "false") << "\n";
//    file << "wal_buffer_size_mb = " << config.wal_buffer_size_mb << "\n";
//    file << "sync_mode = " << config.sync_mode_name << "\n";
//    file << "wal_checkpoint_size_mb = " << config.wal_checkpoint_size_mb << "\n";
//
//    file.close();
//    return true;
//}
//
//LogConfigInfo LogConfigManager::GetSystemDefaultConfig() {
//    return LogConfigInfo();
//}
//
//bool LogConfigManager::UpdateLogLevel(const std::string& db_name, uint8_t new_level) {
//    DatabaseLogger* logger = GlobalLoggerManager::GetLogger(db_name);
//    if (!logger) return false;
//
//    std::cout << "Log level updated for " << db_name << " to " << (int)new_level << std::endl;
//    return true;
//}
//
//bool LogConfigManager::UpdateSyncMode(const std::string& db_name, uint8_t new_mode) {
//    DatabaseLogger* logger = GlobalLoggerManager::GetLogger(db_name);
//    if (!logger) return false;
//
//    std::cout << "Sync mode updated for " << db_name << " to " << (int)new_mode << std::endl;
//    return true;
//}

// ========== SimpleLogger 实现 ==========
SimpleLogger::SimpleLogger(const std::string& db_name) : db_name(db_name), current_transaction_id(0) {
    logger = GlobalLoggerManager::GetLogger(db_name);
    if (!logger) {
        throw std::runtime_error("Failed to get logger for database: " + db_name);
    }
}

SimpleLogger::~SimpleLogger() {
    if (current_transaction_id != 0) {
        RollbackTransaction();
    }
}

void SimpleLogger::BeginTransaction() {
    if (current_transaction_id != 0) {
        throw std::runtime_error("Transaction already active");
    }
    current_transaction_id = logger->BeginTransaction();
}

void SimpleLogger::CommitTransaction() {
    if (current_transaction_id == 0) {
        throw std::runtime_error("No active transaction");
    }
    // logger->CommitTransaction(current_transaction_id);
    current_transaction_id = 0;
}

void SimpleLogger::RollbackTransaction() {
    if (current_transaction_id == 0) {
        throw std::runtime_error("No active transaction");
    }
    logger->AbortTransaction(current_transaction_id);
    current_transaction_id = 0;
}

/*
void SimpleLogger::LogSQL(const std::string& sql, const std::vector<OperationLogRecord::SimpleQuadruple>& quads, bool success) {
    uint32_t txn_id = current_transaction_id;
    if (txn_id == 0) {
        txn_id = logger->BeginTransaction();
    }

    logger->LogQuadrupleExecution(txn_id, sql, quads);

    if (current_transaction_id == 0) {
        if (success) {
            // logger->CommitTransaction(txn_id);
        }
        else {
            logger->AbortTransaction(txn_id);
        }
    }
}

void SimpleLogger::LogInsert(const std::string& table_name, const std::map<std::string, std::string>& data) {
    if (current_transaction_id == 0) {
        throw std::runtime_error("No active transaction for insert operation");
    }

    std::vector<OperationLogRecord::SimpleQuadruple> quads = {
        OperationLogRecord::SimpleQuadruple("INSERT", table_name, "values", "T1")
    };

    std::stringstream ss;
    ss << "INSERT INTO " << table_name << " VALUES (";
    bool first = true;
    for (const auto& pair : data) {
        if (!first) ss << ", ";
        ss << pair.second;
        first = false;
    }
    ss << ")";

    logger->LogQuadrupleExecution(current_transaction_id, ss.str(), quads);
}

void SimpleLogger::LogDelete(const std::string& table_name, const std::string& condition) {
    if (current_transaction_id == 0) {
        throw std::runtime_error("No active transaction for delete operation");
    }

    std::vector<OperationLogRecord::SimpleQuadruple> quads = {
        OperationLogRecord::SimpleQuadruple("DELETE", table_name, condition, "T1")
    };

    std::string sql = "DELETE FROM " + table_name + " WHERE " + condition;
    logger->LogQuadrupleExecution(current_transaction_id, sql, quads);
}

void SimpleLogger::LogUpdate(const std::string& table_name, const std::string& condition,
    const std::map<std::string, std::string>& changes) {
    if (current_transaction_id == 0) {
        throw std::runtime_error("No active transaction for update operation");
    }

    std::vector<OperationLogRecord::SimpleQuadruple> quads = {
        OperationLogRecord::SimpleQuadruple("UPDATE", table_name, condition, "T1")
    };

    std::stringstream ss;
    ss << "UPDATE " << table_name << " SET ";
    bool first = true;
    for (const auto& pair : changes) {
        if (!first) ss << ", ";
        ss << pair.first << " = " << pair.second;
        first = false;
    }
    ss << " WHERE " << condition;

    logger->LogQuadrupleExecution(current_transaction_id, ss.str(), quads);
}
*/

void SimpleLogger::LogError(const std::string& error_msg) {
    logger->LogError(error_msg);
}

void SimpleLogger::LogInfo(const std::string& info_msg) {
    logger->LogError(info_msg, "INFO");
}

bool SimpleLogger::UndoLastOperation() {
    RecoveryManager* recovery = logger->GetRecoveryManager();
    // return recovery->UndoDelete(1);
    return true;
}

bool SimpleLogger::RecoverFromCrash() {
    RecoveryManager* recovery = logger->GetRecoveryManager();
    return recovery->PerformCrashRecovery();
}

// ========== 带日志的算子实现（组合模式）==========
LoggedInsertComposition::LoggedInsertComposition(Table* t, const Row& r, const std::string& tname,
    DatabaseLogger* log, uint32_t txn_id)
    : table(t), row_data(r), logger(log), transaction_id(txn_id), table_name(tname) {
}

std::vector<Row> LoggedInsertComposition::execute() {
    try {
        if (logger) {
            std::vector<char> serialized_row = SerializeRow(row_data);
            std::vector<char> empty_before;
            // logger->LogDataChange(transaction_id, WALLogRecord::INSERT_OP, 0, 0, empty_before, serialized_row);
        }

        table->push_back(row_data);
        return {};
    }
    catch (const std::exception& e) {
        if (logger) {
            logger->LogError(e.what(), "INSERT_ERROR");
        }
        throw;
    }
}

std::vector<char> LoggedInsertComposition::SerializeRow(const Row& row) {
    std::stringstream ss;
    bool first = true;
    for (const auto& pair : row) {
        if (!first) ss << ",";
        ss << pair.first << ":" << pair.second;
        first = false;
    }

    std::string str = ss.str();
    return std::vector<char>(str.begin(), str.end());
}

LoggedDeleteComposition::LoggedDeleteComposition(Table* t, std::function<bool(const Row&)> pred,
    const std::string& tname, DatabaseLogger* log, uint32_t txn_id)
    : table(t), predicate(pred), logger(log), transaction_id(txn_id), table_name(tname) {
}

std::vector<Row> LoggedDeleteComposition::execute() {
    try {
        std::vector<Row> to_delete;

        for (auto it = table->begin(); it != table->end();) {
            if (predicate(*it)) {
                Row row_copy = *it;
                to_delete.push_back(row_copy);

                if (logger) {
                    std::vector<char> serialized_row = SerializeRow(row_copy);
                    std::vector<char> empty_after;
                    // logger->LogDataChange(transaction_id, WALLogRecord::DELETE_OP, table_name, 0, 0, serialized_row, empty_after);
                }

                it = table->erase(it);
            }
            else {
                ++it;
            }
        }

        return {};
    }
    catch (const std::exception& e) {
        if (logger) {
            logger->LogError(e.what(), "DELETE_ERROR");
        }
        throw;
    }
}

std::vector<char> LoggedDeleteComposition::SerializeRow(const Row& row) {
    std::stringstream ss;
    bool first = true;
    for (const auto& pair : row) {
        if (!first) ss << ",";
        ss << pair.first << ":" << pair.second;
        first = false;
    }

    std::string str = ss.str();
    return std::vector<char>(str.begin(), str.end());
}

LoggedUpdateComposition::LoggedUpdateComposition(Table* t, std::function<void(Row&)> u,
    const std::string& tname, DatabaseLogger* log, uint32_t txn_id,
    std::function<bool(const Row&)> p)
    : table(t), updater(u), predicate(p), logger(log),
    transaction_id(txn_id), table_name(tname) {
}

std::vector<Row> LoggedUpdateComposition::execute() {
    try {
        for (auto& row : *table) {
            if (!predicate || predicate(row)) {
                Row before = row;
                updater(row);
                Row after = row;

                if (logger) {
                    std::vector<char> before_data = SerializeRow(before);
                    std::vector<char> after_data = SerializeRow(after);

                    // logger->LogDataChange(transaction_id, WALLogRecord::UPDATE_OP, table_name, 0, 0, before_data, after_data);
                }
            }
        }

        return {};
    }
    catch (const std::exception& e) {
        if (logger) {
            logger->LogError(e.what(), "UPDATE_ERROR");
        }
        throw;
    }
}

std::vector<char> LoggedUpdateComposition::SerializeRow(const Row& row) {
    std::stringstream ss;
    bool first = true;
    for (const auto& pair : row) {
        if (!first) ss << ",";
        ss << pair.first << ":" << pair.second;
        first = false;
    }

    std::string str = ss.str();
    return std::vector<char>(str.begin(), str.end());
}

// ========== LoggedOperatorFactory 实现 ==========
std::unique_ptr<BaseOperator> LoggedOperatorFactory::CreateLoggedInsert(
    Table* table, const Row& row, const std::string& table_name,
    DatabaseLogger* logger, uint32_t transaction_id) {

    return std::make_unique<LoggedInsertComposition>(
        table, row, table_name, logger, transaction_id);
}

std::unique_ptr<BaseOperator> LoggedOperatorFactory::CreateLoggedDelete(
    Table* table, std::function<bool(const Row&)> predicate, const std::string& table_name,
    DatabaseLogger* logger, uint32_t transaction_id) {

    return std::make_unique<LoggedDeleteComposition>(
        table, predicate, table_name, logger, transaction_id);
}

std::unique_ptr<BaseOperator> LoggedOperatorFactory::CreateLoggedUpdate(
    Table* table, std::function<void(Row&)> updater,
    const std::string& table_name, DatabaseLogger* logger, uint32_t transaction_id,
    std::function<bool(const Row&)> predicate) {

    return std::make_unique<LoggedUpdateComposition>(
        table, updater, table_name, logger, transaction_id, predicate);
}

// ========== LogPerformanceMonitor 实现 ==========
void LogPerformanceMonitor::StartOperation(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    operation_start_times[operation_name] = std::chrono::steady_clock::now();
}

void LogPerformanceMonitor::EndOperation(const std::string& operation_name) {
    auto end_time = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(monitor_mutex);

    auto it = operation_start_times.find(operation_name);
    if (it != operation_start_times.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - it->second).count();

        operation_counts[operation_name]++;
        total_execution_times[operation_name] += duration;

        operation_start_times.erase(it);
    }
}

LogPerformanceMonitor::PerformanceStats LogPerformanceMonitor::GetStats(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(monitor_mutex);

    PerformanceStats stats;
    stats.total_operations = operation_counts[operation_name];
    stats.total_time_ms = total_execution_times[operation_name];
    stats.average_time_ms = stats.total_operations > 0 ?
        static_cast<double>(stats.total_time_ms) / stats.total_operations : 0.0;

    return stats;
}

void LogPerformanceMonitor::PrintAllStats() {
    std::lock_guard<std::mutex> lock(monitor_mutex);

    std::cout << "\n=== Performance Statistics ===" << std::endl;
    std::cout << std::setw(20) << "Operation"
        << std::setw(10) << "Count"
        << std::setw(15) << "Total(ms)"
        << std::setw(15) << "Average(ms)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (const auto& pair : operation_counts) {
        const std::string& op_name = pair.first;
        uint64_t count = pair.second;
        uint64_t total_time = total_execution_times[op_name];
        double avg_time = count > 0 ? static_cast<double>(total_time) / count : 0.0;

        std::cout << std::setw(20) << op_name
            << std::setw(10) << count
            << std::setw(15) << total_time
            << std::setw(15) << std::fixed << std::setprecision(2) << avg_time << std::endl;
    }
}

void LogPerformanceMonitor::ResetStats() {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    operation_counts.clear();
    total_execution_times.clear();
    operation_start_times.clear();
}

// ========== LogViewer 实现 ==========
LogViewer::LogViewer(const std::string& db_name) : db_name(db_name) {
    LogConfigInfo config = LoggerFactory::GetDefaultConfig();
    file_manager = std::make_unique<LogFileManager>(config, db_name);
}

LogViewer::~LogViewer() {}

std::vector<std::string> LogViewer::GetRecentOperations(int count) {
    std::vector<std::string> operations;

    try {
        std::string op_file = file_manager->GetOperationLogPath();
        std::ifstream file(op_file);

        if (file.is_open()) {
            std::string line;
            std::vector<std::string> all_lines;

            while (std::getline(file, line)) {
                all_lines.push_back(line);
            }

            int start = std::max(0, (int)all_lines.size() - count);
            for (int i = start; i < (int)all_lines.size(); ++i) {
                operations.push_back(all_lines[i]);
            }

            file.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading operation log: " << e.what() << std::endl;
    }

    return operations;
}

std::vector<std::string> LogViewer::GetRecentErrors(int count) {
    std::vector<std::string> errors;

    try {
        std::string error_file = file_manager->GetErrorLogPath();
        std::ifstream file(error_file);

        if (file.is_open()) {
            std::string line;
            std::vector<std::string> all_lines;

            while (std::getline(file, line)) {
                all_lines.push_back(line);
            }

            int start = std::max(0, (int)all_lines.size() - count);
            for (int i = start; i < (int)all_lines.size(); ++i) {
                errors.push_back(all_lines[i]);
            }

            file.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading error log: " << e.what() << std::endl;
    }

    return errors;
}

std::vector<std::string> LogViewer::SearchLogs(const std::string& pattern,
    const std::string& log_type) {
    std::vector<std::string> results;

    try {
        std::string log_file;
        if (log_type == "operation") {
            log_file = file_manager->GetOperationLogPath();
        }
        else if (log_type == "error") {
            log_file = file_manager->GetErrorLogPath();
        }
        else {
            return results;
        }

        std::ifstream file(log_file);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.find(pattern) != std::string::npos) {
                    results.push_back(line);
                }
            }
            file.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error searching logs: " << e.what() << std::endl;
    }

    return results;
}

bool LogViewer::ExportLogs(const std::string& output_file,
    const std::string& date_range) {
    try {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            return false;
        }

        out << "=== HAODB Log Export ===" << std::endl;
        out << "Database: " << db_name << std::endl;
        out << "Export time: " << LogFileManager::GetCurrentTimestamp() << std::endl;
        out << std::endl;

        // 导出操作日志
        out << "=== Operation Logs ===" << std::endl;
        auto operations = GetRecentOperations(1000); // 导出最近1000条
        for (const auto& op : operations) {
            out << op << std::endl;
        }

        out << std::endl;

        // 导出错误日志
        out << "=== Error Logs ===" << std::endl;
        auto errors = GetRecentErrors(1000);
        for (const auto& err : errors) {
            out << err << std::endl;
        }

        out.close();
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error exporting logs: " << e.what() << std::endl;
        return false;
    }
}

LogViewer::LogStats LogViewer::GetLogStatistics() {
    LogStats stats;

    try {
        auto operations = GetRecentOperations(10000);
        auto errors = GetRecentErrors(10000);

        stats.total_operations = operations.size();
        stats.total_errors = errors.size();
        stats.total_transactions = 0; // 简化实现
        stats.oldest_log_date = "2025-01-01";
        stats.newest_log_date = LogFileManager::GetCurrentTimestamp();

        uint64_t op_size = LogFileManager::GetFileSize(file_manager->GetOperationLogPath());
        uint64_t err_size = LogFileManager::GetFileSize(file_manager->GetErrorLogPath());
        uint64_t wal_size = LogFileManager::GetFileSize(file_manager->GetWALFilePath());

        stats.total_log_size_mb = (op_size + err_size + wal_size) / (1024 * 1024);

    }
    catch (const std::exception& e) {
        std::cerr << "Error getting log statistics: " << e.what() << std::endl;
    }

    return stats;
}

// 打印最近的操作日志
void LogViewer::PrintRecentOperations(int max_lines) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "           RECENT OPERATION LOGS (Last " << max_lines << " lines)" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    try {
        auto operations = GetRecentOperations(max_lines);

        if (operations.empty()) {
            std::cout << "  [INFO] No operation logs found." << std::endl;
            std::cout << "  Tip: Execute some database operations first to generate logs." << std::endl;
            std::cout << std::string(80, '=') << std::endl;
            return;
        }

        std::cout << " Line | Level | Content" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        int line_num = 1;
        for (const auto& op : operations) {
            // 格式化输出每一行
            std::cout << std::setw(5) << line_num << " | ";

            // 根据内容判断操作类型并添加标识
            if (op.find("INSERT") != std::string::npos) {
                std::cout << "ADD  | ";
            }
            else if (op.find("DELETE") != std::string::npos) {
                std::cout << "DEL  | ";
            }
            else if (op.find("UPDATE") != std::string::npos) {
                std::cout << "UPD  | ";
            }
            else if (op.find("SELECT") != std::string::npos) {
                std::cout << "SEL  | ";
            }
            else if (op.find("CREATE") != std::string::npos) {
                std::cout << "CRT  | ";
            }
            else {
                std::cout << "OTH  | ";
            }

            // 截断过长的日志行以便显示
            if (op.length() > 65) {
                std::cout << op.substr(0, 62) << "..." << std::endl;
            }
            else {
                std::cout << op << std::endl;
            }

            line_num++;
        }

        std::cout << std::string(80, '-') << std::endl;
        std::cout << "  Total operations displayed: " << operations.size() << " / " << max_lines << std::endl;
        std::cout << std::string(80, '=') << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "  [ERROR] Failed to read operation logs: " << e.what() << std::endl;
        std::cout << std::string(80, '=') << std::endl;
    }
}

// 打印最近的错误日志
void LogViewer::PrintRecentErrors(int max_lines) {
    std::cout << "\n" << std::string(80, '!') << std::endl;
    std::cout << "             RECENT ERROR LOGS (Last " << max_lines << " lines)" << std::endl;
    std::cout << std::string(80, '!') << std::endl;

    try {
        auto errors = GetRecentErrors(max_lines);

        if (errors.empty()) {
            std::cout << "  [INFO] No error logs found. System appears to be running smoothly!" << std::endl;
            std::cout << std::string(80, '!') << std::endl;
            return;
        }

        std::cout << " Line | Severity | Error Message" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        int line_num = 1;
        for (const auto& error : errors) {
            std::cout << std::setw(5) << line_num << " | ";

            // 根据错误内容判断严重程度
            if (error.find("CRITICAL") != std::string::npos ||
                error.find("FATAL") != std::string::npos) {
                std::cout << "CRIT   | ";
            }
            else if (error.find("ERROR") != std::string::npos) {
                std::cout << "ERROR  | ";
            }
            else if (error.find("WARN") != std::string::npos) {
                std::cout << "WARN   | ";
            }
            else {
                std::cout << "INFO   | ";
            }

            // 处理长错误消息
            if (error.length() > 62) {
                std::cout << error.substr(0, 59) << "..." << std::endl;
            }
            else {
                std::cout << error << std::endl;
            }

            line_num++;
        }

        std::cout << std::string(80, '-') << std::endl;
        std::cout << "  Total errors displayed: " << errors.size() << " / " << max_lines << std::endl;

        // 错误严重程度提示
        if (errors.size() > 0) {
            std::cout << "  [WARNING] Found " << errors.size() << " error entries. Please review!" << std::endl;
        }

        std::cout << std::string(80, '!') << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "  [ERROR] Failed to read error logs: " << e.what() << std::endl;
        std::cout << std::string(80, '!') << std::endl;
    }
}

// 打印所有类型的日志
void LogViewer::PrintAllLogs(int max_lines) {
    std::cout << "\n" << std::string(90, '#') << std::endl;
    std::cout << "                    COMPLETE LOG OVERVIEW (Last " << max_lines << " entries)" << std::endl;
    std::cout << std::string(90, '#') << std::endl;

    try {
        // 分配行数：2/3给操作日志，1/3给错误日志
        int ops_lines = (max_lines * 2) / 3;
        int err_lines = max_lines - ops_lines;

        // 获取日志统计信息
        auto stats = GetLogStatistics();

        std::cout << "\n[LOG SUMMARY]" << std::endl;
        std::cout << "  Database: " << db_name << std::endl;
        std::cout << "  Total Operations: " << stats.total_operations << std::endl;
        std::cout << "  Total Errors: " << stats.total_errors << std::endl;
        std::cout << "  Log File Size: " << stats.total_log_size_mb << " MB" << std::endl;
        std::cout << "  Date Range: " << stats.oldest_log_date << " ~ " << stats.newest_log_date << std::endl;

        // 打印操作日志部分
        std::cout << "\n[OPERATION LOGS SECTION - " << ops_lines << " lines]" << std::endl;
        PrintRecentOperations(ops_lines);

        // 打印错误日志部分  
        std::cout << "\n[ERROR LOGS SECTION - " << err_lines << " lines]" << std::endl;
        PrintRecentErrors(err_lines);

        // 总结信息
        std::cout << "\n[ANALYSIS SUMMARY]" << std::endl;
        if (stats.total_errors == 0) {
            std::cout << "  Status: HEALTHY - No errors detected" << std::endl;
        }
        else if (stats.total_errors < stats.total_operations / 10) {
            std::cout << "  Status: GOOD - Low error rate ("
                << (stats.total_operations > 0 ? (stats.total_errors * 100 / stats.total_operations) : 0)
                << "%)" << std::endl;
        }
        else {
            std::cout << "  Status: ATTENTION NEEDED - High error rate!" << std::endl;
        }

        std::cout << "  Recommendation: ";
        if (stats.total_operations == 0) {
            std::cout << "Generate some test operations to populate logs" << std::endl;
        }
        else if (stats.total_errors > stats.total_operations / 5) {
            std::cout << "Review error logs and fix underlying issues" << std::endl;
        }
        else {
            std::cout << "System appears to be functioning normally" << std::endl;
        }

        std::cout << std::string(90, '#') << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "  [ERROR] Failed to generate complete log overview: " << e.what() << std::endl;
        std::cout << std::string(90, '#') << std::endl;
    }
}

// 按类型打印日志
void LogViewer::PrintLogsByType(const std::string& log_type, int max_lines) {
    std::cout << "\n" << std::string(70, '*') << std::endl;
    std::cout << "           LOG VIEWER BY TYPE: '" << log_type << "'" << std::endl;
    std::cout << std::string(70, '*') << std::endl;

    try {
        // 转换为小写以便比较
        std::string type_lower = log_type;
        std::transform(type_lower.begin(), type_lower.end(), type_lower.begin(), ::tolower);

        if (type_lower == "operation" || type_lower == "ops" || type_lower == "op") {
            std::cout << "  Showing OPERATION logs..." << std::endl;
            PrintRecentOperations(max_lines);

        }
        else if (type_lower == "error" || type_lower == "err" || type_lower == "errors") {
            std::cout << "  Showing ERROR logs..." << std::endl;
            PrintRecentErrors(max_lines);

        }
        else if (type_lower == "all" || type_lower == "both" || type_lower == "complete") {
            std::cout << "  Showing ALL LOG TYPES..." << std::endl;
            PrintAllLogs(max_lines);

        }
        else {
            // 处理无效类型
            std::cout << "  [ERROR] Unknown log type: '" << log_type << "'" << std::endl;
            std::cout << "\n  Available log types:" << std::endl;
            std::cout << "    OPERATION types:" << std::endl;
            std::cout << "      - 'operation', 'ops', 'op'" << std::endl;
            std::cout << "    ERROR types:" << std::endl;
            std::cout << "      - 'error', 'err', 'errors'" << std::endl;
            std::cout << "    ALL types:" << std::endl;
            std::cout << "      - 'all', 'both', 'complete'" << std::endl;

            std::cout << "\n  Usage examples:" << std::endl;
            std::cout << "    viewer.PrintLogsByType(\"operation\", 20);" << std::endl;
            std::cout << "    viewer.PrintLogsByType(\"error\", 10);" << std::endl;
            std::cout << "    viewer.PrintLogsByType(\"all\", 50);" << std::endl;
        }

        std::cout << std::string(70, '*') << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "  [ERROR] Failed to print logs by type: " << e.what() << std::endl;
        std::cout << std::string(70, '*') << std::endl;
    }
}

// ========== EnhancedExecutor 实现 ==========
EnhancedExecutor::EnhancedExecutor(const std::string& db_name, const std::string& session, const std::string& user)
    : db_name(db_name), session_id(session), user_name(user) {
    logger = GlobalLoggerManager::GetLogger(db_name);
    if (!logger) {
        throw std::runtime_error("Failed to get logger for database: " + db_name);
    }
}

EnhancedExecutor::~EnhancedExecutor() {}

//bool EnhancedExecutor::ExecuteSQL(const std::string& sql, const std::vector<OperationLogRecord::SimpleQuadruple>& quads) {
//    try {
//        uint32_t txn_id = logger->BeginTransaction();
//
//        // logger->LogQuadrupleExecution(txn_id, sql, quads, session_id, user_name);
//
//        // 这里应该实际执行SQL，简化为总是成功
//        bool success = true;
//
//        if (success) {
//            // logger->CommitTransaction(txn_id);
//        }
//        else {
//            logger->AbortTransaction(txn_id);
//        }
//
//        return success;
//    }
//    catch (const std::exception& e) {
//        logger->LogError(e.what(), "ExecuteSQL");
//        return false;
//    }
//}

bool EnhancedExecutor::InsertRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length,uint32_t after_page_id,
    uint32_t after_slot_id,  uint32_t after_length, string sql, vector<Quadruple> qua, string user, bool result, uint64_t duration, string message) {
    try {
        uint32_t txn_id = logger->BeginTransaction();

        logger->LogDataChange(txn_id, WALLogRecord::INSERT_OP, before_page_id,
            before_slot_id, after_page_id, after_slot_id, before_length, after_length);

        bool success = true;

        if (success) {
            WALLogRecord record;
            record.transaction_id = txn_id;
            record.record_type = (int)WALLogRecord::INSERT_OP;
            record.withdraw = 0;
            WALLogRecord::DataChange datachange;
            datachange.before_length = 0;
            datachange.before_page_id = 0;
            datachange.before_slot_id = 0;
            datachange.after_length = after_length;
            datachange.after_page_id = after_page_id;
            datachange.after_slot_id = after_slot_id;
            record.changes.push_back(datachange);

            logger->LogQuadrupleExecution(txn_id, sql, qua, user, result, duration, message);
            logger->CommitTransaction(txn_id, record);
        }
        else {
            logger->AbortTransaction(txn_id);
        }

        return success;
    }
    catch (const std::exception& e) {
        logger->LogError(e.what(), "InsertRecord");
        return false;
    }
}

bool EnhancedExecutor::DeleteRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length, uint32_t after_page_id,
    uint32_t after_slot_id, uint32_t after_length, string sql, vector<Quadruple> qua, string user, bool result, uint64_t duration, string message) {
    try {
        uint32_t txn_id = logger->BeginTransaction();

        logger->LogDataChange(txn_id, WALLogRecord::DELETE_OP, before_page_id,
            before_slot_id, after_page_id, after_slot_id, before_length, after_length);

        bool success = true;

        if (success) {
            WALLogRecord record;
            record.transaction_id = txn_id;
            record.record_type = (int)WALLogRecord::DELETE_OP;
            record.withdraw = 0;
            WALLogRecord::DataChange datachange;
            datachange.before_length = before_length;
            datachange.before_page_id = before_page_id;
            datachange.before_slot_id = before_slot_id;
            datachange.after_length = 0;
            datachange.after_page_id = 0;
            datachange.after_slot_id = 0;
            record.changes.push_back(datachange);

            logger->LogQuadrupleExecution(txn_id, sql, qua, user, result, duration, message);
            logger->CommitTransaction(txn_id, record);
        }
        else {
            logger->AbortTransaction(txn_id);
        }

        return success;
    }
    catch (const std::exception& e) {
        logger->LogError(e.what(), "DeleteRecord");
        return false;
    }
}

bool EnhancedExecutor::UpdateRecord(uint32_t before_page_id, uint32_t before_slot_id, uint32_t before_length, uint32_t after_page_id,
    uint32_t after_slot_id, uint32_t after_length, string sql, vector<Quadruple> qua, string user, bool result, uint64_t duration, string message) {
    try {
        uint32_t txn_id = logger->BeginTransaction();

        logger->LogDataChange(txn_id, WALLogRecord::UPDATE_OP, before_page_id,
            before_slot_id, after_page_id, after_slot_id, before_length, after_length);

        bool success = true;

        if (success) {
            WALLogRecord record;
            record.transaction_id = txn_id;
            record.record_type = (int)WALLogRecord::UPDATE_OP;
            record.withdraw = 0;
            WALLogRecord::DataChange datachange;
            datachange.before_length = before_length;
            datachange.before_page_id = before_page_id;
            datachange.before_slot_id = before_slot_id;
            datachange.after_length = after_length;
            datachange.after_page_id = after_page_id;
            datachange.after_slot_id = after_slot_id;
            record.changes.push_back(datachange);

            logger->LogQuadrupleExecution(txn_id, sql, qua, user, result, duration, message);
            logger->CommitTransaction(txn_id, record);
        }
        else {
            logger->AbortTransaction(txn_id);
        }

        return success;
    }
    catch (const std::exception& e) {
        logger->LogError(e.what(), "UpdateRecord");
        return false;
    }
}

bool EnhancedExecutor::UndoLastDelete() {
    RecoveryManager* recovery = logger->GetRecoveryManager();
    uint64_t last_log_id = GetLastLogId();
    // return recovery->UndoDelete(last_log_id);
    return true;
}

bool EnhancedExecutor::UndoLastUpdate() {
    RecoveryManager* recovery = logger->GetRecoveryManager();
    uint64_t last_log_id = GetLastLogId();
    return recovery->UndoUpdate(last_log_id);
}

bool EnhancedExecutor::UndoLastInsert() {
    RecoveryManager* recovery = logger->GetRecoveryManager();
    uint64_t last_log_id = GetLastLogId();
    return recovery->UndoInsert(last_log_id);
}

bool EnhancedExecutor::UndoLastOperation(){
    // 获取最近一条操作
    RecoveryManager* recovery = logger->GetRecoveryManager();
    auto record = recovery->FindLatestWALRecord();

    cout << record.transaction_id << endl;
    cout << record.timestamp << endl;
    cout << record.record_type << endl;

    // 根据类型确定撤销操作
    switch (record.record_type)
    {
    case (int)WALLogRecord::INSERT_OP:
        recovery->UndoInsert(record.transaction_id);
        break;
    case (int)WALLogRecord::DELETE_OP:
        recovery->UndoDelete(record);
        break;
    case (int)WALLogRecord::UPDATE_OP:
        recovery->UndoUpdate(record.transaction_id);
        break;
    default:
        cout << "无法识别的操作" << endl;
        return false;
    }

    return true;
}

bool EnhancedExecutor::PerformCrashRecovery() {
    RecoveryManager* recovery = logger->GetRecoveryManager();
    return recovery->PerformCrashRecovery();
}

void EnhancedExecutor::ShowLoggerStatus() {
    std::cout << "\n=== Logger Status ===" << std::endl;
    const LogConfigInfo& config = logger->GetConfig();

    std::cout << "Database: " << db_name << std::endl;
    std::cout << "Log Level: " << config.log_level_name << " (" << (int)config.log_level_code << ")" << std::endl;
    std::cout << "WAL Enabled: " << (config.enable_wal ? "Yes" : "No") << std::endl;
    std::cout << "WAL Buffer Size: " << config.wal_buffer_size_mb << " MB" << std::endl;
    std::cout << "Sync Mode: " << config.sync_mode_name << " (" << (int)config.sync_mode_code << ")" << std::endl;
    std::cout << "Log Rotation: " << (config.enable_log_rotation ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Max Log File Size: " << config.log_file_size_mb << " MB" << std::endl;
    std::cout << "Max Log Files: " << config.log_file_count << std::endl;
}

std::vector<char> EnhancedExecutor::SerializeRowData(const std::map<std::string, std::string>& row_data) {
    std::stringstream ss;
    bool first = true;
    for (const auto& pair : row_data) {
        if (!first) ss << ",";
        ss << pair.first << ":" << pair.second;
        first = false;
    }

    std::string str = ss.str();
    return std::vector<char>(str.begin(), str.end());
}

uint64_t EnhancedExecutor::GetLastLogId() {
    return 1; // 简化实现
}

// ========== LoggerUtils 实现 ==========
namespace LoggerUtils {
    std::string FormatTimestamp(uint64_t timestamp) {
        auto time = std::chrono::system_clock::from_time_t(timestamp / 1000);
        auto time_t = std::chrono::system_clock::to_time_t(time);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    uint8_t ParseLogLevel(const std::string& level_name) {
        std::string level = level_name;
        std::transform(level.begin(), level.end(), level.begin(), ::toupper);

        if (level == "ERROR") return 0;
        if (level == "WARN" || level == "WARNING") return 1;
        if (level == "INFO") return 2;
        if (level == "DEBUG") return 3;

        return 2; // 默认INFO
    }

    std::string LogLevelToString(uint8_t level_code) {
        switch (level_code) {
        case 0: return "ERROR";
        case 1: return "WARN";
        case 2: return "INFO";
        case 3: return "DEBUG";
        default: return "INFO";
        }
    }

    uint8_t ParseSyncMode(const std::string& mode_name) {
        if (mode_name == "异步" || mode_name == "async") return 0;
        if (mode_name == "同步" || mode_name == "sync") return 1;
        if (mode_name == "全同步" || mode_name == "full_sync") return 2;

        return 1; // 默认同步
    }

    std::string SyncModeToString(uint8_t mode_code) {
        switch (mode_code) {
        case 0: return "异步";
        case 1: return "同步";
        case 2: return "全同步";
        default: return "同步";
        }
    }

    std::string FormatFileSize(uint64_t size_bytes) {
        const char* units[] = { "B", "KB", "MB", "GB", "TB" };
        int unit_index = 0;
        double size = static_cast<double>(size_bytes);

        while (size >= 1024.0 && unit_index < 4) {
            size /= 1024.0;
            unit_index++;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
        return ss.str();
    }

    bool ValidateLogFileIntegrity(const std::string& log_file_path) {
        std::ifstream file(log_file_path);
        return file.good();
    }

    bool MergeLogFiles(const std::vector<std::string>& input_files,
        const std::string& output_file) {
        std::ofstream out(output_file);
        if (!out.is_open()) return false;

        for (const auto& input_file : input_files) {
            std::ifstream in(input_file);
            if (in.is_open()) {
                out << in.rdbuf();
                in.close();
            }
        }

        out.close();
        return true;
    }

    bool RebuildOperationLogFromWAL(const std::string& wal_file,
        const std::string& output_file) {
        std::cout << "Rebuilding operation log from WAL: " << wal_file
            << " to " << output_file << std::endl;
        return true;
    }
}

// ========== 转换函数实现 ==========
//std::vector<OperationLogRecord::SimpleQuadruple> ConvertQuadruples(const std::vector<Quadruple>& quads) {
//    std::vector<OperationLogRecord::SimpleQuadruple> simple_quads;
//
//    // 这里需要实际的转换逻辑，暂时返回空
//    // 实际实现需要根据Quadruple的具体定义来转换
//
//    return simple_quads;
//}
//
//std::vector<Quadruple> ConvertSimpleQuadruples(const std::vector<OperationLogRecord::SimpleQuadruple>& simple_quads) {
//    std::vector<Quadruple> quads;
//
//    // 这里需要实际的转换逻辑，暂时返回空
//    // 实际实现需要根据Quadruple的具体定义来转换
//
//    return quads;
//}
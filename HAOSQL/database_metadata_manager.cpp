#define _CRT_SECURE_NO_WARNINGS
#include "database_metadata_manager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

DatabaseMetadataManager::DatabaseMetadataManager(const std::string& file_path)
    : metadata_file_path(file_path) {
}

//==================== 文件操作 ====================
bool DatabaseMetadataManager::LoadMetadata() 
{
    std::ifstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "元数据文件不存在: " << metadata_file_path << std::endl;
        return false;
    }

    // 读取数据库头部
    file.read(reinterpret_cast<char*>(&db_header), sizeof(DatabaseHeader));

    // 验证文件格式
    if (strncmp(db_header.magic, "HAODB001", 8) != 0) 
    {
        std::cerr << "无效的元数据文件格式" << std::endl;
        return false;
    }

    // 读取存储配置
    file.read(reinterpret_cast<char*>(&storage_config), sizeof(StorageConfig));

    // 读取日志配置
    file.read(reinterpret_cast<char*>(&log_config), sizeof(LogConfig));

    // 读取统计信息
    file.read(reinterpret_cast<char*>(&db_stats), sizeof(DatabaseStats));

    // 读取表元数据
    tables.clear();
    tables.resize(db_header.table_count);
    for (uint64_t i = 0; i < db_header.table_count; i++) 
    {
        file.read(reinterpret_cast<char*>(&tables[i]), sizeof(TableMeta));
    }

    // 读取索引元数据
    indexes.clear();
    indexes.resize(db_header.index_count);
    for (uint64_t i = 0; i < db_header.index_count; i++) 
    {
        file.read(reinterpret_cast<char*>(&indexes[i]), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "数据库元数据加载成功: " << db_header.db_name << std::endl;
    std::cout << "表数量: " << db_header.table_count
        << ", 索引数量: " << db_header.index_count << std::endl;
    return true;
}

bool DatabaseMetadataManager::SaveMetadata() 
{
    std::ofstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "无法写入元数据文件: " << metadata_file_path << std::endl;
        return false;
    }

    // 更新头部信息
    db_header.table_count = tables.size();
    db_header.index_count = indexes.size();
    db_header.last_modify = static_cast<uint64_t>(time(nullptr));

    // 写入数据库头部
    file.write(reinterpret_cast<const char*>(&db_header), sizeof(DatabaseHeader));

    // 写入存储配置
    file.write(reinterpret_cast<const char*>(&storage_config), sizeof(StorageConfig));

    // 写入日志配置
    file.write(reinterpret_cast<const char*>(&log_config), sizeof(LogConfig));

    // 写入统计信息
    file.write(reinterpret_cast<const char*>(&db_stats), sizeof(DatabaseStats));

    // 写入表元数据
    for (const auto& table : tables) 
    {
        file.write(reinterpret_cast<const char*>(&table), sizeof(TableMeta));
    }

    // 写入索引元数据
    for (const auto& index : indexes)
    {
        file.write(reinterpret_cast<const char*>(&index), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "数据库元数据保存成功" << std::endl;
    return true;
}

bool DatabaseMetadataManager::CreateNewDatabase(const std::string& db_name, uint32_t db_id) {
    db_header = DatabaseHeader();
    storage_config = StorageConfig();
    log_config = LogConfig();
    db_stats = DatabaseStats();

    strcpy(db_header.db_name, db_name.c_str());
    db_header.db_id = db_id;
    db_header.create_time = static_cast<uint64_t>(time(nullptr));
    db_header.last_modify = db_header.create_time;

    // 设置默认路径
    std::string base_path = "HAODB\\" + db_name + "\\";
    strcpy(storage_config.data_file_path, (base_path + "data\\" + db_name + ".dat").c_str());
    strcpy(storage_config.log_file_path, (base_path + "logs\\" + db_name + ".log").c_str());
    strcpy(storage_config.index_file_path, (base_path + "index\\" + db_name + ".idx").c_str());

    tables.clear();
    indexes.clear();

    std::cout << "新数据库 " << db_name << " 创建成功" << std::endl;
    return SaveMetadata();
}

//==================== 表管理 ====================
bool DatabaseMetadataManager::CreateTable(const std::string& table_name,
    uint32_t table_id,
    const std::vector<ColumnMeta>& columns,
    uint64_t data_offset)
{
    // 检查表名是否已存在
    for (const auto& table : tables)
    {
        if (strcmp(table.table_name, table_name.c_str()) == 0) 
        {
            std::cerr << "表 " << table_name << " 已存在" << std::endl;
            return false;
        }
    }

    // 创建新表
    TableMeta new_table;
    strcpy(new_table.table_name, table_name.c_str());
    new_table.table_id = table_id;
    new_table.row_count = 0;
    new_table.column_count = columns.size();
    new_table.data_file_offset = data_offset;
    new_table.create_time = static_cast<uint64_t>(time(nullptr));
    new_table.table_type = 0;

    // 复制列定义
    for (size_t i = 0; i < columns.size() && i < 32; i++) 
    {
        new_table.columns[i] = columns[i];
    }

    tables.push_back(new_table);
    std::cout << "表 " << table_name << " 创建成功" << std::endl;
    return true;
}

bool DatabaseMetadataManager::DropTable(const std::string& table_name) 
{
    auto it = std::find_if(tables.begin(), tables.end(),
        [&](const TableMeta& table)
        {
            return strcmp(table.table_name, table_name.c_str()) == 0;
        });

    if (it != tables.end()) 
    {
        uint32_t table_id = it->table_id;
        tables.erase(it);

        // 删除相关索引
        indexes.erase(std::remove_if(indexes.begin(), indexes.end(),
            [table_id](const IndexMeta& index)
            {
                return index.table_id == table_id;
            }), indexes.end());

        std::cout << "表 " << table_name << " 删除成功" << std::endl;
        return true;
    }

    std::cerr << "未找到表: " << table_name << std::endl;
    return false;
}

bool DatabaseMetadataManager::UpdateTableRowCount(const std::string& table_name, uint64_t row_count) 
{
    TableMeta* table = GetTableInfo(table_name);
    if (table) 
    {
        table->row_count = row_count;
        std::cout << "表 " << table_name << " 行数更新为: " << row_count << std::endl;
        return true;
    }
    return false;
}

TableMeta* DatabaseMetadataManager::GetTableInfo(const std::string& table_name) 
{
    for (auto& table : tables) {
        if (strcmp(table.table_name, table_name.c_str()) == 0)
        {
            return &table;
        }
    }
    return nullptr;
}

TableMeta* DatabaseMetadataManager::GetTableInfo(uint32_t table_id)
{
    for (auto& table : tables) 
    {
        if (table.table_id == table_id)
        {
            return &table;
        }
    }
    return nullptr;
}

//==================== 索引管理 ====================
bool DatabaseMetadataManager::CreateIndex(const std::string& index_name,
    uint32_t index_id,
    uint32_t table_id,
    const std::vector<uint32_t>& column_ids,
    bool is_unique,
    uint64_t root_page_id) 
{
    // 检查索引名是否已存在
    for (const auto& index : indexes) 
    {
        if (strcmp(index.index_name, index_name.c_str()) == 0) 
        {
            std::cerr << "索引 " << index_name << " 已存在" << std::endl;
            return false;
        }
    }

    // 创建新索引
    IndexMeta new_index;
    strcpy(new_index.index_name, index_name.c_str());
    new_index.index_id = index_id;
    new_index.table_id = table_id;
    new_index.index_type = 0;  // B+树
    new_index.is_unique = is_unique ? 1 : 0;
    new_index.column_count = column_ids.size();
    new_index.root_page_id = root_page_id;

    // 复制列ID
    for (size_t i = 0; i < column_ids.size() && i < 8; i++)
    {
        new_index.columns[i] = column_ids[i];
    }

    indexes.push_back(new_index);
    std::cout << "索引 " << index_name << " 创建成功" << std::endl;
    return true;
}

bool DatabaseMetadataManager::DropIndex(const std::string& index_name)
{
    auto it = std::find_if(indexes.begin(), indexes.end(),
        [&](const IndexMeta& index)
        {
            return strcmp(index.index_name, index_name.c_str()) == 0;
        });

    if (it != indexes.end())
    {
        indexes.erase(it);
        std::cout << "索引 " << index_name << " 删除成功" << std::endl;
        return true;
    }

    std::cerr << "未找到索引: " << index_name << std::endl;
    return false;
}

IndexMeta* DatabaseMetadataManager::GetIndexInfo(const std::string& index_name)
{
    for (auto& index : indexes) 
    {
        if (strcmp(index.index_name, index_name.c_str()) == 0)
        {
            return &index;
        }
    }
    return nullptr;
}

std::vector<IndexMeta*> DatabaseMetadataManager::GetTableIndexes(uint32_t table_id)
{
    std::vector<IndexMeta*> table_indexes;
    for (auto& index : indexes) 
    {
        if (index.table_id == table_id) 
        {
            table_indexes.push_back(&index);
        }
    }
    return table_indexes;
}

//==================== 列管理 ====================
bool DatabaseMetadataManager::AddColumn(const std::string& table_name, const ColumnMeta& column)
{
    TableMeta* table = GetTableInfo(table_name);
    if (!table) 
    {
        std::cerr << "未找到表: " << table_name << std::endl;
        return false;
    }

    if (table->column_count >= 32) 
    {
        std::cerr << "表 " << table_name << " 列数已达上限" << std::endl;
        return false;
    }

    table->columns[table->column_count] = column;
    table->column_count++;

    std::cout << "列 " << column.column_name << " 添加到表 " << table_name << " 成功" << std::endl;
    return true;
}

bool DatabaseMetadataManager::DropColumn(const std::string& table_name, const std::string& column_name)
{
    TableMeta* table = GetTableInfo(table_name);
    if (!table)
    {
        return false;
    }

    for (uint32_t i = 0; i < table->column_count; i++)
    {
        if (strcmp(table->columns[i].column_name, column_name.c_str()) == 0) 
        {
            // 将后面的列前移
            for (uint32_t j = i; j < table->column_count - 1; j++) 
            {
                table->columns[j] = table->columns[j + 1];
            }
            table->column_count--;

            std::cout << "列 " << column_name << " 从表 " << table_name << " 删除成功" << std::endl;
            return true;
        }
    }

    std::cerr << "未找到列: " << column_name << std::endl;
    return false;
}

ColumnMeta* DatabaseMetadataManager::GetColumnInfo(const std::string& table_name, const std::string& column_name) 
{
    TableMeta* table = GetTableInfo(table_name);
    if (!table)
    {
        return nullptr;
    }

    for (uint32_t i = 0; i < table->column_count; i++) 
    {
        if (strcmp(table->columns[i].column_name, column_name.c_str()) == 0) 
        {
            return &table->columns[i];
        }
    }

    return nullptr;
}

//==================== 显示信息 ====================
void DatabaseMetadataManager::ShowDatabaseStructure()
{
    std::cout << "\n=== 数据库结构: " << db_header.db_name << " ===" << std::endl;
    std::cout << "数据库ID: " << db_header.db_id << std::endl;
    std::cout << "页面大小: " << db_header.page_size << " bytes" << std::endl;
    std::cout << "表数量: " << tables.size() << ", 索引数量: " << indexes.size() << std::endl;

    std::cout << "\n--- 表信息 ---" << std::endl;
    for (const auto& table : tables) 
    {
        std::cout << "表名: " << table.table_name
            << ", ID: " << table.table_id
            << ", 行数: " << table.row_count
            << ", 列数: " << table.column_count << std::endl;

        // 显示列信息
        for (uint32_t i = 0; i < table.column_count; i++)
        {
            const auto& col = table.columns[i];
            std::cout << "  列: " << col.column_name
                << " (类型:" << (int)col.data_type
                << ", 长度:" << col.max_length;
            if (col.is_primary_key) std::cout << ", 主键";
            if (col.is_unique) std::cout << ", 唯一";
            if (!col.is_nullable) std::cout << ", 非空";
            std::cout << ")" << std::endl;
        }
    }

    std::cout << "\n--- 索引信息 ---" << std::endl;
    for (const auto& index : indexes) 
    {
        std::cout << "索引名: " << index.index_name
            << ", 表ID: " << index.table_id
            << ", " << (index.is_unique ? "唯一索引" : "普通索引")
            << ", 根页面: " << index.root_page_id << std::endl;
    }
}

void DatabaseMetadataManager::ShowTableStructure(const std::string& table_name)
{
    TableMeta* table = GetTableInfo(table_name);
    if (!table)
    {
        std::cerr << "未找到表: " << table_name << std::endl;
        return;
    }

    std::cout << "\n=== 表结构: " << table_name << " ===" << std::endl;
    std::cout << "表ID: " << table->table_id << ", 行数: " << table->row_count << std::endl;
    std::cout << "数据偏移量: " << table->data_file_offset << std::endl;

    for (uint32_t i = 0; i < table->column_count; i++)
    {
        const auto& col = table->columns[i];
        std::cout << col.column_name << " "
            << (col.data_type == 1 ? "INT" :
                col.data_type == 2 ? "VARCHAR" :
                col.data_type == 3 ? "DECIMAL" : "DATETIME")
            << "(" << col.max_length << ")";
        if (col.is_primary_key) std::cout << " PRIMARY KEY";
        if (col.is_unique) std::cout << " UNIQUE";
        if (!col.is_nullable) std::cout << " NOT NULL";
        std::cout << std::endl;
    }
}

void DatabaseMetadataManager::ShowIndexInfo()
{
    std::cout << "\n=== 索引信息 ===" << std::endl;
    for (const auto& index : indexes)
    {
        TableMeta* table = GetTableInfo(index.table_id);
        std::cout << "索引: " << index.index_name
            << " ON " << (table ? table->table_name : "UNKNOWN")
            << " (";
        for (uint32_t i = 0; i < index.column_count; i++)
        {
            if (i > 0) std::cout << ", ";
            // 找到对应的列名
            if (table) 
            {
                for (uint32_t j = 0; j < table->column_count; j++)
                {
                    if (table->columns[j].column_id == index.columns[i]) 
                    {
                        std::cout << table->columns[j].column_name;
                        break;
                    }
                }
            }
        }
        std::cout << ")" << std::endl;
    }
}

void DatabaseMetadataManager::ShowCompleteInfo() 
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "          " << db_header.db_name << " 数据库完整信息" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    ShowDatabaseStructure();
    ShowStorageConfig();
    ShowLogConfig();
    ShowDatabaseStats();

    std::cout << std::string(60, '=') << std::endl;
}

//==================== 数据库属性修改 ====================
bool DatabaseMetadataManager::SetDatabaseName(const std::string& new_name)
{
    strcpy(db_header.db_name, new_name.c_str());
    std::cout << "数据库名称更新为: " << new_name << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetPageSize(uint32_t page_size) 
{
    db_header.page_size = page_size;
    std::cout << "页面大小设置为: " << page_size << " bytes" << std::endl;
    return true;
}

//==================== 统计信息管理 ====================
bool DatabaseMetadataManager::UpdateTotalPages(uint64_t total, uint64_t free) 
{
    db_stats.total_pages = total;
    db_stats.free_pages = free;
    std::cout << "页面统计更新: 总页面=" << total << ", 空闲页面=" << free << std::endl;
    return true;
}

bool DatabaseMetadataManager::UpdateDatabaseSize(uint64_t size) 
{
    db_stats.total_size = size;
    std::cout << "数据库大小更新为: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLastBackupTime(uint64_t timestamp) 
{
    db_stats.last_backup_time = timestamp;
    std::cout << "备份时间更新" << std::endl;
    return true;
}

bool DatabaseMetadataManager::IncrementTransactionCount() 
{
    db_stats.transaction_count++;
    return true;
}

bool DatabaseMetadataManager::UpdateIOStats(uint64_t reads, uint64_t writes) 
{
    db_stats.total_reads += reads;
    db_stats.total_writes += writes;
    return true;
}

bool DatabaseMetadataManager::UpdateCacheStats(uint64_t hits, uint64_t misses) 
{
    db_stats.cache_hits += hits;
    db_stats.cache_misses += misses;
    return true;
}

bool DatabaseMetadataManager::SetActiveConnections(uint32_t count)
{
    db_stats.active_connections = count;
    return true;
}

void DatabaseMetadataManager::ShowDatabaseStats() 
{
    std::cout << "\n=== 数据库统计信息 ===" << std::endl;
    std::cout << "总页面数: " << db_stats.total_pages << std::endl;
    std::cout << "空闲页面数: " << db_stats.free_pages << std::endl;
    std::cout << "数据库大小: " << db_stats.total_size << " bytes" << std::endl;
    std::cout << "事务总数: " << db_stats.transaction_count << std::endl;
    std::cout << "总读取次数: " << db_stats.total_reads << std::endl;
    std::cout << "总写入次数: " << db_stats.total_writes << std::endl;
    std::cout << "缓存命中: " << db_stats.cache_hits << std::endl;
    std::cout << "缓存未命中: " << db_stats.cache_misses << std::endl;
    std::cout << "当前连接数: " << db_stats.active_connections << std::endl;

    // 计算统计比率
    if (db_stats.cache_hits + db_stats.cache_misses > 0) {
        double hit_ratio = (double)db_stats.cache_hits / (db_stats.cache_hits + db_stats.cache_misses) * 100;
        std::cout << "缓存命中率: " << std::fixed << std::setprecision(2) << hit_ratio << "%" << std::endl;
    }

    if (db_stats.total_pages > 0) {
        double utilization = (double)(db_stats.total_pages - db_stats.free_pages) / db_stats.total_pages * 100;
        std::cout << "存储利用率: " << std::fixed << std::setprecision(2) << utilization << "%" << std::endl;
    }
}

void DatabaseMetadataManager::ResetStatistics() 
{
    db_stats.total_reads = 0;
    db_stats.total_writes = 0;
    db_stats.cache_hits = 0;
    db_stats.cache_misses = 0;
    db_stats.transaction_count = 0;
    std::cout << "统计信息已重置" << std::endl;
}

uint64_t DatabaseMetadataManager::GetTotalRowCount() const 
{
    uint64_t total_rows = 0;
    for (const auto& table : tables)
    {
        total_rows += table.row_count;
    }
    return total_rows;
}

//==================== 存储配置管理 ====================
bool DatabaseMetadataManager::SetDataFilePath(const std::string& path) 
{
    strcpy(storage_config.data_file_path, path.c_str());
    std::cout << "数据文件路径设置为: " << path << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLogFilePath(const std::string& path) 
{
    strcpy(storage_config.log_file_path, path.c_str());
    std::cout << "日志文件路径设置为: " << path << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetIndexFilePath(const std::string& path) 
{
    strcpy(storage_config.index_file_path, path.c_str());
    std::cout << "索引文件路径设置为: " << path << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetBufferPoolSize(uint32_t size) 
{
    storage_config.buffer_pool_size = size;
    std::cout << "缓冲池大小设置为: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetMaxConnections(uint32_t max_conn) 
{
    storage_config.max_connections = max_conn;
    std::cout << "最大连接数设置为: " << max_conn << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLockTimeout(uint32_t timeout) 
{
    storage_config.lock_timeout = timeout;
    std::cout << "锁超时时间设置为: " << timeout << " 秒" << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableAutoCheckpoint(bool enable) 
{
    storage_config.auto_checkpoint = enable ? 1 : 0;
    std::cout << "自动检查点" << (enable ? "启用" : "禁用") << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetPageCacheSize(uint32_t size) 
{
    storage_config.page_cache_size = size;
    std::cout << "页面缓存大小设置为: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableCompression(bool enable) 
{
    storage_config.enable_compression = enable ? 1 : 0;
    std::cout << "数据压缩" << (enable ? "启用" : "禁用") << std::endl;
    return true;
}

void DatabaseMetadataManager::ShowStorageConfig() 
{
    std::cout << "\n=== 存储配置 ===" << std::endl;
    std::cout << "数据文件路径: " << storage_config.data_file_path << std::endl;
    std::cout << "日志文件路径: " << storage_config.log_file_path << std::endl;
    std::cout << "索引文件路径: " << storage_config.index_file_path << std::endl;
    std::cout << "缓冲池大小: " << storage_config.buffer_pool_size << " bytes" << std::endl;
    std::cout << "最大连接数: " << storage_config.max_connections << std::endl;
    std::cout << "锁超时时间: " << storage_config.lock_timeout << " 秒" << std::endl;
    std::cout << "自动检查点: " << (storage_config.auto_checkpoint ? "启用" : "禁用") << std::endl;
    std::cout << "页面缓存大小: " << storage_config.page_cache_size << " bytes" << std::endl;
    std::cout << "数据压缩: " << (storage_config.enable_compression ? "启用" : "禁用") << std::endl;
}

//==================== 日志配置管理 ====================
bool DatabaseMetadataManager::SetLogLevel(uint8_t level) 
{
    if (level > 3)
    {
        std::cerr << "无效的日志级别: " << (int)level << std::endl;
        return false;
    }
    log_config.log_level = level;
    const char* level_names[] = { "ERROR", "WARN", "INFO", "DEBUG" };
    std::cout << "日志级别设置为: " << level_names[level] << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLogFileSize(uint32_t size_mb) 
{
    log_config.log_file_size = size_mb;
    std::cout << "日志文件大小设置为: " << size_mb << " MB" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLogFileCount(uint32_t count)
{
    log_config.log_file_count = count;
    std::cout << "保留日志文件数量设置为: " << count << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableWAL(bool enable) 
{
    log_config.enable_wal = enable ? 1 : 0;
    std::cout << "WAL日志" << (enable ? "启用" : "禁用") << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetWALBufferSize(uint32_t size)
{
    log_config.wal_buffer_size = size;
    std::cout << "WAL缓冲区大小设置为: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetSyncMode(uint8_t mode)
{
    if (mode > 2) 
    {
        std::cerr << "无效的同步模式: " << (int)mode << std::endl;
        return false;
    }
    log_config.sync_mode = mode;
    const char* mode_names[] = { "异步", "同步", "全同步" };
    std::cout << "同步模式设置为: " << mode_names[mode] << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableLogRotation(bool enable)
{
    log_config.enable_log_rotation = enable ? 1 : 0;
    std::cout << "日志轮转" << (enable ? "启用" : "禁用") << std::endl;
    return true;
}

void DatabaseMetadataManager::ShowLogConfig() 
{
    std::cout << "\n=== 日志配置 ===" << std::endl;
    const char* level_names[] = { "ERROR", "WARN", "INFO", "DEBUG" };
    std::cout << "日志级别: " << level_names[log_config.log_level] << std::endl;
    std::cout << "日志文件大小: " << log_config.log_file_size << " MB" << std::endl;
    std::cout << "保留文件数量: " << log_config.log_file_count << std::endl;
    std::cout << "WAL日志: " << (log_config.enable_wal ? "启用" : "禁用") << std::endl;
    std::cout << "WAL缓冲区: " << log_config.wal_buffer_size << " bytes" << std::endl;
    const char* sync_names[] = { "异步", "同步", "全同步" };
    std::cout << "同步模式: " << sync_names[log_config.sync_mode] << std::endl;
    std::cout << "日志轮转: " << (log_config.enable_log_rotation ? "启用" : "禁用") << std::endl;
}

//==================== 性能分析 ====================
double DatabaseMetadataManager::GetCacheHitRatio() const 
{
    uint64_t total_access = db_stats.cache_hits + db_stats.cache_misses;
    if (total_access == 0) return 0.0;
    return (double)db_stats.cache_hits / total_access * 100.0;
}

uint64_t DatabaseMetadataManager::GetAverageTransactionSize() const 
{
    if (db_stats.transaction_count == 0) return 0;
    return (db_stats.total_reads + db_stats.total_writes) / db_stats.transaction_count;
}

void DatabaseMetadataManager::GeneratePerformanceReport() 
{
    std::cout << "\n=== 性能分析报告 ===" << std::endl;
    std::cout << "数据库: " << db_header.db_name << std::endl;
    std::cout << "分析时间: " << time(nullptr) << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    // 缓存性能
    std::cout << "缓存性能:" << std::endl;
    std::cout << "  命中率: " << std::fixed << std::setprecision(2)
        << GetCacheHitRatio() << "%" << std::endl;
    std::cout << "  总访问: " << (db_stats.cache_hits + db_stats.cache_misses) << std::endl;

    // I/O性能
    std::cout << "I/O性能:" << std::endl;
    std::cout << "  读取次数: " << db_stats.total_reads << std::endl;
    std::cout << "  写入次数: " << db_stats.total_writes << std::endl;
    std::cout << "  读写比例: " << std::fixed << std::setprecision(2);
    if (db_stats.total_writes > 0) 
    {
        std::cout << (double)db_stats.total_reads / db_stats.total_writes << ":1" << std::endl;
    }
    else {
        std::cout << "N/A" << std::endl;
    }

    // 存储性能
    std::cout << "存储性能:" << std::endl;
    if (db_stats.total_pages > 0)
    {
        double utilization = (double)(db_stats.total_pages - db_stats.free_pages) / db_stats.total_pages * 100;
        std::cout << "  利用率: " << std::fixed << std::setprecision(2) << utilization << "%" << std::endl;
    }
    std::cout << "  平均事务大小: " << GetAverageTransactionSize() << " I/O" << std::endl;

    // 连接性能
    std::cout << "连接性能:" << std::endl;
    std::cout << "  当前连接数: " << db_stats.active_connections << std::endl;
    std::cout << "  最大连接数: " << storage_config.max_connections << std::endl;
    if (storage_config.max_connections > 0) 
    {
        double conn_utilization = (double)db_stats.active_connections / storage_config.max_connections * 100;
        std::cout << "  连接利用率: " << std::fixed << std::setprecision(2) << conn_utilization << "%" << std::endl;
    }
}

//==================== 配置管理辅助类 ====================
// 存储配置管理器实现
bool StorageConfigManager::SetDataPath(const std::string& path) 
{
    strcpy(config.data_file_path, path.c_str());
    return true;
}

bool StorageConfigManager::SetLogPath(const std::string& path)
{
    strcpy(config.log_file_path, path.c_str());
    return true;
}

bool StorageConfigManager::SetIndexPath(const std::string& path)
{
    strcpy(config.index_file_path, path.c_str());
    return true;
}

bool StorageConfigManager::SetBufferPoolSize(uint32_t size_mb) 
{
    config.buffer_pool_size = size_mb * 1024 * 1024;  // 转换为字节
    return true;
}

bool StorageConfigManager::SetPageCacheSize(uint32_t size_mb)
{
    config.page_cache_size = size_mb * 1024 * 1024;  // 转换为字节
    return true;
}

bool StorageConfigManager::SetMaxConnections(uint32_t max_conn) 
{
    if (max_conn > 1000) {  // 限制最大连接数
        std::cerr << "警告: 最大连接数过大，建议不超过1000" << std::endl;
    }
    config.max_connections = max_conn;
    return true;
}

bool StorageConfigManager::SetLockTimeout(uint32_t timeout_sec) 
{
    config.lock_timeout = timeout_sec;
    return true;
}

bool StorageConfigManager::EnableAutoCheckpoint(bool enable)
{
    config.auto_checkpoint = enable ? 1 : 0;
    return true;
}

bool StorageConfigManager::EnableCompression(bool enable)
{
    config.enable_compression = enable ? 1 : 0;
    return true;
}

bool StorageConfigManager::ValidateConfig() 
{
    // 检查路径是否为空
    if (strlen(config.data_file_path) == 0) 
    {
        std::cerr << "错误: 数据文件路径不能为空" << std::endl;
        return false;
    }

    // 检查缓冲池大小是否合理
    if (config.buffer_pool_size < 1024 * 1024)
    {  // 小于1MB
        std::cerr << "警告: 缓冲池大小过小，建议至少1MB" << std::endl;
    }

    // 检查连接数是否合理
    if (config.max_connections == 0) 
    {
        std::cerr << "错误: 最大连接数不能为0" << std::endl;
        return false;
    }

    return true;
}

void StorageConfigManager::PrintConfig() 
{
    std::cout << "=== 存储配置 ===" << std::endl;
    std::cout << "数据文件: " << config.data_file_path << std::endl;
    std::cout << "日志文件: " << config.log_file_path << std::endl;
    std::cout << "索引文件: " << config.index_file_path << std::endl;
    std::cout << "缓冲池: " << (config.buffer_pool_size / 1024 / 1024) << " MB" << std::endl;
    std::cout << "页面缓存: " << (config.page_cache_size / 1024 / 1024) << " MB" << std::endl;
    std::cout << "最大连接: " << config.max_connections << std::endl;
    std::cout << "锁超时: " << config.lock_timeout << " 秒" << std::endl;
    std::cout << "自动检查点: " << (config.auto_checkpoint ? "启用" : "禁用") << std::endl;
    std::cout << "数据压缩: " << (config.enable_compression ? "启用" : "禁用") << std::endl;
}

// 日志配置管理器实现
bool LogConfigManager::SetLogLevel(const std::string& level) 
{
    if (level == "ERROR") { config.log_level = 0; }
    else if (level == "WARN") { config.log_level = 1; }
    else if (level == "INFO") { config.log_level = 2; }
    else if (level == "DEBUG") { config.log_level = 3; }
    else 
    {
        std::cerr << "无效的日志级别: " << level << std::endl;
        return false;
    }
    return true;
}

bool LogConfigManager::SetSyncMode(const std::string& mode)
{
    if (mode == "async") { config.sync_mode = 0; }
    else if (mode == "sync") { config.sync_mode = 1; }
    else if (mode == "full_sync") { config.sync_mode = 2; }
    else 
    {
        std::cerr << "无效的同步模式: " << mode << std::endl;
        return false;
    }
    return true;
}

std::string LogConfigManager::GetLogLevelString() const 
{
    const char* levels[] = { "ERROR", "WARN", "INFO", "DEBUG" };
    return levels[config.log_level];
}

std::string LogConfigManager::GetSyncModeString() const
{
    const char* modes[] = { "async", "sync", "full_sync" };
    return modes[config.sync_mode];
}

// 统计信息管理器实现
bool DatabaseStatsManager::UpdateStorageStats(uint64_t total_pages, uint64_t free_pages, uint64_t total_size)
{
    stats.total_pages = total_pages;
    stats.free_pages = free_pages;
    stats.total_size = total_size;
    return true;
}

double DatabaseStatsManager::GetCacheHitRatio() const
{
    uint64_t total = stats.cache_hits + stats.cache_misses;
    return total > 0 ? (double)stats.cache_hits / total * 100.0 : 0.0;
}

double DatabaseStatsManager::GetStorageUtilization() const 
{
    return stats.total_pages > 0 ?
        (double)(stats.total_pages - stats.free_pages) / stats.total_pages * 100.0 : 0.0;
}

void DatabaseStatsManager::ResetAllStats() 
{
    memset(&stats, 0, sizeof(DatabaseStats));
    std::cout << "所有统计信息已重置" << std::endl;
}
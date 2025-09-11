#include "metadata_structures.h"
#include <cstring>

// DatabaseEntry构造函数
DatabaseEntry::DatabaseEntry() 
{
    memset(this, 0, sizeof(DatabaseEntry));
}

// DatabaseRegistry构造函数
DatabaseRegistry::DatabaseRegistry() 
{
    strcpy(magic, "HAODBSYS");
    version = 10001;
    database_count = 0;
    last_update = 0;
}

// SystemConfig构造函数
SystemConfig::SystemConfig() 
{
    strcpy(magic, "HAOSYSCF");
    version = 10001;
    max_databases = 100;
    max_connections_total = 500;
    default_page_size = 4096;
    strcpy(default_charset, "UTF-8");
    memset(system_log_path, 0, sizeof(system_log_path));
    memset(temp_path, 0, sizeof(temp_path));
    memset(backup_path, 0, sizeof(backup_path));
    global_buffer_size = 134217728;  // 128MB
    checkpoint_interval = 300;
    enable_query_cache = 1;
    enable_authentication = 1;
    session_timeout = 3600;
    enable_audit_log = 1;
}

// DatabaseHeader构造函数
DatabaseHeader::DatabaseHeader() 
{
    strcpy(magic, "HAODB001");
    version = 10001;
    memset(db_name, 0, sizeof(db_name));
    db_id = 0;
    create_time = 0;
    last_modify = 0;
    page_size = 4096;
    charset = 1;
    table_count = 0;
    index_count = 0;
}

// 存储配置构造函数
StorageConfig::StorageConfig()
{
    memset(data_file_path, 0, sizeof(data_file_path));
    memset(log_file_path, 0, sizeof(log_file_path));
    memset(index_file_path, 0, sizeof(index_file_path));
    buffer_pool_size = 33554432;     // 32MB默认
    max_connections = 50;
    lock_timeout = 30;
    auto_checkpoint = 1;
    page_cache_size = 16777216;      // 16MB默认
    enable_compression = 0;
    checkpoint_pages = 1000;
}

// 日志配置构造函数
LogConfig::LogConfig() 
{
    log_level = 2;                   // INFO级别
    log_file_size = 50;              // 50MB
    log_file_count = 5;
    enable_wal = 1;
    wal_buffer_size = 8388608;       // 8MB
    sync_mode = 1;                   // 同步模式
    wal_checkpoint_size = 67108864;  // 64MB
    enable_log_rotation = 1;
}

// 数据库统计信息构造函数
DatabaseStats::DatabaseStats()
{
    total_pages = 0;
    free_pages = 0;
    total_size = 0;
    last_backup_time = 0;
    transaction_count = 0;
    total_reads = 0;
    total_writes = 0;
    cache_hits = 0;
    cache_misses = 0;
    active_connections = 0;
}

// ColumnMeta构造函数
ColumnMeta::ColumnMeta() 
{
    memset(this, 0, sizeof(ColumnMeta));
}

// TableMeta构造函数
TableMeta::TableMeta() 
{
    memset(this, 0, sizeof(TableMeta));
    table_type = 0;
}

// IndexMeta构造函数
IndexMeta::IndexMeta() 
{
    memset(this, 0, sizeof(IndexMeta));
}

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
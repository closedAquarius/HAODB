#pragma once
#include <cstdint>
#include <cstring>

// ���ݿ�ע����Ŀ
struct DatabaseEntry 
{
    char db_name[64];
    char db_path[256];
    uint32_t db_id;
    uint8_t status;           // 0:���� 1:���� 2:ά����
    uint64_t create_time;
    uint64_t last_access;
    char owner[32];
    uint8_t auto_start;

    DatabaseEntry();
};

// ���ݿ�ע���ͷ��
struct DatabaseRegistry 
{
    char magic[8];            // "HAODBSYS"
    uint32_t version;
    uint32_t database_count;
    uint64_t last_update;

    DatabaseRegistry();
};

// ϵͳ���ýṹ
struct SystemConfig 
{
    char magic[8];            // "HAOSYSCF"
    uint32_t version;
    uint32_t max_databases;
    uint32_t max_connections_total;
    uint32_t default_page_size;
    char default_charset[16];
    char system_log_path[256];
    char temp_path[256];
    char backup_path[256];
    uint32_t global_buffer_size;
    uint32_t checkpoint_interval;
    uint8_t enable_query_cache;
    uint8_t enable_authentication;
    uint32_t session_timeout;
    uint8_t enable_audit_log;

    SystemConfig();
};

// ���ݿ�ͷ����Ϣ
struct DatabaseHeader 
{
    char magic[8];            // "HAODB001"
    uint32_t version;
    char db_name[64];
    uint32_t db_id;
    uint64_t create_time;
    uint64_t last_modify;
    uint32_t page_size;
    uint32_t charset;
    uint64_t table_count;
    uint64_t index_count;

    DatabaseHeader();
};

// ��Ԫ����
struct ColumnMeta 
{
    char column_name[64];
    uint32_t column_id;
    uint8_t data_type;        // 1:INT 2:VARCHAR 3:DECIMAL 4:DATETIME
    uint32_t max_length;
    uint8_t is_nullable;
    uint8_t is_primary_key;
    uint8_t is_unique;
    char default_value[128];
    uint32_t column_offset;

    ColumnMeta();
};

// ��Ԫ����
struct TableMeta 
{
    char table_name[64];
    uint32_t table_id;
    uint64_t row_count;
    uint32_t column_count;
    uint64_t data_file_offset;
    uint64_t create_time;
    uint8_t table_type;
    ColumnMeta columns[32];   // ���32��

    TableMeta();
};

// ����Ԫ����
struct IndexMeta 
{
    char index_name[64];
    uint32_t index_id;
    uint32_t table_id;
    uint8_t index_type;
    uint8_t is_unique;
    uint32_t column_count;
    uint32_t columns[8];      // ���8����ɸ�������
    uint64_t root_page_id;

    IndexMeta();
};

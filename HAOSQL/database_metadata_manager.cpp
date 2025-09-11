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

//==================== �ļ����� ====================
bool DatabaseMetadataManager::LoadMetadata() 
{
    std::ifstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "Ԫ�����ļ�������: " << metadata_file_path << std::endl;
        return false;
    }

    // ��ȡ���ݿ�ͷ��
    file.read(reinterpret_cast<char*>(&db_header), sizeof(DatabaseHeader));

    // ��֤�ļ���ʽ
    if (strncmp(db_header.magic, "HAODB001", 8) != 0) 
    {
        std::cerr << "��Ч��Ԫ�����ļ���ʽ" << std::endl;
        return false;
    }

    // ��ȡ�洢����
    file.read(reinterpret_cast<char*>(&storage_config), sizeof(StorageConfig));

    // ��ȡ��־����
    file.read(reinterpret_cast<char*>(&log_config), sizeof(LogConfig));

    // ��ȡͳ����Ϣ
    file.read(reinterpret_cast<char*>(&db_stats), sizeof(DatabaseStats));

    // ��ȡ��Ԫ����
    tables.clear();
    tables.resize(db_header.table_count);
    for (uint64_t i = 0; i < db_header.table_count; i++) 
    {
        file.read(reinterpret_cast<char*>(&tables[i]), sizeof(TableMeta));
    }

    // ��ȡ����Ԫ����
    indexes.clear();
    indexes.resize(db_header.index_count);
    for (uint64_t i = 0; i < db_header.index_count; i++) 
    {
        file.read(reinterpret_cast<char*>(&indexes[i]), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "���ݿ�Ԫ���ݼ��سɹ�: " << db_header.db_name << std::endl;
    std::cout << "������: " << db_header.table_count
        << ", ��������: " << db_header.index_count << std::endl;
    return true;
}

bool DatabaseMetadataManager::SaveMetadata() 
{
    std::ofstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) 
    {
        std::cerr << "�޷�д��Ԫ�����ļ�: " << metadata_file_path << std::endl;
        return false;
    }

    // ����ͷ����Ϣ
    db_header.table_count = tables.size();
    db_header.index_count = indexes.size();
    db_header.last_modify = static_cast<uint64_t>(time(nullptr));

    // д�����ݿ�ͷ��
    file.write(reinterpret_cast<const char*>(&db_header), sizeof(DatabaseHeader));

    // д��洢����
    file.write(reinterpret_cast<const char*>(&storage_config), sizeof(StorageConfig));

    // д����־����
    file.write(reinterpret_cast<const char*>(&log_config), sizeof(LogConfig));

    // д��ͳ����Ϣ
    file.write(reinterpret_cast<const char*>(&db_stats), sizeof(DatabaseStats));

    // д���Ԫ����
    for (const auto& table : tables) 
    {
        file.write(reinterpret_cast<const char*>(&table), sizeof(TableMeta));
    }

    // д������Ԫ����
    for (const auto& index : indexes)
    {
        file.write(reinterpret_cast<const char*>(&index), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "���ݿ�Ԫ���ݱ���ɹ�" << std::endl;
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

    // ����Ĭ��·��
    std::string base_path = "HAODB\\" + db_name + "\\";
    strcpy(storage_config.data_file_path, (base_path + "data\\" + db_name + ".dat").c_str());
    strcpy(storage_config.log_file_path, (base_path + "logs\\" + db_name + ".log").c_str());
    strcpy(storage_config.index_file_path, (base_path + "index\\" + db_name + ".idx").c_str());

    tables.clear();
    indexes.clear();

    std::cout << "�����ݿ� " << db_name << " �����ɹ�" << std::endl;
    return SaveMetadata();
}

//==================== ����� ====================
bool DatabaseMetadataManager::CreateTable(const std::string& table_name,
    uint32_t table_id,
    const std::vector<ColumnMeta>& columns,
    uint64_t data_offset)
{
    // �������Ƿ��Ѵ���
    for (const auto& table : tables)
    {
        if (strcmp(table.table_name, table_name.c_str()) == 0) 
        {
            std::cerr << "�� " << table_name << " �Ѵ���" << std::endl;
            return false;
        }
    }

    // �����±�
    TableMeta new_table;
    strcpy(new_table.table_name, table_name.c_str());
    new_table.table_id = table_id;
    new_table.row_count = 0;
    new_table.column_count = columns.size();
    new_table.data_file_offset = data_offset;
    new_table.create_time = static_cast<uint64_t>(time(nullptr));
    new_table.table_type = 0;

    // �����ж���
    for (size_t i = 0; i < columns.size() && i < 32; i++) 
    {
        new_table.columns[i] = columns[i];
    }

    tables.push_back(new_table);
    std::cout << "�� " << table_name << " �����ɹ�" << std::endl;
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

        // ɾ���������
        indexes.erase(std::remove_if(indexes.begin(), indexes.end(),
            [table_id](const IndexMeta& index)
            {
                return index.table_id == table_id;
            }), indexes.end());

        std::cout << "�� " << table_name << " ɾ���ɹ�" << std::endl;
        return true;
    }

    std::cerr << "δ�ҵ���: " << table_name << std::endl;
    return false;
}

bool DatabaseMetadataManager::UpdateTableRowCount(const std::string& table_name, uint64_t row_count) 
{
    TableMeta* table = GetTableInfo(table_name);
    if (table) 
    {
        table->row_count = row_count;
        std::cout << "�� " << table_name << " ��������Ϊ: " << row_count << std::endl;
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

//==================== �������� ====================
bool DatabaseMetadataManager::CreateIndex(const std::string& index_name,
    uint32_t index_id,
    uint32_t table_id,
    const std::vector<uint32_t>& column_ids,
    bool is_unique,
    uint64_t root_page_id) 
{
    // ����������Ƿ��Ѵ���
    for (const auto& index : indexes) 
    {
        if (strcmp(index.index_name, index_name.c_str()) == 0) 
        {
            std::cerr << "���� " << index_name << " �Ѵ���" << std::endl;
            return false;
        }
    }

    // ����������
    IndexMeta new_index;
    strcpy(new_index.index_name, index_name.c_str());
    new_index.index_id = index_id;
    new_index.table_id = table_id;
    new_index.index_type = 0;  // B+��
    new_index.is_unique = is_unique ? 1 : 0;
    new_index.column_count = column_ids.size();
    new_index.root_page_id = root_page_id;

    // ������ID
    for (size_t i = 0; i < column_ids.size() && i < 8; i++)
    {
        new_index.columns[i] = column_ids[i];
    }

    indexes.push_back(new_index);
    std::cout << "���� " << index_name << " �����ɹ�" << std::endl;
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
        std::cout << "���� " << index_name << " ɾ���ɹ�" << std::endl;
        return true;
    }

    std::cerr << "δ�ҵ�����: " << index_name << std::endl;
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

//==================== �й��� ====================
bool DatabaseMetadataManager::AddColumn(const std::string& table_name, const ColumnMeta& column)
{
    TableMeta* table = GetTableInfo(table_name);
    if (!table) 
    {
        std::cerr << "δ�ҵ���: " << table_name << std::endl;
        return false;
    }

    if (table->column_count >= 32) 
    {
        std::cerr << "�� " << table_name << " �����Ѵ�����" << std::endl;
        return false;
    }

    table->columns[table->column_count] = column;
    table->column_count++;

    std::cout << "�� " << column.column_name << " ��ӵ��� " << table_name << " �ɹ�" << std::endl;
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
            // ���������ǰ��
            for (uint32_t j = i; j < table->column_count - 1; j++) 
            {
                table->columns[j] = table->columns[j + 1];
            }
            table->column_count--;

            std::cout << "�� " << column_name << " �ӱ� " << table_name << " ɾ���ɹ�" << std::endl;
            return true;
        }
    }

    std::cerr << "δ�ҵ���: " << column_name << std::endl;
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

//==================== ��ʾ��Ϣ ====================
void DatabaseMetadataManager::ShowDatabaseStructure()
{
    std::cout << "\n=== ���ݿ�ṹ: " << db_header.db_name << " ===" << std::endl;
    std::cout << "���ݿ�ID: " << db_header.db_id << std::endl;
    std::cout << "ҳ���С: " << db_header.page_size << " bytes" << std::endl;
    std::cout << "������: " << tables.size() << ", ��������: " << indexes.size() << std::endl;

    std::cout << "\n--- ����Ϣ ---" << std::endl;
    for (const auto& table : tables) 
    {
        std::cout << "����: " << table.table_name
            << ", ID: " << table.table_id
            << ", ����: " << table.row_count
            << ", ����: " << table.column_count << std::endl;

        // ��ʾ����Ϣ
        for (uint32_t i = 0; i < table.column_count; i++)
        {
            const auto& col = table.columns[i];
            std::cout << "  ��: " << col.column_name
                << " (����:" << (int)col.data_type
                << ", ����:" << col.max_length;
            if (col.is_primary_key) std::cout << ", ����";
            if (col.is_unique) std::cout << ", Ψһ";
            if (!col.is_nullable) std::cout << ", �ǿ�";
            std::cout << ")" << std::endl;
        }
    }

    std::cout << "\n--- ������Ϣ ---" << std::endl;
    for (const auto& index : indexes) 
    {
        std::cout << "������: " << index.index_name
            << ", ��ID: " << index.table_id
            << ", " << (index.is_unique ? "Ψһ����" : "��ͨ����")
            << ", ��ҳ��: " << index.root_page_id << std::endl;
    }
}

void DatabaseMetadataManager::ShowTableStructure(const std::string& table_name)
{
    TableMeta* table = GetTableInfo(table_name);
    if (!table)
    {
        std::cerr << "δ�ҵ���: " << table_name << std::endl;
        return;
    }

    std::cout << "\n=== ��ṹ: " << table_name << " ===" << std::endl;
    std::cout << "��ID: " << table->table_id << ", ����: " << table->row_count << std::endl;
    std::cout << "����ƫ����: " << table->data_file_offset << std::endl;

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
    std::cout << "\n=== ������Ϣ ===" << std::endl;
    for (const auto& index : indexes)
    {
        TableMeta* table = GetTableInfo(index.table_id);
        std::cout << "����: " << index.index_name
            << " ON " << (table ? table->table_name : "UNKNOWN")
            << " (";
        for (uint32_t i = 0; i < index.column_count; i++)
        {
            if (i > 0) std::cout << ", ";
            // �ҵ���Ӧ������
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
    std::cout << "          " << db_header.db_name << " ���ݿ�������Ϣ" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    ShowDatabaseStructure();
    ShowStorageConfig();
    ShowLogConfig();
    ShowDatabaseStats();

    std::cout << std::string(60, '=') << std::endl;
}

//==================== ���ݿ������޸� ====================
bool DatabaseMetadataManager::SetDatabaseName(const std::string& new_name)
{
    strcpy(db_header.db_name, new_name.c_str());
    std::cout << "���ݿ����Ƹ���Ϊ: " << new_name << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetPageSize(uint32_t page_size) 
{
    db_header.page_size = page_size;
    std::cout << "ҳ���С����Ϊ: " << page_size << " bytes" << std::endl;
    return true;
}

//==================== ͳ����Ϣ���� ====================
bool DatabaseMetadataManager::UpdateTotalPages(uint64_t total, uint64_t free) 
{
    db_stats.total_pages = total;
    db_stats.free_pages = free;
    std::cout << "ҳ��ͳ�Ƹ���: ��ҳ��=" << total << ", ����ҳ��=" << free << std::endl;
    return true;
}

bool DatabaseMetadataManager::UpdateDatabaseSize(uint64_t size) 
{
    db_stats.total_size = size;
    std::cout << "���ݿ��С����Ϊ: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLastBackupTime(uint64_t timestamp) 
{
    db_stats.last_backup_time = timestamp;
    std::cout << "����ʱ�����" << std::endl;
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
    std::cout << "\n=== ���ݿ�ͳ����Ϣ ===" << std::endl;
    std::cout << "��ҳ����: " << db_stats.total_pages << std::endl;
    std::cout << "����ҳ����: " << db_stats.free_pages << std::endl;
    std::cout << "���ݿ��С: " << db_stats.total_size << " bytes" << std::endl;
    std::cout << "��������: " << db_stats.transaction_count << std::endl;
    std::cout << "�ܶ�ȡ����: " << db_stats.total_reads << std::endl;
    std::cout << "��д�����: " << db_stats.total_writes << std::endl;
    std::cout << "��������: " << db_stats.cache_hits << std::endl;
    std::cout << "����δ����: " << db_stats.cache_misses << std::endl;
    std::cout << "��ǰ������: " << db_stats.active_connections << std::endl;

    // ����ͳ�Ʊ���
    if (db_stats.cache_hits + db_stats.cache_misses > 0) {
        double hit_ratio = (double)db_stats.cache_hits / (db_stats.cache_hits + db_stats.cache_misses) * 100;
        std::cout << "����������: " << std::fixed << std::setprecision(2) << hit_ratio << "%" << std::endl;
    }

    if (db_stats.total_pages > 0) {
        double utilization = (double)(db_stats.total_pages - db_stats.free_pages) / db_stats.total_pages * 100;
        std::cout << "�洢������: " << std::fixed << std::setprecision(2) << utilization << "%" << std::endl;
    }
}

void DatabaseMetadataManager::ResetStatistics() 
{
    db_stats.total_reads = 0;
    db_stats.total_writes = 0;
    db_stats.cache_hits = 0;
    db_stats.cache_misses = 0;
    db_stats.transaction_count = 0;
    std::cout << "ͳ����Ϣ������" << std::endl;
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

//==================== �洢���ù��� ====================
bool DatabaseMetadataManager::SetDataFilePath(const std::string& path) 
{
    strcpy(storage_config.data_file_path, path.c_str());
    std::cout << "�����ļ�·������Ϊ: " << path << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLogFilePath(const std::string& path) 
{
    strcpy(storage_config.log_file_path, path.c_str());
    std::cout << "��־�ļ�·������Ϊ: " << path << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetIndexFilePath(const std::string& path) 
{
    strcpy(storage_config.index_file_path, path.c_str());
    std::cout << "�����ļ�·������Ϊ: " << path << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetBufferPoolSize(uint32_t size) 
{
    storage_config.buffer_pool_size = size;
    std::cout << "����ش�С����Ϊ: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetMaxConnections(uint32_t max_conn) 
{
    storage_config.max_connections = max_conn;
    std::cout << "�������������Ϊ: " << max_conn << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLockTimeout(uint32_t timeout) 
{
    storage_config.lock_timeout = timeout;
    std::cout << "����ʱʱ������Ϊ: " << timeout << " ��" << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableAutoCheckpoint(bool enable) 
{
    storage_config.auto_checkpoint = enable ? 1 : 0;
    std::cout << "�Զ�����" << (enable ? "����" : "����") << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetPageCacheSize(uint32_t size) 
{
    storage_config.page_cache_size = size;
    std::cout << "ҳ�滺���С����Ϊ: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableCompression(bool enable) 
{
    storage_config.enable_compression = enable ? 1 : 0;
    std::cout << "����ѹ��" << (enable ? "����" : "����") << std::endl;
    return true;
}

void DatabaseMetadataManager::ShowStorageConfig() 
{
    std::cout << "\n=== �洢���� ===" << std::endl;
    std::cout << "�����ļ�·��: " << storage_config.data_file_path << std::endl;
    std::cout << "��־�ļ�·��: " << storage_config.log_file_path << std::endl;
    std::cout << "�����ļ�·��: " << storage_config.index_file_path << std::endl;
    std::cout << "����ش�С: " << storage_config.buffer_pool_size << " bytes" << std::endl;
    std::cout << "���������: " << storage_config.max_connections << std::endl;
    std::cout << "����ʱʱ��: " << storage_config.lock_timeout << " ��" << std::endl;
    std::cout << "�Զ�����: " << (storage_config.auto_checkpoint ? "����" : "����") << std::endl;
    std::cout << "ҳ�滺���С: " << storage_config.page_cache_size << " bytes" << std::endl;
    std::cout << "����ѹ��: " << (storage_config.enable_compression ? "����" : "����") << std::endl;
}

//==================== ��־���ù��� ====================
bool DatabaseMetadataManager::SetLogLevel(uint8_t level) 
{
    if (level > 3)
    {
        std::cerr << "��Ч����־����: " << (int)level << std::endl;
        return false;
    }
    log_config.log_level = level;
    const char* level_names[] = { "ERROR", "WARN", "INFO", "DEBUG" };
    std::cout << "��־��������Ϊ: " << level_names[level] << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLogFileSize(uint32_t size_mb) 
{
    log_config.log_file_size = size_mb;
    std::cout << "��־�ļ���С����Ϊ: " << size_mb << " MB" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetLogFileCount(uint32_t count)
{
    log_config.log_file_count = count;
    std::cout << "������־�ļ���������Ϊ: " << count << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableWAL(bool enable) 
{
    log_config.enable_wal = enable ? 1 : 0;
    std::cout << "WAL��־" << (enable ? "����" : "����") << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetWALBufferSize(uint32_t size)
{
    log_config.wal_buffer_size = size;
    std::cout << "WAL��������С����Ϊ: " << size << " bytes" << std::endl;
    return true;
}

bool DatabaseMetadataManager::SetSyncMode(uint8_t mode)
{
    if (mode > 2) 
    {
        std::cerr << "��Ч��ͬ��ģʽ: " << (int)mode << std::endl;
        return false;
    }
    log_config.sync_mode = mode;
    const char* mode_names[] = { "�첽", "ͬ��", "ȫͬ��" };
    std::cout << "ͬ��ģʽ����Ϊ: " << mode_names[mode] << std::endl;
    return true;
}

bool DatabaseMetadataManager::EnableLogRotation(bool enable)
{
    log_config.enable_log_rotation = enable ? 1 : 0;
    std::cout << "��־��ת" << (enable ? "����" : "����") << std::endl;
    return true;
}

void DatabaseMetadataManager::ShowLogConfig() 
{
    std::cout << "\n=== ��־���� ===" << std::endl;
    const char* level_names[] = { "ERROR", "WARN", "INFO", "DEBUG" };
    std::cout << "��־����: " << level_names[log_config.log_level] << std::endl;
    std::cout << "��־�ļ���С: " << log_config.log_file_size << " MB" << std::endl;
    std::cout << "�����ļ�����: " << log_config.log_file_count << std::endl;
    std::cout << "WAL��־: " << (log_config.enable_wal ? "����" : "����") << std::endl;
    std::cout << "WAL������: " << log_config.wal_buffer_size << " bytes" << std::endl;
    const char* sync_names[] = { "�첽", "ͬ��", "ȫͬ��" };
    std::cout << "ͬ��ģʽ: " << sync_names[log_config.sync_mode] << std::endl;
    std::cout << "��־��ת: " << (log_config.enable_log_rotation ? "����" : "����") << std::endl;
}

//==================== ���ܷ��� ====================
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
    std::cout << "\n=== ���ܷ������� ===" << std::endl;
    std::cout << "���ݿ�: " << db_header.db_name << std::endl;
    std::cout << "����ʱ��: " << time(nullptr) << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    // ��������
    std::cout << "��������:" << std::endl;
    std::cout << "  ������: " << std::fixed << std::setprecision(2)
        << GetCacheHitRatio() << "%" << std::endl;
    std::cout << "  �ܷ���: " << (db_stats.cache_hits + db_stats.cache_misses) << std::endl;

    // I/O����
    std::cout << "I/O����:" << std::endl;
    std::cout << "  ��ȡ����: " << db_stats.total_reads << std::endl;
    std::cout << "  д�����: " << db_stats.total_writes << std::endl;
    std::cout << "  ��д����: " << std::fixed << std::setprecision(2);
    if (db_stats.total_writes > 0) 
    {
        std::cout << (double)db_stats.total_reads / db_stats.total_writes << ":1" << std::endl;
    }
    else {
        std::cout << "N/A" << std::endl;
    }

    // �洢����
    std::cout << "�洢����:" << std::endl;
    if (db_stats.total_pages > 0)
    {
        double utilization = (double)(db_stats.total_pages - db_stats.free_pages) / db_stats.total_pages * 100;
        std::cout << "  ������: " << std::fixed << std::setprecision(2) << utilization << "%" << std::endl;
    }
    std::cout << "  ƽ�������С: " << GetAverageTransactionSize() << " I/O" << std::endl;

    // ��������
    std::cout << "��������:" << std::endl;
    std::cout << "  ��ǰ������: " << db_stats.active_connections << std::endl;
    std::cout << "  ���������: " << storage_config.max_connections << std::endl;
    if (storage_config.max_connections > 0) 
    {
        double conn_utilization = (double)db_stats.active_connections / storage_config.max_connections * 100;
        std::cout << "  ����������: " << std::fixed << std::setprecision(2) << conn_utilization << "%" << std::endl;
    }
}

//==================== ���ù������� ====================
// �洢���ù�����ʵ��
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
    config.buffer_pool_size = size_mb * 1024 * 1024;  // ת��Ϊ�ֽ�
    return true;
}

bool StorageConfigManager::SetPageCacheSize(uint32_t size_mb)
{
    config.page_cache_size = size_mb * 1024 * 1024;  // ת��Ϊ�ֽ�
    return true;
}

bool StorageConfigManager::SetMaxConnections(uint32_t max_conn) 
{
    if (max_conn > 1000) {  // �������������
        std::cerr << "����: ������������󣬽��鲻����1000" << std::endl;
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
    // ���·���Ƿ�Ϊ��
    if (strlen(config.data_file_path) == 0) 
    {
        std::cerr << "����: �����ļ�·������Ϊ��" << std::endl;
        return false;
    }

    // ��黺��ش�С�Ƿ����
    if (config.buffer_pool_size < 1024 * 1024)
    {  // С��1MB
        std::cerr << "����: ����ش�С��С����������1MB" << std::endl;
    }

    // ����������Ƿ����
    if (config.max_connections == 0) 
    {
        std::cerr << "����: �������������Ϊ0" << std::endl;
        return false;
    }

    return true;
}

void StorageConfigManager::PrintConfig() 
{
    std::cout << "=== �洢���� ===" << std::endl;
    std::cout << "�����ļ�: " << config.data_file_path << std::endl;
    std::cout << "��־�ļ�: " << config.log_file_path << std::endl;
    std::cout << "�����ļ�: " << config.index_file_path << std::endl;
    std::cout << "�����: " << (config.buffer_pool_size / 1024 / 1024) << " MB" << std::endl;
    std::cout << "ҳ�滺��: " << (config.page_cache_size / 1024 / 1024) << " MB" << std::endl;
    std::cout << "�������: " << config.max_connections << std::endl;
    std::cout << "����ʱ: " << config.lock_timeout << " ��" << std::endl;
    std::cout << "�Զ�����: " << (config.auto_checkpoint ? "����" : "����") << std::endl;
    std::cout << "����ѹ��: " << (config.enable_compression ? "����" : "����") << std::endl;
}

// ��־���ù�����ʵ��
bool LogConfigManager::SetLogLevel(const std::string& level) 
{
    if (level == "ERROR") { config.log_level = 0; }
    else if (level == "WARN") { config.log_level = 1; }
    else if (level == "INFO") { config.log_level = 2; }
    else if (level == "DEBUG") { config.log_level = 3; }
    else 
    {
        std::cerr << "��Ч����־����: " << level << std::endl;
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
        std::cerr << "��Ч��ͬ��ģʽ: " << mode << std::endl;
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

// ͳ����Ϣ������ʵ��
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
    std::cout << "����ͳ����Ϣ������" << std::endl;
}
#include "database_metadata_manager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>

DatabaseMetadataManager::DatabaseMetadataManager(const std::string& file_path)
    : metadata_file_path(file_path) {
}

bool DatabaseMetadataManager::LoadMetadata() {
    std::ifstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ԫ�����ļ�������: " << metadata_file_path << std::endl;
        return false;
    }

    // ��ȡ���ݿ�ͷ��
    file.read(reinterpret_cast<char*>(&db_header), sizeof(DatabaseHeader));

    // ��֤�ļ���ʽ
    if (strncmp(db_header.magic, "HAODB001", 8) != 0) {
        std::cerr << "��Ч��Ԫ�����ļ���ʽ" << std::endl;
        return false;
    }

    // ��ȡ��Ԫ����
    tables.clear();
    tables.resize(db_header.table_count);
    for (uint64_t i = 0; i < db_header.table_count; i++) {
        file.read(reinterpret_cast<char*>(&tables[i]), sizeof(TableMeta));
    }

    // ��ȡ����Ԫ����
    indexes.clear();
    indexes.resize(db_header.index_count);
    for (uint64_t i = 0; i < db_header.index_count; i++) {
        file.read(reinterpret_cast<char*>(&indexes[i]), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "���ݿ�Ԫ���ݼ��سɹ�: " << db_header.db_name << std::endl;
    std::cout << "������: " << db_header.table_count
        << ", ��������: " << db_header.index_count << std::endl;
    return true;
}

bool DatabaseMetadataManager::SaveMetadata() {
    std::ofstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "�޷�д��Ԫ�����ļ�: " << metadata_file_path << std::endl;
        return false;
    }

    // ����ͷ����Ϣ
    db_header.table_count = tables.size();
    db_header.index_count = indexes.size();
    db_header.last_modify = static_cast<uint64_t>(time(nullptr));

    // д�����ݿ�ͷ��
    file.write(reinterpret_cast<const char*>(&db_header), sizeof(DatabaseHeader));

    // д���Ԫ����
    for (const auto& table : tables) {
        file.write(reinterpret_cast<const char*>(&table), sizeof(TableMeta));
    }

    // д������Ԫ����
    for (const auto& index : indexes) {
        file.write(reinterpret_cast<const char*>(&index), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "���ݿ�Ԫ���ݱ���ɹ�" << std::endl;
    return true;
}

bool DatabaseMetadataManager::CreateNewDatabase(const std::string& db_name, uint32_t db_id) {
    db_header = DatabaseHeader();  // ʹ��Ĭ�Ϲ��캯��
    strcpy(db_header.db_name, db_name.c_str());
    db_header.db_id = db_id;
    db_header.create_time = static_cast<uint64_t>(time(nullptr));
    db_header.last_modify = db_header.create_time;

    tables.clear();
    indexes.clear();

    std::cout << "�����ݿ� " << db_name << " �����ɹ�" << std::endl;
    return SaveMetadata();
}

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

uint64_t DatabaseMetadataManager::GetTotalRowCount() const 
{
    uint64_t total_rows = 0;
    for (const auto& table : tables)
    {
        total_rows += table.row_count;
    }
    return total_rows;
}
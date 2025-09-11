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
        std::cerr << "元数据文件不存在: " << metadata_file_path << std::endl;
        return false;
    }

    // 读取数据库头部
    file.read(reinterpret_cast<char*>(&db_header), sizeof(DatabaseHeader));

    // 验证文件格式
    if (strncmp(db_header.magic, "HAODB001", 8) != 0) {
        std::cerr << "无效的元数据文件格式" << std::endl;
        return false;
    }

    // 读取表元数据
    tables.clear();
    tables.resize(db_header.table_count);
    for (uint64_t i = 0; i < db_header.table_count; i++) {
        file.read(reinterpret_cast<char*>(&tables[i]), sizeof(TableMeta));
    }

    // 读取索引元数据
    indexes.clear();
    indexes.resize(db_header.index_count);
    for (uint64_t i = 0; i < db_header.index_count; i++) {
        file.read(reinterpret_cast<char*>(&indexes[i]), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "数据库元数据加载成功: " << db_header.db_name << std::endl;
    std::cout << "表数量: " << db_header.table_count
        << ", 索引数量: " << db_header.index_count << std::endl;
    return true;
}

bool DatabaseMetadataManager::SaveMetadata() {
    std::ofstream file(metadata_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法写入元数据文件: " << metadata_file_path << std::endl;
        return false;
    }

    // 更新头部信息
    db_header.table_count = tables.size();
    db_header.index_count = indexes.size();
    db_header.last_modify = static_cast<uint64_t>(time(nullptr));

    // 写入数据库头部
    file.write(reinterpret_cast<const char*>(&db_header), sizeof(DatabaseHeader));

    // 写入表元数据
    for (const auto& table : tables) {
        file.write(reinterpret_cast<const char*>(&table), sizeof(TableMeta));
    }

    // 写入索引元数据
    for (const auto& index : indexes) {
        file.write(reinterpret_cast<const char*>(&index), sizeof(IndexMeta));
    }

    file.close();
    std::cout << "数据库元数据保存成功" << std::endl;
    return true;
}

bool DatabaseMetadataManager::CreateNewDatabase(const std::string& db_name, uint32_t db_id) {
    db_header = DatabaseHeader();  // 使用默认构造函数
    strcpy(db_header.db_name, db_name.c_str());
    db_header.db_id = db_id;
    db_header.create_time = static_cast<uint64_t>(time(nullptr));
    db_header.last_modify = db_header.create_time;

    tables.clear();
    indexes.clear();

    std::cout << "新数据库 " << db_name << " 创建成功" << std::endl;
    return SaveMetadata();
}

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

uint64_t DatabaseMetadataManager::GetTotalRowCount() const 
{
    uint64_t total_rows = 0;
    for (const auto& table : tables)
    {
        total_rows += table.row_count;
    }
    return total_rows;
}
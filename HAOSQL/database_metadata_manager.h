#pragma once
#include "metadata_structures.h"
#include <string>
#include <vector>

class DatabaseMetadataManager 
{
private:
    std::string metadata_file_path;
    DatabaseHeader db_header;
    std::vector<TableMeta> tables;
    std::vector<IndexMeta> indexes;

public:
    explicit DatabaseMetadataManager(const std::string& file_path);

    // 文件操作
    bool LoadMetadata();
    bool SaveMetadata();
    bool CreateNewDatabase(const std::string& db_name, uint32_t db_id);

    // 表管理
    bool CreateTable(const std::string& table_name, uint32_t table_id,
        const std::vector<ColumnMeta>& columns, uint64_t data_offset);
    bool DropTable(const std::string& table_name);
    bool UpdateTableRowCount(const std::string& table_name, uint64_t row_count);
    TableMeta* GetTableInfo(const std::string& table_name);
    TableMeta* GetTableInfo(uint32_t table_id);

    // 索引管理
    bool CreateIndex(const std::string& index_name, uint32_t index_id,
        uint32_t table_id, const std::vector<uint32_t>& column_ids,
        bool is_unique, uint64_t root_page_id);
    bool DropIndex(const std::string& index_name);
    IndexMeta* GetIndexInfo(const std::string& index_name);
    std::vector<IndexMeta*> GetTableIndexes(uint32_t table_id);

    // 列管理
    bool AddColumn(const std::string& table_name, const ColumnMeta& column);
    bool DropColumn(const std::string& table_name, const std::string& column_name);
    ColumnMeta* GetColumnInfo(const std::string& table_name, const std::string& column_name);

    // 查询接口
    const DatabaseHeader& GetDatabaseHeader() const { return db_header; }
    const std::vector<TableMeta>& GetTables() const { return tables; }
    const std::vector<IndexMeta>& GetIndexes() const { return indexes; }

    // 显示信息
    void ShowDatabaseStructure();
    void ShowTableStructure(const std::string& table_name);
    void ShowIndexInfo();

    // 数据库属性修改
    bool SetDatabaseName(const std::string& new_name);
    bool SetPageSize(uint32_t page_size);

    // 统计信息
    size_t GetTableCount() const { return tables.size(); }
    size_t GetIndexCount() const { return indexes.size(); }
    uint64_t GetTotalRowCount() const;
};
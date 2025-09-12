#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

#include "file_manager.h"
#include "B+tree.h"
#include "catalog_manager.h"   // ✅ 改成你队友的 CatalogManager 定义

// ---------------- IndexManager ----------------
class IndexManager {
public:
    IndexManager(FileManager& fm, CatalogManager& catalog, const std::string& db_name);

    // 仅在 Catalog 写入元数据（不自动创建物理索引文件）
    bool CreateIndex(const std::string& table_name,
        const std::string& index_name,
        const std::vector<std::string>& column_names,
        bool is_unique = false,
        int degree = 4);

    // 删除索引（销毁物理索引并移除元数据）
    bool DropIndex(const std::string& index_name);

    // 列出索引（直接调用 catalog）
    std::vector<IndexInfo> ListIndexes(const std::string& table_name = "");

    // 检查索引存在性
    bool IndexExists(const std::string& index_name);

    // 查找所有包含指定列的索引（调用 catalog）
    std::vector<IndexInfo> FindIndexesWithColumns(const std::string& table_name,
        const std::vector<std::string>& column_names);

    // 打开索引（加载 BPlusTree 到内存缓存）
    bool OpenIndex(const IndexInfo& info);

    // 关闭索引（写回 meta 并释放内存）
    bool CloseIndex(const std::string& index_name);

    // 执行器调用接口：插入 / 删除 / 精确查找 / 范围查找
    bool InsertEntry(const std::string& index_name, int key, const RID& rid);
    bool DeleteEntry(const std::string& index_name, int key, const RID& rid);
    std::vector<RID> Search(const std::string& index_name, int key);
    std::vector<RID> SearchRange(const std::string& index_name, int low, int high);

    // Drop table 时：删除该表的所有索引
    bool DropIndexesOnTable(const std::string& table_name);

private:
    struct IndexInstance {
        IndexInfo meta;                  // 来自 CatalogManager 的 IndexInfo
        std::unique_ptr<BPlusTree> tree; // nullptr 表示未 load
        std::mutex mtx;                  // 用于该索引的并发控制
    };

    // 确保索引已 load 到内存（加载 BPlusTree），返回 true 成功
    bool ensureIndexLoaded(const std::string& index_name);

    // helper: 根据 index_name 查找缓存实例
    IndexInstance* findInstance(const std::string& index_name);

    FileManager& fm_;
    CatalogManager& catalog_;  // ✅ 替换成 CatalogManager
    std::string db_name_;

    std::mutex global_mtx_;
    std::unordered_map<std::string, IndexInstance> instances_; // keyed by index_name
};

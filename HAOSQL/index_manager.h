#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

#include "file_manager.h"
#include "B+tree.h"
#include "catalog_manager.h"

struct IndexInstance {

    IndexInstance(IndexInstance&&) noexcept = default;
    IndexInstance& operator=(IndexInstance&&) noexcept = default;
    IndexInstance(const IndexInstance&) = delete;
    IndexInstance& operator=(const IndexInstance&) = delete;

    IndexInfo meta;                  // 来自 CatalogManager 的 IndexInfo
    std::unique_ptr<BPlusTree> tree; // nullptr 表示未 load
    std::mutex mtx;                  // 用于该索引的并发控制

    IndexInstance() = default;       // 默认构造
};

// ---------------- IndexManager ----------------
class IndexManager {
public:
    IndexManager(FileManager& fm, CatalogManager& catalog, const std::string& db_name);

    bool CreateIndex(const std::string& table_name,
        const std::string& index_name,
        const std::vector<std::string>& column_names,
        bool is_unique);

    bool _updateRootIfNeeded(IndexInstance* inst);
    bool DropIndex(const std::string& index_name);
    std::vector<IndexInfo> ListIndexes(const std::string& table_name = "");
    bool IndexExists(const std::string& index_name);
    std::vector<IndexInfo> FindIndexesWithColumns(const std::string& table_name,
        const std::vector<std::string>& column_names);
    bool OpenIndex(const IndexInfo& info);
    bool CloseIndex(const std::string& index_name);

    bool InsertEntry(const std::string& index_name, int key, const RID& rid);
    bool DeleteEntry(const std::string& index_name, int key, const RID& rid);
    std::vector<RID> Search(const std::string& index_name, int key);
    std::vector<RID> SearchRange(const std::string& index_name, int low, int high);

    bool DropIndexesOnTable(const std::string& table_name);

private:
    bool ensureIndexLoaded(const std::string& index_name);
    IndexInstance* findInstance(const std::string& index_name);

    FileManager& fm_;
    CatalogManager& catalog_;
    std::string db_name_;

    std::mutex global_mtx_;
    std::unordered_map<std::string, std::unique_ptr<IndexInstance>> instances_; // keyed by index_name
};




#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "file_manager.h"
#include "catalog_manager.h"
#include "B+tree.h"

// 已定义结构体引用
struct RID;
struct IndexInfo;

// Index实例
struct IndexInstance {
    IndexInfo meta;
    std::unique_ptr<BPlusTree> tree;
    std::mutex mtx;
};

class IndexManager {
public:
    IndexManager(FileManager& fm, CatalogManager& catalog, const std::string& db_name);

    // ---------------- Create / Drop ----------------
    bool CreateIndex(const std::string& table_name,
        const std::string& index_name,
        const std::vector<std::string>& column_names,
        bool is_unique);

    bool DropIndex(const std::string& index_name);
    bool DropIndexesOnTable(const std::string& table_name);

    // ---------------- Close ----------------
    bool CloseIndex(const std::string& table_name, const std::vector<std::string>& column_names);

    // ---------------- Insert / Delete ----------------
    bool InsertEntry(const std::string& table_name,
        const std::vector<std::string>& column_names,
        int key,
        const RID& rid);

    bool DeleteEntry(const std::string& table_name,
        const std::vector<std::string>& column_names,
        int key,
        const RID& rid);

    // ---------------- Search ----------------
    std::vector<RID> Search(const std::string& table_name,
        const std::vector<std::string>& column_names,
        int key);

    std::vector<RID> SearchRange(const std::string& table_name,
        const std::vector<std::string>& column_names,
        int low, int high);

    // ---------------- List / Exists ----------------
    std::vector<IndexInfo> ListIndexes(const std::string& table_name = "");
    bool IndexExists(const std::string& index_name);

    std::vector<IndexInfo> FindIndexesWithColumns(const std::string& table_name,
        const std::vector<std::string>& column_names);

private:
    FileManager& fm_;
    CatalogManager& catalog_;
    std::string db_name_;
    std::unordered_map<std::string, std::unique_ptr<IndexInstance>> instances_;
    std::mutex global_mtx_;

    // ---------------- 内部方法 ----------------
    bool ensureIndexLoaded(const std::string& index_name);
    IndexInstance* findInstance(const std::string& index_name);
    bool OpenIndex(const IndexInfo& info);
    bool _updateRootIfNeeded(IndexInstance* inst);
};

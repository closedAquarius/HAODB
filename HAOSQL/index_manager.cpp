#include "index_manager.h"
#include <algorithm>
#include <cassert>
#include <iostream>

// ---------------- Constructor ----------------
IndexManager::IndexManager(FileManager& fm, CatalogManager* catalog)
    : fm_(fm), catalog_(catalog) {
}

// ---------------- CreateIndex ----------------
bool IndexManager::CreateIndex(const std::string& table_name,
    const std::string& index_name,
    const std::vector<std::string>& column_names,
    bool is_unique)
{
    std::lock_guard<std::mutex> g(global_mtx_);
    if (catalog_->IndexExists(DBName, index_name)) return false;

    uint32_t index_id = 0;
    std::string lower_db = DBName;
    std::transform(lower_db.begin(), lower_db.end(), lower_db.begin(),
        [](unsigned char c) { return std::tolower(c); });
    std::string indexFileName = DBName + "/index/" + lower_db + ".idx";
    std::cout << indexFileName << std::endl;

    FileManager fm("./HAODB/" + DBName + "/index");
    std::string i = lower_db + ".idx";
    std::cout << i << std::endl;
    int file_id = fm.openFile(i);

    int degree = 4;
    BPlusTree tree(fm, file_id, degree, 0);
    int root_pid = tree.createIndex(fm, file_id);

    if (!catalog_->CreateIndex(DBName, table_name, index_name, column_names, is_unique, root_pid))
        return false;

    catalog_->UpdateIndexRoot(DBName, index_name, static_cast<uint64_t>(root_pid));
    return true;
}

// ---------------- DropIndex ----------------
bool IndexManager::DropIndex(const std::string& index_name)
{
    std::lock_guard<std::mutex> g(global_mtx_);

    auto all = catalog_->ListIndexes(DBName);
    auto it = std::find_if(all.begin(), all.end(),
        [&](const IndexInfo& info) { return info.index_name == index_name; });
    if (it == all.end()) return false;
    IndexInfo found = *it;

    std::string lower_db = DBName;
    std::transform(lower_db.begin(), lower_db.end(), lower_db.begin(),
        [](unsigned char c) { return std::tolower(c); });
    std::cout << DBName << lower_db;
    std::string indexFileName = "./HAODB/" + DBName + "/index/" + lower_db + ".idx";
    FileManager fm("./HAODB/" + DBName + "/index");
    std::string i = lower_db + ".idx";
    std::cout << i << std::endl;
    int file_id = fm.openFile(i);

    BPlusTree tmp(fm, file_id, 4, static_cast<int>(found.root_page_id));
    tmp.destroyIndex(fm, file_id, static_cast<int>(found.root_page_id));

    return catalog_->DropIndex(DBName, index_name);
}

// ---------------- DropIndexesOnTable ----------------
bool IndexManager::DropIndexesOnTable(const std::string& table_name)
{
    std::lock_guard<std::mutex> g(global_mtx_);
    auto idxs = catalog_->ListIndexes(DBName, table_name);
    for (auto& info : idxs) {
        DropIndex(info.index_name);
    }
    return true;
}

// ---------------- CloseIndex ----------------
bool IndexManager::CloseIndex(const std::string& table_name, const std::vector<std::string>& column_names)
{
    // 无缓存版本不需要关闭，直接返回 true
    return true;
}

// ---------------- InsertEntry ----------------
bool IndexManager::InsertEntry(const std::string& table_name,
    const std::vector<std::string>& column_names,
    int key,
    const RID& rid)
{
    auto indexes = FindIndexesWithColumns(table_name, column_names);
    if (indexes.empty()) return false;

    const IndexInfo& idx = indexes[0];
    std::string lower_db = DBName;
    std::transform(lower_db.begin(), lower_db.end(), lower_db.begin(),
        [](unsigned char c) { return std::tolower(c); });
    FileManager fm("./HAODB/" + DBName + "/index");
    std::string fname = lower_db + ".idx";
    int file_id = fm.openFile(fname);

    BPlusTree tree(fm, file_id, 4, static_cast<int>(idx.root_page_id));
    tree.insert(key, rid);

    int currentRoot = tree.getRootPid();
    if (static_cast<uint64_t>(currentRoot) != idx.root_page_id) {
        catalog_->UpdateIndexRoot(DBName, idx.index_name, static_cast<uint64_t>(currentRoot));
    }
    tree.printTree();
    return true;
}

// ---------------- DeleteEntry ----------------
bool IndexManager::DeleteEntry(const std::string& table_name,
    const std::vector<std::string>& column_names,
    int key,
    const RID& rid)
{
    auto indexes = FindIndexesWithColumns(table_name, column_names);
    if (indexes.empty()) return false;

    const IndexInfo& idx = indexes[0];
    std::string lower_db = DBName;
    std::transform(lower_db.begin(), lower_db.end(), lower_db.begin(),
        [](unsigned char c) { return std::tolower(c); });
    FileManager fm("./HAODB/" + DBName + "/index");
    std::string fname = lower_db + ".idx";
    int file_id = fm.openFile(fname);

    BPlusTree tree(fm, file_id, 4, static_cast<int>(idx.root_page_id));
    tree.remove(key, rid);

    int currentRoot = tree.getRootPid();
    if (static_cast<uint64_t>(currentRoot) != idx.root_page_id) {
        catalog_->UpdateIndexRoot(DBName, idx.index_name, static_cast<uint64_t>(currentRoot));
    }
    tree.printTree();
    return true;
}

// ---------------- Search ----------------
std::vector<RID> IndexManager::Search(const std::string& table_name,
    const std::vector<std::string>& column_names,
    int key)
{
    auto indexes = FindIndexesWithColumns(table_name, column_names);
    if (indexes.empty()) return {};

    const IndexInfo& idx = indexes[0];
    std::string lower_db = DBName;
    std::transform(lower_db.begin(), lower_db.end(), lower_db.begin(),
        [](unsigned char c) { return std::tolower(c); });
    FileManager fm("./HAODB/" + DBName + "/index");
    std::string fname = lower_db + ".idx";
    int file_id = fm.openFile(fname);

    BPlusTree tree(fm, file_id, 4, static_cast<int>(idx.root_page_id));
    return tree.search(key);
}

// ---------------- SearchRange ----------------
std::vector<RID> IndexManager::SearchRange(const std::string& table_name,
    const std::vector<std::string>& column_names,
    int low, int high)
{
    auto indexes = FindIndexesWithColumns(table_name, column_names);
    if (indexes.empty()) return {};

    const IndexInfo& idx = indexes[0];
    std::string lower_db = DBName;
    std::transform(lower_db.begin(), lower_db.end(), lower_db.begin(),
        [](unsigned char c) { return std::tolower(c); });
    FileManager fm("./HAODB/" + DBName + "/index");
    std::string fname = lower_db + ".idx";
    int file_id = fm.openFile(fname);

    BPlusTree tree(fm, file_id, 4, static_cast<int>(idx.root_page_id));
    return tree.searchRange(low, high);
}

// ---------------- ListIndexes / IndexExists ----------------
std::vector<IndexInfo> IndexManager::ListIndexes(const std::string& table_name)
{
    return catalog_->ListIndexes(DBName, table_name);
}

bool IndexManager::IndexExists(const std::string& index_name)
{
    return catalog_->IndexExists(DBName, index_name);
}

// ---------------- FindIndexesWithColumns ----------------
std::vector<IndexInfo> IndexManager::FindIndexesWithColumns(
    const std::string& table_name,
    const std::vector<std::string>& column_names)
{
    std::vector<IndexInfo> result;
    auto allIndexes = catalog_->ListIndexes(DBName, table_name);

    for (const auto& idx : allIndexes) {
        bool match = true;
        for (const auto& col : column_names) {
            if (std::find(idx.column_names.begin(), idx.column_names.end(), col) == idx.column_names.end()) {
                match = false;
                break;
            }
        }
        if (match) result.push_back(idx);
    }
    return result;
}

// ---------------- FindIndexesByTable ----------------
std::vector<IndexInfo> IndexManager::FindIndexesByTable(const std::string& table_name) {
    return catalog_->ListIndexes(DBName, table_name);
}

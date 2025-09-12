#include "index_manager.h"
#include <iostream>
#include <cassert>
#include <algorithm>

IndexManager::IndexManager(FileManager& fm, CatalogManager& catalog, const std::string& db_name)
    : fm_(fm), catalog_(catalog), db_name_(db_name) {}

// ---------------- CreateIndex ----------------
bool IndexManager::CreateIndex(const std::string& table_name,
    const std::string& index_name,
    const std::vector<std::string>& column_names,
    bool is_unique)
{
    std::lock_guard<std::mutex> g(global_mtx_);

    if (catalog_.IndexExists(db_name_, index_name)) return false;

    uint32_t index_id = 0; // 可以通过 CatalogManager 获取
    std::string indexFileName = "HAODB/index/" + db_name_ + ".idx";
    int file_id = fm_.openFile(indexFileName);

    int degree = 4;
    BPlusTree tree(fm_, file_id, degree, 0);
    int root_pid = tree.createIndex(fm_, file_id);

    if (!catalog_.CreateIndex(db_name_, table_name, index_name, column_names, is_unique, root_pid))
        return false;

    catalog_.UpdateIndexRoot(db_name_, index_name, static_cast<uint64_t>(root_pid));

    // ---------------- 保存 IndexInstance 到 map ----------------
    auto inst = std::make_unique<IndexInstance>();
    inst->meta.index_name = index_name;
    inst->meta.index_id = index_id;
    inst->meta.table_name = table_name;
    inst->meta.column_names = column_names;
    inst->meta.is_unique = is_unique;
    inst->meta.root_page_id = root_pid;
    inst->tree = std::make_unique<BPlusTree>(fm_, file_id, degree, root_pid);

    instances_.emplace(index_name, std::move(inst));
    return true;
}

// ---------------- helper ----------------
bool IndexManager::_updateRootIfNeeded(IndexInstance* inst) {
    int currentRoot = inst->tree->getRootPid();
    if (static_cast<uint64_t>(currentRoot) != inst->meta.root_page_id) {
        inst->meta.root_page_id = currentRoot;
        return catalog_.UpdateIndexRoot(db_name_, inst->meta.index_name, static_cast<uint64_t>(currentRoot));
    }
    return true;
}

// ---------------- InsertEntry ----------------
bool IndexManager::InsertEntry(const std::string& index_name, int key, const RID& rid) {
    if (!ensureIndexLoaded(index_name)) return false;
    IndexInstance* inst = findInstance(index_name);
    assert(inst);
    std::lock_guard<std::mutex> ig(inst->mtx);

    inst->tree->insert(key, rid);

    int currentRoot = inst->tree->getRootPid();
    if (static_cast<uint64_t>(currentRoot) != inst->meta.root_page_id) {
        inst->meta.root_page_id = currentRoot;
        catalog_.UpdateIndexRoot(db_name_, index_name, static_cast<uint64_t>(currentRoot));
    }
    return true;
}

// ---------------- DeleteEntry ----------------
bool IndexManager::DeleteEntry(const std::string& index_name, int key, const RID& rid) {
    if (!ensureIndexLoaded(index_name)) return false;
    IndexInstance* inst = findInstance(index_name);
    assert(inst);
    std::lock_guard<std::mutex> ig(inst->mtx);

    inst->tree->remove(key, rid);

    int currentRoot = inst->tree->getRootPid();
    if (static_cast<uint64_t>(currentRoot) != inst->meta.root_page_id) {
        inst->meta.root_page_id = currentRoot;
        catalog_.UpdateIndexRoot(db_name_, index_name, static_cast<uint64_t>(currentRoot));
    }
    return true;
}

// ---------------- DropIndex ----------------
bool IndexManager::DropIndex(const std::string& index_name) {
    std::lock_guard<std::mutex> g(global_mtx_);

    auto all = catalog_.ListIndexes(db_name_);
    auto it = std::find_if(all.begin(), all.end(),
        [&](const IndexInfo& info) { return info.index_name == index_name; });
    if (it == all.end()) return false;
    IndexInfo found = *it;

    std::string indexFileName = "HAODB/index/" + db_name_ + ".idx";
    int file_id = fm_.openFile(indexFileName);

    auto instIt = instances_.find(index_name);
    if (instIt == instances_.end()) {
        BPlusTree tmp(fm_, file_id, 4, static_cast<int>(found.root_page_id));
        tmp.destroyIndex(fm_, file_id, static_cast<int>(found.root_page_id));
    }
    else {
        IndexInstance* inst = instIt->second.get();
        std::lock_guard<std::mutex> ig(inst->mtx);
        if (inst->tree) {
            inst->tree->destroyIndex(fm_, file_id, static_cast<int>(inst->meta.root_page_id));
            inst->tree.reset();
        }
        instances_.erase(instIt);
    }

    return catalog_.DropIndex(db_name_, index_name);
}

// ---------------- OpenIndex ----------------
bool IndexManager::OpenIndex(const IndexInfo& info) {
    std::lock_guard<std::mutex> g(global_mtx_);
    if (instances_.count(info.index_name)) return true;

    auto inst = std::make_unique<IndexInstance>();
    inst->meta = info;

    std::string indexFileName = "HAODB/index/" + db_name_ + ".idx";
    int file_id = fm_.openFile(indexFileName);

    inst->tree = std::make_unique<BPlusTree>(fm_, file_id, 4, static_cast<int>(inst->meta.root_page_id));
    instances_.emplace(info.index_name, std::move(inst));
    return true;
}

// ---------------- CloseIndex ----------------
bool IndexManager::CloseIndex(const std::string& index_name) {
    std::lock_guard<std::mutex> g(global_mtx_);
    auto it = instances_.find(index_name);
    if (it == instances_.end()) return true;

    IndexInstance* inst = it->second.get();
    std::lock_guard<std::mutex> ig(inst->mtx);
    if (inst->tree) {
        _updateRootIfNeeded(inst);
        inst->tree.reset();
    }
    instances_.erase(it);
    return true;
}

// ---------------- ensureIndexLoaded ----------------
bool IndexManager::ensureIndexLoaded(const std::string& index_name) {
    if (instances_.count(index_name)) return true;

    auto all = catalog_.ListIndexes(db_name_);
    for (auto& info : all) {
        if (info.index_name == index_name) {
            return OpenIndex(info);
        }
    }
    return false;
}

// ---------------- findInstance ----------------
IndexInstance* IndexManager::findInstance(const std::string& index_name) {
    auto it = instances_.find(index_name);
    if (it == instances_.end()) return nullptr;
    return it->second.get();
}

// ---------------- Search / SearchRange ----------------
std::vector<RID> IndexManager::Search(const std::string& index_name, int key) {
    if (!ensureIndexLoaded(index_name)) return {};
    IndexInstance* inst = findInstance(index_name);
    assert(inst);
    std::lock_guard<std::mutex> ig(inst->mtx);
    return inst->tree->search(key);
}

std::vector<RID> IndexManager::SearchRange(const std::string& index_name, int low, int high) {
    if (!ensureIndexLoaded(index_name)) return {};
    IndexInstance* inst = findInstance(index_name);
    assert(inst);
    std::lock_guard<std::mutex> ig(inst->mtx);
    return inst->tree->searchRange(low, high);
}

// ---------------- DropIndexesOnTable ----------------
bool IndexManager::DropIndexesOnTable(const std::string& table_name) {
    std::lock_guard<std::mutex> g(global_mtx_);
    auto idxs = catalog_.ListIndexes(db_name_, table_name);
    for (auto& info : idxs) {
        DropIndex(info.index_name);
    }
    return true;
}

// ---------------- ListIndexes ----------------
std::vector<IndexInfo> IndexManager::ListIndexes(const std::string& table_name) {
    // 如果 table_name 为空，返回该数据库所有索引
    return catalog_.ListIndexes(db_name_, table_name);
}

// ---------------- IndexExists ----------------
bool IndexManager::IndexExists(const std::string& index_name) {
    return catalog_.IndexExists(db_name_, index_name);
}

// ---------------- FindIndexesWithColumns ----------------
std::vector<IndexInfo> IndexManager::FindIndexesWithColumns(
    const std::string& table_name,
    const std::vector<std::string>& column_names)
{
    std::vector<IndexInfo> result;
    auto allIndexes = catalog_.ListIndexes(db_name_, table_name);

    for (const auto& idx : allIndexes) {
        bool match = true;
        for (const auto& col : column_names) {
            if (std::find(idx.column_names.begin(), idx.column_names.end(), col) == idx.column_names.end()) {
                match = false;
                break;
            }
        }
        if (match) {
            result.push_back(idx);
        }
    }
    return result;
}
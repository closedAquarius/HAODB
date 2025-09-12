/*#include "index_manager.h"
#include <iostream>
#include <cassert>

IndexManager::IndexManager(FileManager& fm, CatalogManager& catalog, const std::string& db_name)
    : fm_(fm), catalog_(catalog), db_name_(db_name) {}

bool IndexManager::CreateIndex(const std::string& table_name,
                               const std::string& index_name,
                               const std::vector<std::string>& column_names,
                               bool is_unique,
                               int degree) {
    std::lock_guard<std::mutex> g(global_mtx_);
    if (catalog_.IndexExists(db_name_, index_name)) return false;

    // 写 Catalog 元数据，获取真实 index_id
    uint32_t index_id = 0;
    if (!catalog_.CreateIndex(db_name_, table_name, index_name, column_names, is_unique, &index_id)) return false;

    // 创建物理 B+ 树
    int file_id = fm_.openFile(index_name + ".idx");
    BPlusTree tree(fm_, file_id, degree, -1);
    int root_pid = tree.createIndex(fm_, file_id);

    catalog_.UpdateIndexRoot(db_name_, index_name, static_cast<uint64_t>(root_pid));

    IndexInstance inst;
    inst.meta.index_name = index_name;
    inst.meta.index_id = index_id;
    inst.meta.table_name = table_name;
    inst.meta.column_names = column_names;
    inst.meta.is_unique = is_unique;
    inst.meta.root_page_id = root_pid;
    inst.meta.file_id = file_id;
    inst.meta.degree = degree;
    inst.tree = std::make_unique<BPlusTree>(fm_, file_id, degree, root_pid);

    instances_.emplace(index_name, std::move(inst));
    return true;
}

bool IndexManager::_updateRootIfNeeded(IndexInstance* inst) {
    int currentRoot = inst->tree->getRootPid();
    if (static_cast<uint64_t>(currentRoot) != inst->meta.root_page_id) {
        inst->meta.root_page_id = currentRoot;
        return catalog_.UpdateIndexRoot(db_name_, inst->meta.index_name, static_cast<uint64_t>(currentRoot));
    }
    return true;
}

bool IndexManager::InsertEntry(const std::string& index_name, int key, const RID& rid) {
    if (!ensureIndexLoaded(index_name)) return false;
    IndexInstance* inst = findInstance(index_name);
    std::lock_guard<std::mutex> ig(inst->mtx);
    inst->tree->insert(key, rid);
    return _updateRootIfNeeded(inst);
}

bool IndexManager::DeleteEntry(const std::string& index_name, int key, const RID& rid) {
    if (!ensureIndexLoaded(index_name)) return false;
    IndexInstance* inst = findInstance(index_name);
    std::lock_guard<std::mutex> ig(inst->mtx);
    inst->tree->remove(key, rid);
    return _updateRootIfNeeded(inst);
}

bool IndexManager::DropIndex(const std::string& index_name) {
    std::lock_guard<std::mutex> g(global_mtx_);
    auto all = catalog_.ListIndexes(db_name_);
    auto it = std::find_if(all.begin(), all.end(),
                           [&](const IndexInfo& info) { return info.index_name == index_name; });
    if (it == all.end()) return false;

    IndexInfo found = *it;
    if (!ensureIndexLoaded(index_name)) {
        int file_id = fm_.openFile(index_name + ".idx");
        BPlusTree tmp(fm_, file_id, 4, static_cast<int>(found.root_page_id));
        tmp.destroyIndex(fm_, file_id, static_cast<int>(found.root_page_id));
    } else {
        auto instIt = instances_.find(index_name);
        if (instIt != instances_.end()) {
            IndexInstance& inst = instIt->second;
            std::lock_guard<std::mutex> ig(inst.mtx);
            if (inst.tree) inst.tree->destroyIndex(fm_, inst.meta.file_id, static_cast<int>(inst.meta.root_page_id));
            inst.tree.reset();
            instances_.erase(instIt);
        }
    }

    return catalog_.DropIndex(db_name_, index_name);
}

bool IndexManager::OpenIndex(const IndexInfo& info) {
    std::lock_guard<std::mutex> g(global_mtx_);
    if (instances_.count(info.index_name)) return true;
    IndexInstance inst;
    inst.meta = info;
    int file_id = fm_.openFile(info.index_name + ".idx");
    inst.meta.file_id = file_id;
    inst.tree = std::make_unique<BPlusTree>(fm_, file_id, inst.meta.degree, static_cast<int>(inst.meta.root_page_id));
    instances_.emplace(info.index_name, std::move(inst));
    return true;
}

bool IndexManager::CloseIndex(const std::string& index_name) {
    std::lock_guard<std::mutex> g(global_mtx_);
    auto it = instances_.find(index_name);
    if (it == instances_.end()) return true;
    std::lock_guard<std::mutex> ig(it->second.mtx);
    if (it->second.tree) {
        _updateRootIfNeeded(&it->second);
        it->second.tree.reset();
    }
    instances_.erase(it);
    return true;
}


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

IndexManager::IndexInstance* IndexManager::findInstance(const std::string& index_name) {
    auto it = instances_.find(index_name);
    if (it == instances_.end()) return nullptr;
    return &(it->second);
}

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

bool IndexManager::DropIndexesOnTable(const std::string& table_name) {
    std::lock_guard<std::mutex> g(global_mtx_);
    auto idxs = catalog_.ListIndexes(db_name_, table_name);
    for (auto& info : idxs) {
        DropIndex(info.index_name);
    }
    return true;
}
*/
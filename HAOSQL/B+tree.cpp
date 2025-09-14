// B+tree.cpp  — 已适配 PageHeader / Page 结构
#include "B+tree.h"
#include <iostream>
#include <algorithm>
#include <cstring>

using namespace std;

// ================= 构造函数 =================
BPlusTree::BPlusTree(FileManager& fm, int file_id, int degree, int root_pid)
    : fm(fm), file_id(file_id), degree(degree), root_pid(root_pid) {}



// 工厂函数：构造 Page 并显式赋 id
Page BPlusTree::initialPage(PageType type, int pid) {
    Page page(type, pid); // 调用原构造函数
    page.id = pid;        // 显式赋值
    return page;
}

// ==================== 序列化 ====================
void BPlusTree::serializeNode(const BPlusTreeNode& node, Page& page) {
    // 清页数据
    std::memset(page.data, 0, sizeof(page.data));

    // 准备并写入 PageHeader（把头部放在 page.data 开头）
    PageHeader ph;
    ph.type = node.isLeaf ? DATA_PAGE : INDEX_PAGE;
    ph.page_id = page.id;
    ph.free_offset = sizeof(PageHeader); // 我们把负载写在 header 之后
    ph.slot_count = 0; // B+Tree 节点不使用 slot 机制（可扩展）
    std::memcpy(page.data, &ph, sizeof(PageHeader));

    // buf 指向 header 之后，开始写序列化的 node 内容
    char* buf = page.data + sizeof(PageHeader);

    // 写基础头信息
    std::memcpy(buf, &node.isLeaf, sizeof(bool)); buf += sizeof(bool);
    std::memcpy(buf, &node.keyCount, sizeof(int)); buf += sizeof(int);
    std::memcpy(buf, &node.page_id, sizeof(int)); buf += sizeof(int);

    // 写 keys
    for (int k : node.keys) {
        std::memcpy(buf, &k, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        // 叶子：对于每个 key 写入 RID 列表长度 + RID 列表内容
        for (int i = 0; i < node.keyCount; ++i) {
            uint16_t cnt = 0;
            if (i < (int)node.ridLists.size()) cnt = static_cast<uint16_t>(node.ridLists[i].size());
            std::memcpy(buf, &cnt, sizeof(uint16_t)); buf += sizeof(uint16_t);

            for (uint16_t j = 0; j < cnt; ++j) {
                const RID& r = node.ridLists[i][j];
                std::memcpy(buf, &r, sizeof(RID)); buf += sizeof(RID);
            }
        }
        // 写 nextLeaf
        std::memcpy(buf, &node.nextLeaf, sizeof(int)); buf += sizeof(int);
    }
    else {
        // 内部节点：写 children（child count = keyCount + 1）
        for (int c : node.children) {
            std::memcpy(buf, &c, sizeof(int)); buf += sizeof(int);
        }
    }

    // （可选）如果你希望 header.free_offset 反映有效载荷末尾，可以计算并更新它
    // size_t used = (buf - (page.data + sizeof(PageHeader)));
    // ph.free_offset = sizeof(PageHeader) + used;
    // std::memcpy(page.data, &ph, sizeof(PageHeader));
}

// ==================== 反序列化 ====================
BPlusTreeNode BPlusTree::deserializeNode(const Page& page) {
    BPlusTreeNode node;
    // 读取页头（如果将来需要校验类型可以用）
    PageHeader ph;
    std::memcpy(&ph, page.data, sizeof(PageHeader));

    const char* buf = page.data + sizeof(PageHeader);

    // 读基础头信息
    std::memcpy(&node.isLeaf, buf, sizeof(bool)); buf += sizeof(bool);
    std::memcpy(&node.keyCount, buf, sizeof(int)); buf += sizeof(int);
    std::memcpy(&node.page_id, buf, sizeof(int)); buf += sizeof(int);

    // 读 keys
    node.keys.resize(node.keyCount);
    for (int i = 0; i < node.keyCount; ++i) {
        std::memcpy(&node.keys[i], buf, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        // 叶子：对每个 key 先读 rid_count，再读对应数量的 RID
        node.ridLists.clear();
        node.ridLists.resize(node.keyCount);
        for (int i = 0; i < node.keyCount; ++i) {
            uint16_t cnt = 0;
            std::memcpy(&cnt, buf, sizeof(uint16_t)); buf += sizeof(uint16_t);
            node.ridLists[i].reserve(cnt);
            for (uint16_t j = 0; j < cnt; ++j) {
                RID r;
                std::memcpy(&r, buf, sizeof(RID)); buf += sizeof(RID);
                node.ridLists[i].push_back(r);
            }
        }
        // 读 nextLeaf
        std::memcpy(&node.nextLeaf, buf, sizeof(int)); buf += sizeof(int);
    }
    else {
        // 内部节点：读 children（keyCount+1）
        node.children.resize(node.keyCount + 1);
        for (int i = 0; i < node.keyCount + 1; ++i) {
            std::memcpy(&node.children[i], buf, sizeof(int)); buf += sizeof(int);
        }
    }

    return node;
}

// ==================== 精确搜索 ====================
std::vector<RID> BPlusTree::search(int key) {
    std::vector<RID> results;
    if (root_pid == -1) return results;  // 空树

    // 从根开始
    Page page = initialPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return results;
    BPlusTreeNode node = deserializeNode(page);

    // 一路往下走到叶子
    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) ++i;
        int childPid = node.children[i];
        Page childPage = initialPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return results;
        node = deserializeNode(childPage);
    }

    // 在当前叶子中查找等于 key 的位置，并收集对应的 ridLists
    for (int i = 0; i < node.keyCount; ++i) {
        if (node.keys[i] == key) {
            for (const RID& r : node.ridLists[i]) results.push_back(r);
        }
        else if (node.keys[i] > key) {
            return results;
        }
    }

    // 继续沿叶子链向右查找可能分布在后续叶子的相同 key
    int nextPid = node.nextLeaf;
    while (nextPid != -1) {
        Page nextPage = initialPage(DATA_PAGE, nextPid);
        if (!fm.readPage(file_id, nextPid, nextPage)) break;
        node = deserializeNode(nextPage);

        for (int i = 0; i < node.keyCount; ++i) {
            if (node.keys[i] == key) {
                for (const RID& r : node.ridLists[i]) results.push_back(r);
            }
            else if (node.keys[i] > key) {
                return results;
            }
        }
        nextPid = node.nextLeaf;
    }

    return results;
}

// ==================== 范围搜索 [keyLow, keyHigh] ====================
std::vector<RID> BPlusTree::searchRange(int keyLow, int keyHigh) {
    std::vector<RID> results;
    if (root_pid == -1) return results; // 空树
    if (keyLow > keyHigh) return results;

    // 找到包含 keyLow 的叶子（或第一个 >= keyLow 的叶子）
    Page page = initialPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return results;
    BPlusTreeNode node = deserializeNode(page);

    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && keyLow >= node.keys[i]) ++i;
        int childPid = node.children[i];
        Page childPage = initialPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return results;
        node = deserializeNode(childPage);
    }

    // 从该叶子开始向右遍历叶子链，收集范围内的所有 RID
    int curPid = node.page_id;
    while (curPid != -1) {
        Page curPage = initialPage(DATA_PAGE, curPid);
        if (!fm.readPage(file_id, curPid, curPage)) break;
        node = deserializeNode(curPage);

        for (int i = 0; i < node.keyCount; ++i) {
            int k = node.keys[i];
            if (k < keyLow) continue;
            if (k > keyHigh) return results;
            for (const RID& r : node.ridLists[i]) results.push_back(r);
        }
        curPid = node.nextLeaf;
    }

    return results;
}


// ---------- IO 封装 ----------
BPlusTreeNode BPlusTree::loadNode(int pid) {
    Page p = initialPage(DATA_PAGE, pid);
    if (!fm.readPage(file_id, pid, p)) {
        // 如果读失败，返回空节点（page_id 设置为 pid 以便写回）
        BPlusTreeNode empty;
        empty.page_id = pid;
        return empty;
    }
    return deserializeNode(p);
}

void BPlusTree::saveNode(const BPlusTreeNode& node) {
    Page p = initialPage(node.isLeaf ? DATA_PAGE : INDEX_PAGE, node.page_id);
    serializeNode(node, p);
    fm.writePage(file_id, p);
}

// ---------- 辅助 ----------
int BPlusTree::minKeysLeaf() const {
    return degree / 2;
}
int BPlusTree::minKeysInternal() const {
    int minChildren = (degree + 1) / 2;
    return minChildren - 1;
}
int BPlusTree::findChildIndex(const BPlusTreeNode& parent, int childPid) const {
    for (int i = 0; i < (int)parent.children.size(); ++i)
        if (parent.children[i] == childPid) return i;
    return -1;
}


// ================= 插入 =================
// ================== 插入入口 ==================
void BPlusTree::insert(int key, const RID& rid) {
    std::lock_guard<std::mutex> lock(treeMutex);

    if (fm.getPageCount(file_id) == 0) {
        BPlusTreeNode root;
        root.isLeaf = true;
        root.page_id = 0;
        root.keys = { key };
        root.ridLists = { std::vector<RID>{rid} };
        root.keyCount = 1;
        root.nextLeaf = -1;

        Page rootPage = initialPage(DATA_PAGE, 0);
        serializeNode(root, rootPage);
        fm.writePage(file_id, rootPage);
        root_pid = 0;
        return;
    }

    int newChildPid = -1;
    int upKey = -1;
    bool hasSplit = insertRecursive(root_pid, key, rid, upKey, newChildPid);

    if (hasSplit) {
        int newRootPid = fm.getPageCount(file_id);

        BPlusTreeNode newRoot;
        newRoot.isLeaf = false;
        newRoot.page_id = newRootPid;
        newRoot.keys = { upKey };
        newRoot.children = { root_pid, newChildPid };
        newRoot.keyCount = 1;

        Page rootPage = initialPage(INDEX_PAGE, newRootPid);
        serializeNode(newRoot, rootPage);
        fm.writePage(file_id, rootPage);

        root_pid = newRootPid;
    }
}

// ================== 递归插入 ==================
bool BPlusTree::insertRecursive(int nodePid, int key, const RID& rid, int& upKey, int& newChildPid) {
    cout << "insertRecursive nodePid=" << nodePid
        << " key=" << key << endl;
    Page page = initialPage(DATA_PAGE, nodePid);
    fm.readPage(file_id, nodePid, page);
    BPlusTreeNode node = deserializeNode(page);

    if (node.isLeaf) {
        auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        int idx = it - node.keys.begin();

        if (it != node.keys.end() && *it == key) {
            node.ridLists[idx].push_back(rid);
            Page p = initialPage(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false;
        }

        node.keys.insert(it, key);
        node.ridLists.insert(node.ridLists.begin() + idx, std::vector<RID>{rid});
        node.keyCount = static_cast<int>(node.keys.size());

        if (node.keyCount < degree) {
            Page p = initialPage(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false;
        }

        int mid = node.keyCount / 2;
        int newPageId = fm.getPageCount(file_id);

        BPlusTreeNode newLeaf;
        newLeaf.isLeaf = true;
        newLeaf.page_id = newPageId;
        newLeaf.keys.assign(node.keys.begin() + mid, node.keys.end());
        newLeaf.ridLists.assign(node.ridLists.begin() + mid, node.ridLists.end());
        newLeaf.keyCount = static_cast<int>(newLeaf.keys.size());
        newLeaf.nextLeaf = node.nextLeaf;

        node.keys.resize(mid);
        node.ridLists.resize(mid);
        node.keyCount = static_cast<int>(node.keys.size());
        node.nextLeaf = newPageId;

        Page oldPage = initialPage(DATA_PAGE, node.page_id);
        serializeNode(node, oldPage);
        fm.writePage(file_id, oldPage);

        Page newPage = initialPage(DATA_PAGE, newLeaf.page_id);
        serializeNode(newLeaf, newPage);
        fm.writePage(file_id, newPage);

        upKey = newLeaf.keys[0];
        newChildPid = newLeaf.page_id;
        return true;
    }
    else {
        int i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) ++i;

        int childPid = node.children[i];
        int childUpKey = -1, childNewPid = -1;
        cout << "Recursing to childPid=" << childPid << endl;
        bool childSplit = insertRecursive(childPid, key, rid, childUpKey, childNewPid);

        if (!childSplit) {
            return false;
        }

        node.keys.insert(node.keys.begin() + i, childUpKey);
        node.children.insert(node.children.begin() + i + 1, childNewPid);
        node.keyCount = static_cast<int>(node.keys.size());

        if (node.keyCount < degree) {
            Page p = initialPage(INDEX_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false;
        }

        int mid = node.keyCount / 2;
        upKey = node.keys[mid];

        int newPageId = fm.getPageCount(file_id);
        BPlusTreeNode newInternal;
        newInternal.isLeaf = false;
        newInternal.page_id = newPageId;

        newInternal.keys.assign(node.keys.begin() + mid + 1, node.keys.end());
        newInternal.children.assign(node.children.begin() + mid + 1, node.children.end());
        newInternal.keyCount = static_cast<int>(newInternal.keys.size());

        node.keys.resize(mid);
        node.children.resize(mid + 1);
        node.keyCount = static_cast<int>(node.keys.size());

        Page oldPage = initialPage(INDEX_PAGE, node.page_id);
        serializeNode(node, oldPage);
        fm.writePage(file_id, oldPage);

        Page newPage = initialPage(INDEX_PAGE, newInternal.page_id);
        serializeNode(newInternal, newPage);
        fm.writePage(file_id, newPage);

        newChildPid = newInternal.page_id;
        return true;
    }
}



// === 删除入口 ===
void BPlusTree::remove(int key, const RID& rid) {
    std::lock_guard<std::mutex> lock(treeMutex);

    if (root_pid == -1) return; // 空树

    deleteEntry(root_pid, -1, -1, key, rid);

    Page rootPage = initialPage(DATA_PAGE, root_pid);
    fm.readPage(file_id, root_pid, rootPage);
    BPlusTreeNode root = deserializeNode(rootPage);

    if (!root.isLeaf && root.keyCount == 0) {
        int newRootPid = root.children[0];
        root_pid = newRootPid;
    }
}


// deleteEntry: 在 node_pid 子树中删除 (key, rid)
void BPlusTree::deleteEntry(int node_pid, int parent_pid, int childIndex, int key, const RID& rid) {
    BPlusTreeNode node = loadNode(node_pid);

    if (node.isLeaf) {
        auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        if (it == node.keys.end() || *it != key) {
            return;
        }
        int idx = it - node.keys.begin();

        auto& ridList = node.ridLists[idx];
        auto ridIt = std::find(ridList.begin(), ridList.end(), rid);
        if (ridIt == ridList.end()) {
            return;
        }
        ridList.erase(ridIt);

        if (ridList.empty()) {
            node.keys.erase(node.keys.begin() + idx);
            node.ridLists.erase(node.ridLists.begin() + idx);
            node.keyCount = (int)node.keys.size();
        }

        saveNode(node);

        if (node.page_id == root_pid) return;

        if (node.keyCount < minKeysLeaf()) {
            if (parent_pid >= 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                handleLeafUnderflow(node, parent, childIndex);
                saveNode(parent);
            }
        }
        else {
            if (parent_pid >= 0 && childIndex > 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                parent.keys[childIndex - 1] = node.keys.front();
                saveNode(parent);
            }
        }
    }
    else {
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) ++i;
        int childPid = node.children[i];

        deleteEntry(childPid, node_pid, i, key, rid);

        node = loadNode(node_pid);
        if (node.page_id != root_pid && node.keyCount < minKeysInternal()) {
            if (parent_pid >= 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                handleInternalUnderflow(node, parent, childIndex);
                saveNode(parent);
            }
        }
    }
}

// 叶子下溢处理：child 是被下溢的叶子节点，parent 已经 load，parentIndex 是 child 在 parent.children 中的索引
void BPlusTree::handleLeafUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex) {
    int idx = parentIndex; // child 在 parent.children[idx]

    // 尝试左兄弟
    if (idx > 0) {
        BPlusTreeNode left = loadNode(parent.children[idx - 1]);
        if (borrowFromLeftLeaf(left, child, parent, idx - 1)) {
            saveNode(left); saveNode(child); saveNode(parent);
            return;
        }
        // 否则合并 left + child
        mergeLeafs(left, child, parent, idx - 1);
        saveNode(left); saveNode(parent);
        return;
    }

    // 尝试右兄弟
    if (idx + 1 < (int)parent.children.size()) {
        BPlusTreeNode right = loadNode(parent.children[idx + 1]);
        if (borrowFromRightLeaf(child, right, parent, idx)) {
            saveNode(right); saveNode(child); saveNode(parent);
            return;
        }
        // 合并 child + right
        mergeLeafs(child, right, parent, idx);
        saveNode(child); saveNode(parent);
        return;
    }
    // 如果没有兄弟（理论上不应发生），直接返回
}

// 内部节点下溢处理：child 是被下溢的内部节点，parent 已 load，parentIndex 为 child 在 parent.children 的索引
void BPlusTree::handleInternalUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex) {
    int idx = parentIndex;

    // 尝试左兄弟
    if (idx > 0) {
        BPlusTreeNode left = loadNode(parent.children[idx - 1]);
        if (borrowFromLeftInternal(left, child, parent, idx - 1)) {
            saveNode(left); saveNode(child); saveNode(parent);
            return;
        }
        // 合并 left + child
        mergeInternals(left, child, parent, idx - 1);
        saveNode(left); saveNode(parent);
        return;
    }

    // 尝试右兄弟
    if (idx + 1 < (int)parent.children.size()) {
        BPlusTreeNode right = loadNode(parent.children[idx + 1]);
        if (borrowFromRightInternal(child, right, parent, idx)) {
            saveNode(right); saveNode(child); saveNode(parent);
            return;
        }
        // 合并 child + right
        mergeInternals(child, right, parent, idx);
        saveNode(child); saveNode(parent);
        return;
    }
}

// ========== 借 / 合并 叶子 ==========

// 从左借（left 是左兄弟，right 是当前 child； parentIndex 指 left 在 parent.children 的索引）
bool BPlusTree::borrowFromLeftLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (left.keyCount <= minKeysLeaf()) return false;

    int movedKey = left.keys.back();
    auto movedList = left.ridLists.back();

    left.keys.pop_back();
    left.ridLists.pop_back();
    left.keyCount--;

    right.keys.insert(right.keys.begin(), movedKey);
    right.ridLists.insert(right.ridLists.begin(), movedList);
    right.keyCount++;

    parent.keys[parentIndex] = right.keys.front();
    return true;
}

// 从右借（left 是当前 child，right 是右兄弟； parentIndex 是 left 在 parent.children 的索引）
bool BPlusTree::borrowFromRightLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (right.keyCount <= minKeysLeaf()) return false;

    int movedKey = right.keys.front();
    auto movedList = right.ridLists.front();

    right.keys.erase(right.keys.begin());
    right.ridLists.erase(right.ridLists.begin());
    right.keyCount--;

    left.keys.push_back(movedKey);
    left.ridLists.push_back(movedList);
    left.keyCount++;

    if (!right.keys.empty()) {
        parent.keys[parentIndex] = right.keys.front();
    }
    return true;
}

// 合并右到左：left <- left + right；parentIndex 指 left 在 parent.children 的索引
void BPlusTree::mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
    left.ridLists.insert(left.ridLists.end(), right.ridLists.begin(), right.ridLists.end());
    left.keyCount = (int)left.keys.size();

    left.nextLeaf = right.nextLeaf;

    parent.keys.erase(parent.keys.begin() + parentIndex);
    parent.children.erase(parent.children.begin() + parentIndex + 1);
    parent.keyCount = (int)parent.keys.size();
}

// ========== 借 / 合并 内部节点 ==========

bool BPlusTree::borrowFromLeftInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (left.keyCount <= minKeysInternal()) return false;

    int leftLastKey = left.keys.back();
    int leftLastChild = left.children.back();
    left.keys.pop_back(); left.children.pop_back(); left.keyCount--;

    int sep = parent.keys[parentIndex];
    right.keys.insert(right.keys.begin(), sep);
    right.children.insert(right.children.begin(), leftLastChild);
    right.keyCount++;

    parent.keys[parentIndex] = leftLastKey;
    return true;
}

bool BPlusTree::borrowFromRightInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (right.keyCount <= minKeysInternal()) return false;

    int sep = parent.keys[parentIndex];
    left.keys.push_back(sep);
    left.children.push_back(right.children.front());
    left.keyCount++;

    parent.keys[parentIndex] = right.keys.front();

    right.keys.erase(right.keys.begin());
    right.children.erase(right.children.begin());
    right.keyCount--;
    return true;
}

// 合并内部节点： left <- left + sep + right
void BPlusTree::mergeInternals(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    int sep = parent.keys[parentIndex];
    left.keys.push_back(sep);
    left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
    left.children.insert(left.children.end(), right.children.begin(), right.children.end());
    left.keyCount = (int)left.keys.size();

    parent.keys.erase(parent.keys.begin() + parentIndex);
    parent.children.erase(parent.children.begin() + parentIndex + 1);
    parent.keyCount = (int)parent.keys.size();
}



// ================= 打印树 =================
void BPlusTree::printTree() {
    Page rootPage = initialPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, rootPage)) {
        std::cerr << "无法读取根节点 page " << root_pid << std::endl;
        return;
    }
    BPlusTreeNode root = deserializeNode(rootPage);

    std::cout << "=== B+Tree Structure ===" << std::endl;
    printNode(root, 0);
    std::cout << "========================" << std::endl;
}

void BPlusTree::printNode(const BPlusTreeNode& node, int level) {
    for (int i = 0; i < level; i++) std::cout << "  ";

    if (node.isLeaf) {
        std::cout << "[Leaf p" << node.page_id << "] keys: ";
        for (int i = 0; i < node.keyCount; i++) {
            std::cout << node.keys[i] << " (";
            for (size_t j = 0; j < node.ridLists[i].size(); j++) {
                const RID& rid = node.ridLists[i][j];
                std::cout << "RID=" << rid.page_id << "," << rid.slot_id;
                if (j + 1 < node.ridLists[i].size())
                    std::cout << "; ";
            }
            std::cout << ") ";
        }
        std::cout << " next=" << node.nextLeaf << std::endl;
    }
    else {
        std::cout << "[Internal p" << node.page_id << "] keys: ";
        for (int k : node.keys) std::cout << k << " ";
        std::cout << std::endl;

        for (int child_pid : node.children) {
            Page childPage = initialPage(DATA_PAGE, child_pid);
            if (fm.readPage(file_id, child_pid, childPage)) {
                BPlusTreeNode childNode = deserializeNode(childPage);
                printNode(childNode, level + 1);
            }
        }
    }
}

int BPlusTree::createIndex(FileManager& fm, int fileId) {
    std::lock_guard<std::mutex> lock(treeMutex);

    // 分配根节点页
    int rootPid = fm.allocatePage(fileId, INDEX_PAGE);

    // 初始化根节点（叶子节点）
    BPlusTreeNode root;
    root.isLeaf = true;
    root.page_id = rootPid;
    root.keyCount = 0;
    root.nextLeaf = -1;

    // 将根节点序列化写入页
    Page rootPage = initialPage(INDEX_PAGE, rootPid);
    rootPage.id = rootPid;
    serializeNode(root, rootPage);
    fm.writePage(fileId, rootPage);

    std::cout << "Index created in file_id=" << fileId
        << " root page=" << rootPid << std::endl;

    return rootPid;
}

// B+树索引销毁（释放所有占用页）
void BPlusTree::destroyIndex(FileManager& fm, int root_pid, int file_id) {
    std::lock_guard<std::mutex> lock(treeMutex);

    if (root_pid < 0) return;

    Page page = initialPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return;

    BPlusTreeNode node = deserializeNode(page);

    if (node.isLeaf) {
        fm.freePage(file_id, node.page_id);
    }
    else {
        for (int child_pid : node.children) {
            destroyIndex(fm, child_pid, file_id);
        }
        fm.freePage(file_id, node.page_id);
    }
}

void BPlusTree::setRootPid(int rootid) {
    this->root_pid = rootid;
}

int BPlusTree::getRootPid() {
    return this->root_pid;
}

/*int main() {
    FileManager fm("./");
    int file_id = fm.openFile("students_idx");

    BPlusTree tree(fm, file_id, 4, 0); // degree=4
    tree.printTree();

    //tree.insert(11, { 1, 1 });
    //tree.insert(22, { 1, 2 });
    //tree.insert(33, { 1, 3 });
    //tree.insert(44, { 1, 4 });
    //tree.insert(55, { 1, 5 });
    //tree.insert(66, { 1, 6 });

    tree.printTree();

    //cout << "删除 20...\n";
    //tree.remove(20);
    //tree.printTree();

    //cout << "删除 70...\n";
    //tree.remove(70);
    //tree.printTree();
    return 0;
}*/

/*int main() {
    std::string indexFileName = "students";

    // ---------------- 创建文件管理器 ----------------
    FileManager fm("./");                 // 数据库目录
    int file_id = fm.openFile(indexFileName);

    // ---------------- 初始化 B+Tree ----------------
    BPlusTree tree(fm, file_id, 4 , 0);      // degree = 4
    int root_pid = tree.createIndex(fm, file_id);  // 返回根页号
    std::cout << "Index created. Root page: " << root_pid << std::endl;
    tree.setRootPid(root_pid);           // 设置刚创建的根页
    tree.printTree();

    // ---------------- 插入测试 ----------------
    std::cout << "\n--- Inserting keys ---\n";
    tree.insert(10, { 1,1 });
    tree.insert(20, { 1,2 });
    tree.insert(20, { 1,3 });
    tree.insert(30, { 2,1 });
    tree.insert(30, { 2,3 });
    tree.insert(40, { 2,2 });
    tree.insert(50, { 3,1 });
    tree.insert(60, { 3,2 });
    tree.insert(70, { 4,1 });
    tree.insert(80, { 4,2 });
    tree.printTree();

    // ---------------- 删除测试 ----------------
    std::cout << "\n--- Removing keys ---\n";
    tree.remove(20, { 1,2 });
    tree.remove(40, { 2,2 });
    tree.remove(70, { 4,1 });
    tree.printTree();

    // ---------------- 查询测试 ----------------
    std::cout << "\n--- Searching keys ---\n";
    int keysToSearch[] = { 10, 20, 30, 40, 50 };

    for (int key : keysToSearch) {
        std::vector<RID> results;
        results = tree.search(key);
        if (!results.empty() ){
            std::cout << "Found key " << key << " -> ";
            for (const auto& r : results) {
                std::cout << "(RID=" << r.page_id << "," << r.slot_id << ") ";
            }
            std::cout << std::endl;
        }
        else {
            std::cout << "Key " << key << " not found.\n";
        }
    }

    // ---------------- 销毁索引 ----------------
    std::cout << "\n--- Destroying index ---\n";
    tree.destroyIndex(fm, file_id, root_pid);  // 递归释放所有节点
    std::cout << "Index destroyed.\n";

    return 0;
}*/


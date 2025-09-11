#include "B+tree.h"
#include <iostream>
#include <algorithm>
#include <cstring>

// ================= 构造函数 =================
BPlusTree::BPlusTree(FileManager& fm, int file_id, int degree, int root_pid)
    : fm(fm), file_id(file_id), degree(degree), root_pid(root_pid) {}

// ================= 序列化 =================
void BPlusTree::serializeNode(const BPlusTreeNode& node, Page& page) {
    char* buf = page.data;
    memcpy(buf, &node.isLeaf, sizeof(bool)); buf += sizeof(bool);
    memcpy(buf, &node.keyCount, sizeof(int)); buf += sizeof(int);
    memcpy(buf, &node.page_id, sizeof(int)); buf += sizeof(int);

    for (int k : node.keys) {
        memcpy(buf, &k, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        for (const RID& rid : node.rids) {
            memcpy(buf, &rid, sizeof(RID)); buf += sizeof(RID);
        }
        memcpy(buf, &node.nextLeaf, sizeof(int)); buf += sizeof(int);
    }
    else {
        for (int c : node.children) {
            memcpy(buf, &c, sizeof(int)); buf += sizeof(int);
        }
    }
}

// ================= 反序列化 =================
BPlusTreeNode BPlusTree::deserializeNode(const Page& page) {
    BPlusTreeNode node;
    const char* buf = page.data;

    memcpy(&node.isLeaf, buf, sizeof(bool)); buf += sizeof(bool);
    memcpy(&node.keyCount, buf, sizeof(int)); buf += sizeof(int);
    memcpy(&node.page_id, buf, sizeof(int)); buf += sizeof(int);

    node.keys.resize(node.keyCount);
    for (int i = 0; i < node.keyCount; i++) {
        memcpy(&node.keys[i], buf, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        node.rids.resize(node.keyCount);
        for (int i = 0; i < node.keyCount; i++) {
            memcpy(&node.rids[i], buf, sizeof(RID)); buf += sizeof(RID);
        }
        memcpy(&node.nextLeaf, buf, sizeof(int)); buf += sizeof(int);
    }
    else {
        node.children.resize(node.keyCount + 1);
        for (int i = 0; i < node.keyCount + 1; i++) {
            memcpy(&node.children[i], buf, sizeof(int)); buf += sizeof(int);
        }
    }

    return node;
}

// ================= 搜索 =================
bool BPlusTree::search(int key, RID& rid) {
    if (root_pid == -1) return false;  // 空树

    // 从根开始
    Page rootPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, rootPage)) return false;

    BPlusTreeNode node = deserializeNode(rootPage);

    // 一路往下走
    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) {
            i++;
        }
        int childPid = node.children[i];

        Page childPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return false;
        node = deserializeNode(childPage);
    }

    // 在叶子查找 key
    for (int i = 0; i < node.keyCount; i++) {
        if (node.keys[i] == key) {
            rid = node.rids[i];
            return true;
        }
    }
    return false;
}

std::vector<RID> BPlusTree::searchRange(int keyLow, int keyHigh) {
    std::vector<RID> results;
    if (root_pid == -1) return results; // 空树

    // 先找到 keyLow 所在的叶子
    Page rootPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, rootPage)) return results;

    BPlusTreeNode node = deserializeNode(rootPage);

    // 一路往下找到叶子
    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && keyLow >= node.keys[i]) {
            i++;
        }
        int childPid = node.children[i];
        Page childPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return results;
        node = deserializeNode(childPage);
    }

    // 遍历叶子链表，收集 [keyLow, keyHigh] 范围的键
    while (true) {
        for (int i = 0; i < node.keyCount; i++) {
            int k = node.keys[i];
            if (k >= keyLow && k <= keyHigh) {
                results.push_back(node.rids[i]);
            }
            else if (k > keyHigh) {
                return results; // 提前结束
            }
        }
        if (node.nextLeaf == -1) break; // 到了最后一个叶子
        Page nextPage(DATA_PAGE, node.nextLeaf);
        if (!fm.readPage(file_id, node.nextLeaf, nextPage)) break;
        node = deserializeNode(nextPage);
    }

    return results;
}


// ---------- IO 封装 ----------
BPlusTreeNode BPlusTree::loadNode(int pid) {
    Page p(DATA_PAGE, pid);
    if (!fm.readPage(file_id, pid, p)) {
        // 如果读失败，返回空节点（page_id 设置为 pid 以便写回）
        BPlusTreeNode empty;
        empty.page_id = pid;
        return empty;
    }
    return deserializeNode(p);
}

void BPlusTree::saveNode(const BPlusTreeNode& node) {
    Page p(DATA_PAGE, node.page_id);
    serializeNode(node, p);
    fm.writePage(file_id, p);
}

// ---------- 辅助 ----------
int BPlusTree::minKeysLeaf() const {
    // leaf 最大 key = degree - 1
    // 通常最小 = ceil((degree-1)/2) => 简化取 (degree-1+1)/2 = degree/2
    return degree / 2;
}
int BPlusTree::minKeysInternal() const {
    // internal 最小 keys = ceil(degree/2) - 1
    int minChildren = (degree + 1) / 2;
    return minChildren - 1;
}
int BPlusTree::findChildIndex(const BPlusTreeNode& parent, int childPid) const {
    for (int i = 0; i < (int)parent.children.size(); ++i) if (parent.children[i] == childPid) return i;
    return -1;
}

// ================= 插入 =================
// ================== 插入入口 ==================
void BPlusTree::insert(int key, const RID& rid) {
    if (fm.getPageCount(file_id) == 0) {
        // 空树，新建根叶子
        BPlusTreeNode root;
        root.isLeaf = true;
        root.page_id = 0;
        root.keys = { key };
        root.rids = { rid };
        root.keyCount = 1;
        root.nextLeaf = -1;

        Page rootPage(DATA_PAGE, 0);
        serializeNode(root, rootPage);
        fm.writePage(file_id, rootPage);
        root_pid = 0;
        return;
    }

    // 递归插入
    int newChildPid = -1;
    int upKey = -1;
    bool hasSplit = insertRecursive(root_pid, key, rid, upKey, newChildPid);

    if (hasSplit) {
        // 根被分裂，新建根
        int newRootPid = fm.getPageCount(file_id);

        BPlusTreeNode newRoot;
        newRoot.isLeaf = false;
        newRoot.page_id = newRootPid;
        newRoot.keys = { upKey };
        newRoot.children = { root_pid, newChildPid };
        newRoot.keyCount = 1;

        Page rootPage(DATA_PAGE, newRootPid);
        serializeNode(newRoot, rootPage);
        fm.writePage(file_id, rootPage);

        root_pid = newRootPid;
    }
}

// ================== 递归插入 ==================
bool BPlusTree::insertRecursive(int nodePid, int key, const RID& rid, int& upKey, int& newChildPid) {
    // 读当前节点
    Page page(DATA_PAGE, nodePid);
    fm.readPage(file_id, nodePid, page);
    BPlusTreeNode node = deserializeNode(page);

    if (node.isLeaf) {
        // -------- 叶子节点插入 --------
        auto it = lower_bound(node.keys.begin(), node.keys.end(), key);
        int idx = it - node.keys.begin();
        node.keys.insert(it, key);
        node.rids.insert(node.rids.begin() + idx, rid);
        node.keyCount++;

        if (node.keyCount < degree) {
            // 没超出容量，直接写回
            Page p(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false; // 没分裂
        }

        // 分裂
        int mid = node.keyCount / 2;
        int newPageId = fm.getPageCount(file_id);

        BPlusTreeNode newLeaf;
        newLeaf.isLeaf = true;
        newLeaf.page_id = newPageId;
        newLeaf.keys.assign(node.keys.begin() + mid, node.keys.end());
        newLeaf.rids.assign(node.rids.begin() + mid, node.rids.end());
        newLeaf.keyCount = newLeaf.keys.size();
        newLeaf.nextLeaf = node.nextLeaf;

        node.keys.resize(mid);
        node.rids.resize(mid);
        node.keyCount = mid;
        node.nextLeaf = newPageId;

        // 写回
        Page oldPage(DATA_PAGE, node.page_id);
        serializeNode(node, oldPage);
        fm.writePage(file_id, oldPage);

        Page newPage(DATA_PAGE, newLeaf.page_id);
        serializeNode(newLeaf, newPage);
        fm.writePage(file_id, newPage);

        // 返回分裂信息
        upKey = newLeaf.keys[0];
        newChildPid = newLeaf.page_id;
        return true;
    }
    else {
        // -------- 内部节点 --------
        int i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) i++;

        int childPid = node.children[i];
        int childUpKey, childNewPid;
        bool childSplit = insertRecursive(childPid, key, rid, childUpKey, childNewPid);

        if (!childSplit) {
            return false;
        }

        // 子节点分裂，插入新的 key 和 child
        node.keys.insert(node.keys.begin() + i, childUpKey);
        node.children.insert(node.children.begin() + i + 1, childNewPid);
        node.keyCount++;

        if (node.keyCount < degree) {
            Page p(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false;
        }

        // 内部节点分裂
        int mid = node.keyCount / 2;
        upKey = node.keys[mid];

        int newPageId = fm.getPageCount(file_id);
        BPlusTreeNode newInternal;
        newInternal.isLeaf = false;
        newInternal.page_id = newPageId;

        // 右半部分
        newInternal.keys.assign(node.keys.begin() + mid + 1, node.keys.end());
        newInternal.children.assign(node.children.begin() + mid + 1, node.children.end());
        newInternal.keyCount = newInternal.keys.size();

        // 左半部分
        node.keys.resize(mid);
        node.children.resize(mid + 1);
        node.keyCount = node.keys.size();

        // 写回
        Page oldPage(DATA_PAGE, node.page_id);
        serializeNode(node, oldPage);
        fm.writePage(file_id, oldPage);

        Page newPage(DATA_PAGE, newInternal.page_id);
        serializeNode(newInternal, newPage);
        fm.writePage(file_id, newPage);

        newChildPid = newInternal.page_id;
        return true;
    }
}

// === 删除入口 ===
void BPlusTree::remove(int key) {
    if (root_pid == -1) return; // 空树

    // 从根开始递归删除，根的 parent_pid = -1
    deleteEntry(root_pid, -1, -1, key);

    // 如果根节点是内部节点且已经空了，需要更新 root_pid
    Page rootPage(DATA_PAGE, root_pid);
    fm.readPage(file_id, root_pid, rootPage);
    BPlusTreeNode root = deserializeNode(rootPage);

    if (!root.isLeaf && root.keyCount == 0) {
        // 根被删空 -> 提升唯一子节点为新根
        int newRootPid = root.children[0];
        root_pid = newRootPid;
    }
}


// deleteEntry: 在 node_pid 子树中删除 key； parent_pid/childIndex 表示父亲和 child 在父亲中的索引（root 的 parent_pid = -1）
void BPlusTree::deleteEntry(int node_pid, int parent_pid, int childIndex, int key) {
    BPlusTreeNode node = loadNode(node_pid);

    if (node.isLeaf) {
        // 删除 key（若存在）
        auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        if (it == node.keys.end() || *it != key) {
            // key 不存在，直接返回
            return;
        }
        int idx = it - node.keys.begin();
        node.keys.erase(node.keys.begin() + idx);
        node.rids.erase(node.rids.begin() + idx);
        node.keyCount = (int)node.keys.size();
        saveNode(node);

        // 若 node 是根则结束
        if (node.page_id == root_pid) return;

        // 检查下溢
        if (node.keyCount < minKeysLeaf()) {
            // 需要处理父亲（parent 必须存在）
            if (parent_pid >= 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                handleLeafUnderflow(node, parent, childIndex);
                saveNode(parent);
            }
        }
        else {
            // 若不是 underflow，可能需要更新 parent 分隔键（childIndex > 0）
            if (parent_pid >= 0 && childIndex > 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                parent.keys[childIndex - 1] = node.keys.front();
                saveNode(parent);
            }
        }
    }
    else {
        // 内部节点：找到下钻 child
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) ++i;
        int childPid = node.children[i];
        // 递归删除
        deleteEntry(childPid, node_pid, i, key);

        // 读回当前 node（在子递归中可能被修改）
        node = loadNode(node_pid);

        // 内部节点可能丢失了一个 key（如果子合并）
        if (node.page_id == root_pid) {
            // special: root shrink handled after top-level deletion
        }
        else {
            if (node.keyCount < minKeysInternal()) {
                // 处理内部下溢
                if (parent_pid >= 0) {
                    BPlusTreeNode parent = loadNode(parent_pid);
                    handleInternalUnderflow(node, parent, childIndex);
                    saveNode(parent);
                }
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
    // move left.last -> right.front
    int movedKey = left.keys.back();
    RID movedRid = left.rids.back();
    left.keys.pop_back(); left.rids.pop_back(); left.keyCount--;

    right.keys.insert(right.keys.begin(), movedKey);
    right.rids.insert(right.rids.begin(), movedRid);
    right.keyCount++;

    // update parent separator (left is parent.children[parentIndex], right is parent.children[parentIndex+1])
    parent.keys[parentIndex] = right.keys.front();
    return true;
}

// 从右借（left 是当前 child， right 是右兄弟； parentIndex 是 left 在 parent.children 的索引）
bool BPlusTree::borrowFromRightLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (right.keyCount <= minKeysLeaf()) return false;
    // move right.front -> left.back
    int movedKey = right.keys.front();
    RID movedRid = right.rids.front();
    right.keys.erase(right.keys.begin()); right.rids.erase(right.rids.begin()); right.keyCount--;

    left.keys.push_back(movedKey);
    left.rids.push_back(movedRid);
    left.keyCount++;

    // update parent separator: parent.keys[parentIndex] should be right.keys.front()
    parent.keys[parentIndex] = right.keys.front();
    return true;
}

// 合并右到左：left <- left + right；parentIndex 指 left 在 parent.children 的索引
void BPlusTree::mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
    left.rids.insert(left.rids.end(), right.rids.begin(), right.rids.end());
    left.keyCount = (int)left.keys.size();
    left.nextLeaf = right.nextLeaf;

    // 修改 parent：删除 separator parent.keys[parentIndex]，删除 right child
    parent.keys.erase(parent.keys.begin() + parentIndex);
    parent.children.erase(parent.children.begin() + parentIndex + 1);
    parent.keyCount = (int)parent.keys.size();
}

// ========== 借 / 合并 内部节点 ==========

// 从左借（left 是左兄弟， right 是当前 child， parentIndex 指 left 在 parent.children）
bool BPlusTree::borrowFromLeftInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (left.keyCount <= minKeysInternal()) return false;

    // 移动 left 的最后 key 到 parent, parent 的 sep 下移到 right.front
    int leftLastKey = left.keys.back();
    int leftLastChild = left.children.back();
    left.keys.pop_back(); left.children.pop_back(); left.keyCount--;

    // parent.keys[parentIndex] 放到 right 的前面
    int sep = parent.keys[parentIndex];
    right.keys.insert(right.keys.begin(), sep);
    right.children.insert(right.children.begin(), leftLastChild);
    right.keyCount++;

    // parent 的 new sep 是 leftLastKey
    parent.keys[parentIndex] = leftLastKey;
    return true;
}

bool BPlusTree::borrowFromRightInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (right.keyCount <= minKeysInternal()) return false;

    int sep = parent.keys[parentIndex];
    // sep 下移到 left
    left.keys.push_back(sep);
    left.children.push_back(right.children.front());
    left.keyCount++;

    // parent 的 new sep 是 right.keys.front()
    parent.keys[parentIndex] = right.keys.front();

    // remove from right
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
    // 记得设置root_pid
    Page rootPage(DATA_PAGE, root_pid);
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
            std::cout << node.keys[i] << "(RID="
                << node.rids[i].page_id << ","
                << node.rids[i].slot_id << ") ";
        }
        std::cout << " next=" << node.nextLeaf << std::endl;
    }
    else {
        std::cout << "[Internal p" << node.page_id << "] keys: ";
        for (int k : node.keys) std::cout << k << " ";
        std::cout << std::endl;

        for (int child_pid : node.children) {
            Page childPage(DATA_PAGE, child_pid);
            if (fm.readPage(file_id, child_pid, childPage)) {
                BPlusTreeNode childNode = deserializeNode(childPage);
                printNode(childNode, level + 1);
            }
        }
    }
}

int BPlusTree::createIndex(FileManager& fm, int fileId) {
    // 分配根节点页
    int rootPid = fm.allocatePage(fileId, INDEX_PAGE);

    // 初始化根节点（叶子节点）
    BPlusTreeNode root;
    root.isLeaf = true;
    root.page_id = rootPid;
    root.keyCount = 0;
    root.nextLeaf = -1;

    // 将根节点序列化写入页
    Page rootPage(INDEX_PAGE, rootPid);
    serializeNode(root, rootPage);   // 使用已有 BPlusTree 序列化方法
    fm.writePage(fileId, rootPage);

    std::cout << "Index created in file_id=" << fileId
        << " root page=" << rootPid << std::endl;

    return rootPid;
}

// B+树索引销毁（释放所有占用页）
void BPlusTree::destroyIndex(FileManager& fm, int root_pid, int file_id) {
    if (root_pid < 0) return;

    // 读取当前节点
    Page page(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return;

    BPlusTreeNode node;
    const char* buf = page.data;

    memcpy(&node.isLeaf, buf, sizeof(bool)); buf += sizeof(bool);
    memcpy(&node.keyCount, buf, sizeof(int)); buf += sizeof(int);
    memcpy(&node.page_id, buf, sizeof(int)); buf += sizeof(int);

    node.keys.resize(node.keyCount);
    for (int i = 0; i < node.keyCount; i++) {
        memcpy(&node.keys[i], buf, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        // 叶子节点
        node.rids.resize(node.keyCount);
        for (int i = 0; i < node.keyCount; i++) {
            memcpy(&node.rids[i], buf, sizeof(RID)); buf += sizeof(RID);
        }
        memcpy(&node.nextLeaf, buf, sizeof(int)); buf += sizeof(int);

        // 释放叶子节点页
        fm.freePage(file_id, node.page_id);
    }
    else {
        // 内部节点
        node.children.resize(node.keyCount + 1);
        for (int i = 0; i < node.keyCount + 1; i++) {
            memcpy(&node.children[i], buf, sizeof(int)); buf += sizeof(int);
        }

        // 递归销毁子节点
        for (int child_pid : node.children) {
            destroyIndex(fm, child_pid, file_id);
        }

        // 释放内部节点页
        fm.freePage(file_id, node.page_id);
    }
}

void BPlusTree::setRootPid(int rootid) {
    this->root_pid = rootid;
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

int main() {
    std::string indexFileName = "students";

    // ---------------- 创建文件管理器 ----------------
    FileManager fm("./");                 // 数据库目录
    int file_id = fm.openFile(indexFileName);

    // ---------------- 初始化 B+Tree ----------------
    BPlusTree tree(fm, file_id, 5 , 0);      // degree = 4
    int root_pid = tree.createIndex(fm, file_id);  // 返回根页号
    std::cout << "Index created. Root page: " << root_pid << std::endl;
    tree.setRootPid(root_pid);           // 设置刚创建的根页
    tree.printTree();

    // ---------------- 插入测试 ----------------
    std::cout << "\n--- Inserting keys ---\n";
    tree.insert(10, { 1,1 });
    tree.insert(20, { 1,2 });
    tree.insert(30, { 2,1 });
    tree.insert(40, { 2,2 });
    tree.insert(50, { 3,1 });
    tree.insert(60, { 3,2 });
    tree.insert(70, { 4,1 });
    tree.insert(80, { 4,2 });
    tree.printTree();

    // ---------------- 删除测试 ----------------
    std::cout << "\n--- Removing keys ---\n";
    tree.remove(20);
    tree.remove(40);
    tree.remove(70);
    tree.printTree();

    // ---------------- 查询测试 ----------------
    std::cout << "\n--- Searching keys ---\n";
    RID rid;
    int keysToSearch[] = { 10,20,30,40,50 };
    for (int key : keysToSearch) {
        if (tree.search(key, rid)) {
            std::cout << "Found key " << key << " -> RID(" << rid.page_id << "," << rid.slot_id << ")\n";
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
}


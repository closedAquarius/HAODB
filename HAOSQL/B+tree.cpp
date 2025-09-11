#include "B+tree.h"
#include <iostream>
#include <algorithm>
#include <cstring>

// ================= 构造函数 =================
BPlusTree::BPlusTree(FileManager& fm, int file_id, int degree, int root_pid)
    : fm(fm), file_id(file_id), degree(degree), root_pid(root_pid) {}

// ================= 序列化 =================
void BPlusTree::serializeNode(const BPlusTreeNode& node, Page& page) {
    // 清页数据（可选）
    std::memset(page.data, 0, sizeof(page.data));

    char* buf = page.data;
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
            // 使用 uint16_t 存储每个 key 对应的 RID 数量
            uint16_t cnt = 0;
            if (i < (int)node.ridLists.size()) cnt = static_cast<uint16_t>(node.ridLists[i].size());
            std::memcpy(buf, &cnt, sizeof(uint16_t)); buf += sizeof(uint16_t);

            // 写入 cnt 个 RID
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

    // （可选）更新 page.header 等由调用方处理
}


// ================= 反序列化 =================
BPlusTreeNode BPlusTree::deserializeNode(const Page& page) {
    BPlusTreeNode node;
    const char* buf = page.data;

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

/// 精确搜索：返回所有与 key 相等的 RID（可能来自多个叶子）
std::vector<RID> BPlusTree::search(int key) {
    std::vector<RID> results;
    if (root_pid == -1) return results;  // 空树

    // 从根开始
    Page page(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return results;
    BPlusTreeNode node = deserializeNode(page);

    // 一路往下走到叶子
    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) ++i;
        int childPid = node.children[i];
        Page childPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return results;
        node = deserializeNode(childPage);
    }

    // 在当前叶子中查找等于 key 的位置，并收集对应的 ridLists
    for (int i = 0; i < node.keyCount; ++i) {
        if (node.keys[i] == key) {
            // 把该 key 的所有 RID 加入结果
            for (const RID& r : node.ridLists[i]) results.push_back(r);
        }
        else if (node.keys[i] > key) {
            // 因为 keys 升序，一旦大于 key，可以提前结束当前叶的扫描
            return results;
        }
    }

    // 继续沿叶子链向右查找可能分布在后续叶子的相同 key
    int nextPid = node.nextLeaf;
    while (nextPid != -1) {
        Page nextPage(DATA_PAGE, nextPid);
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

// 范围搜索 [keyLow, keyHigh]：返回所有落在区间内的 RID
std::vector<RID> BPlusTree::searchRange(int keyLow, int keyHigh) {
    std::vector<RID> results;
    if (root_pid == -1) return results; // 空树
    if (keyLow > keyHigh) return results;

    // 找到包含 keyLow 的叶子（或第一个 >= keyLow 的叶子）
    Page page(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return results;
    BPlusTreeNode node = deserializeNode(page);

    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && keyLow >= node.keys[i]) ++i;
        int childPid = node.children[i];
        Page childPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return results;
        node = deserializeNode(childPage);
    }

    // 从该叶子开始向右遍历叶子链，收集范围内的所有 RID
    int curPid = node.page_id;
    while (curPid != -1) {
        Page curPage(DATA_PAGE, curPid);
        if (!fm.readPage(file_id, curPid, curPage)) break;
        node = deserializeNode(curPage);

        for (int i = 0; i < node.keyCount; ++i) {
            int k = node.keys[i];
            if (k < keyLow) continue;
            if (k > keyHigh) return results; // 可提前结束整个范围扫描
            // k 在 [keyLow, keyHigh]
            for (const RID& r : node.ridLists[i]) results.push_back(r);
        }
        curPid = node.nextLeaf;
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
    // 如果文件为空，创建根叶子（page 0）
    if (fm.getPageCount(file_id) == 0) {
        BPlusTreeNode root;
        root.isLeaf = true;
        root.page_id = 0;
        root.keys = { key };
        root.ridLists = { std::vector<RID>{rid} }; // 注意：使用 ridLists
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
        // 根被分裂，新建根（在文件末尾追加）
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
        // 先查找是否已有该 key
        auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        int idx = it - node.keys.begin();

        if (it != node.keys.end() && *it == key) {
            // key 已存在：把 rid 加到对应的 ridLists 中（不新增 key）
            node.ridLists[idx].push_back(rid);
            // 写回页（无需分裂处理，除非你按字节检测溢出）
            Page p(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false; // 没分裂
        }

        // key 不存在：插入新 key 与对应的 rid 列表
        node.keys.insert(it, key);
        node.ridLists.insert(node.ridLists.begin() + idx, std::vector<RID>{rid});
        node.keyCount = static_cast<int>(node.keys.size());

        if (node.keyCount < degree) {
            // 没超出容量，直接写回
            Page p(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false; // 没分裂
        }

        // 分裂叶子
        int mid = node.keyCount / 2;
        int newPageId = fm.getPageCount(file_id);

        BPlusTreeNode newLeaf;
        newLeaf.isLeaf = true;
        newLeaf.page_id = newPageId;
        // keys 和 ridLists 都从 mid 开始搬到新叶
        newLeaf.keys.assign(node.keys.begin() + mid, node.keys.end());
        newLeaf.ridLists.assign(node.ridLists.begin() + mid, node.ridLists.end());
        newLeaf.keyCount = static_cast<int>(newLeaf.keys.size());
        newLeaf.nextLeaf = node.nextLeaf;

        // shrink left (原 node)
        node.keys.resize(mid);
        node.ridLists.resize(mid);
        node.keyCount = static_cast<int>(node.keys.size());
        node.nextLeaf = newPageId;

        // 写回两页
        Page oldPage(DATA_PAGE, node.page_id);
        serializeNode(node, oldPage);
        fm.writePage(file_id, oldPage);

        Page newPage(DATA_PAGE, newLeaf.page_id);
        serializeNode(newLeaf, newPage);
        fm.writePage(file_id, newPage);

        // 返回分裂信息：提升 newLeaf.keys[0]
        upKey = newLeaf.keys[0];
        newChildPid = newLeaf.page_id;
        return true;
    }
    else {
        // -------- 内部节点 --------
        int i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) ++i;

        int childPid = node.children[i];
        int childUpKey = -1, childNewPid = -1;
        bool childSplit = insertRecursive(childPid, key, rid, childUpKey, childNewPid);

        if (!childSplit) {
            return false;
        }

        // 子节点分裂，插入新的 key 和 child
        node.keys.insert(node.keys.begin() + i, childUpKey);
        node.children.insert(node.children.begin() + i + 1, childNewPid);
        node.keyCount = static_cast<int>(node.keys.size());

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

        // 右半部分（keys after mid, children after mid+1）
        newInternal.keys.assign(node.keys.begin() + mid + 1, node.keys.end());
        newInternal.children.assign(node.children.begin() + mid + 1, node.children.end());
        newInternal.keyCount = static_cast<int>(newInternal.keys.size());

        // 左半部分 shrink
        node.keys.resize(mid);
        node.children.resize(mid + 1);
        node.keyCount = static_cast<int>(node.keys.size());

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
void BPlusTree::remove(int key, const RID& rid) {
    if (root_pid == -1) return; // 空树

    deleteEntry(root_pid, -1, -1, key, rid);

    // 检查 root 是否需要收缩
    Page rootPage(DATA_PAGE, root_pid);
    fm.readPage(file_id, root_pid, rootPage);
    BPlusTreeNode root = deserializeNode(rootPage);

    if (!root.isLeaf && root.keyCount == 0) {
        // 根被删空 -> 提升唯一子节点为新根
        int newRootPid = root.children[0];
        root_pid = newRootPid;
    }
}


// deleteEntry: 在 node_pid 子树中删除 (key, rid)
void BPlusTree::deleteEntry(int node_pid, int parent_pid, int childIndex, int key, const RID& rid) {
    BPlusTreeNode node = loadNode(node_pid);

    if (node.isLeaf) {
        // 找 key
        auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        if (it == node.keys.end() || *it != key) {
            return; // key 不存在
        }
        int idx = it - node.keys.begin();

        // 在 ridList 里删 rid
        auto& ridList = node.ridLists[idx];
        auto ridIt = std::find(ridList.begin(), ridList.end(), rid);
        if (ridIt == ridList.end()) {
            return; // 该 rid 不存在
        }
        ridList.erase(ridIt);

        // 如果 ridList 为空，整条 key 删掉
        if (ridList.empty()) {
            node.keys.erase(node.keys.begin() + idx);
            node.ridLists.erase(node.ridLists.begin() + idx);
            node.keyCount = (int)node.keys.size();
        }

        saveNode(node);

        // 根节点直接结束
        if (node.page_id == root_pid) return;

        // 检查下溢
        if (node.keyCount < minKeysLeaf()) {
            if (parent_pid >= 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                handleLeafUnderflow(node, parent, childIndex);
                saveNode(parent);
            }
        }
        else {
            // 更新 parent 分隔键
            if (parent_pid >= 0 && childIndex > 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                parent.keys[childIndex - 1] = node.keys.front();
                saveNode(parent);
            }
        }
    }
    else {
        // 内部节点，递归
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

// ========== 借 / 合并 叶子 ==========

// 从左借（left 是左兄弟，right 是当前 child； parentIndex 指 left 在 parent.children 的索引）
bool BPlusTree::borrowFromLeftLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (left.keyCount <= minKeysLeaf()) return false;

    // move left.last -> right.front
    int movedKey = left.keys.back();
    auto movedList = left.ridLists.back();

    left.keys.pop_back();
    left.ridLists.pop_back();
    left.keyCount--;

    right.keys.insert(right.keys.begin(), movedKey);
    right.ridLists.insert(right.ridLists.begin(), movedList);
    right.keyCount++;

    // update parent separator
    parent.keys[parentIndex] = right.keys.front();
    return true;
}

// 从右借（left 是当前 child，right 是右兄弟； parentIndex 是 left 在 parent.children 的索引）
bool BPlusTree::borrowFromRightLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (right.keyCount <= minKeysLeaf()) return false;

    // move right.front -> left.back
    int movedKey = right.keys.front();
    auto movedList = right.ridLists.front();

    right.keys.erase(right.keys.begin());
    right.ridLists.erase(right.ridLists.begin());
    right.keyCount--;

    left.keys.push_back(movedKey);
    left.ridLists.push_back(movedList);
    left.keyCount++;

    // update parent separator
    if (!right.keys.empty()) {
        parent.keys[parentIndex] = right.keys.front();
    }
    return true;
}

// 合并右到左：left <- left + right；parentIndex 指 left 在 parent.children 的索引
void BPlusTree::mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    // 合并 key & ridLists
    left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
    left.ridLists.insert(left.ridLists.end(), right.ridLists.begin(), right.ridLists.end());
    left.keyCount = (int)left.keys.size();

    // 维护链表指针
    left.nextLeaf = right.nextLeaf;

    // 修改 parent：删除分隔 key 和右孩子指针
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
    // 记得设置 root_pid
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
        // ridLists
        node.ridLists.resize(node.keyCount);
        for (int i = 0; i < node.keyCount; i++) {
            int ridCount;
            memcpy(&ridCount, buf, sizeof(int)); buf += sizeof(int);
            node.ridLists[i].resize(ridCount);
            for (int j = 0; j < ridCount; j++) {
                memcpy(&node.ridLists[i][j], buf, sizeof(RID)); buf += sizeof(RID);
            }
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

/*
int main() {
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
}
*/


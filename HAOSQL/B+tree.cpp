// B+tree.cpp  �� ������ PageHeader / Page �ṹ
#include "B+tree.h"
#include <iostream>
#include <algorithm>
#include <cstring>

using namespace std;

// ================= ���캯�� =================
BPlusTree::BPlusTree(FileManager& fm, int file_id, int degree, int root_pid)
    : fm(fm), file_id(file_id), degree(degree), root_pid(root_pid) {}



// �������������� Page ����ʽ�� id
Page BPlusTree::initialPage(PageType type, int pid) {
    Page page(type, pid); // ����ԭ���캯��
    page.id = pid;        // ��ʽ��ֵ
    return page;
}

// ==================== ���л� ====================
void BPlusTree::serializeNode(const BPlusTreeNode& node, Page& page) {
    // ��ҳ����
    std::memset(page.data, 0, sizeof(page.data));

    // ׼����д�� PageHeader����ͷ������ page.data ��ͷ��
    PageHeader ph;
    ph.type = node.isLeaf ? DATA_PAGE : INDEX_PAGE;
    ph.page_id = page.id;
    ph.free_offset = sizeof(PageHeader); // ���ǰѸ���д�� header ֮��
    ph.slot_count = 0; // B+Tree �ڵ㲻ʹ�� slot ���ƣ�����չ��
    std::memcpy(page.data, &ph, sizeof(PageHeader));

    // buf ָ�� header ֮�󣬿�ʼд���л��� node ����
    char* buf = page.data + sizeof(PageHeader);

    // д����ͷ��Ϣ
    std::memcpy(buf, &node.isLeaf, sizeof(bool)); buf += sizeof(bool);
    std::memcpy(buf, &node.keyCount, sizeof(int)); buf += sizeof(int);
    std::memcpy(buf, &node.page_id, sizeof(int)); buf += sizeof(int);

    // д keys
    for (int k : node.keys) {
        std::memcpy(buf, &k, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        // Ҷ�ӣ�����ÿ�� key д�� RID �б��� + RID �б�����
        for (int i = 0; i < node.keyCount; ++i) {
            uint16_t cnt = 0;
            if (i < (int)node.ridLists.size()) cnt = static_cast<uint16_t>(node.ridLists[i].size());
            std::memcpy(buf, &cnt, sizeof(uint16_t)); buf += sizeof(uint16_t);

            for (uint16_t j = 0; j < cnt; ++j) {
                const RID& r = node.ridLists[i][j];
                std::memcpy(buf, &r, sizeof(RID)); buf += sizeof(RID);
            }
        }
        // д nextLeaf
        std::memcpy(buf, &node.nextLeaf, sizeof(int)); buf += sizeof(int);
    }
    else {
        // �ڲ��ڵ㣺д children��child count = keyCount + 1��
        for (int c : node.children) {
            std::memcpy(buf, &c, sizeof(int)); buf += sizeof(int);
        }
    }

    // ����ѡ�������ϣ�� header.free_offset ��ӳ��Ч�غ�ĩβ�����Լ��㲢������
    // size_t used = (buf - (page.data + sizeof(PageHeader)));
    // ph.free_offset = sizeof(PageHeader) + used;
    // std::memcpy(page.data, &ph, sizeof(PageHeader));
}

// ==================== �����л� ====================
BPlusTreeNode BPlusTree::deserializeNode(const Page& page) {
    BPlusTreeNode node;
    // ��ȡҳͷ�����������ҪУ�����Ϳ����ã�
    PageHeader ph;
    std::memcpy(&ph, page.data, sizeof(PageHeader));

    const char* buf = page.data + sizeof(PageHeader);

    // ������ͷ��Ϣ
    std::memcpy(&node.isLeaf, buf, sizeof(bool)); buf += sizeof(bool);
    std::memcpy(&node.keyCount, buf, sizeof(int)); buf += sizeof(int);
    std::memcpy(&node.page_id, buf, sizeof(int)); buf += sizeof(int);

    // �� keys
    node.keys.resize(node.keyCount);
    for (int i = 0; i < node.keyCount; ++i) {
        std::memcpy(&node.keys[i], buf, sizeof(int)); buf += sizeof(int);
    }

    if (node.isLeaf) {
        // Ҷ�ӣ���ÿ�� key �ȶ� rid_count���ٶ���Ӧ������ RID
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
        // �� nextLeaf
        std::memcpy(&node.nextLeaf, buf, sizeof(int)); buf += sizeof(int);
    }
    else {
        // �ڲ��ڵ㣺�� children��keyCount+1��
        node.children.resize(node.keyCount + 1);
        for (int i = 0; i < node.keyCount + 1; ++i) {
            std::memcpy(&node.children[i], buf, sizeof(int)); buf += sizeof(int);
        }
    }

    return node;
}

// ==================== ��ȷ���� ====================
std::vector<RID> BPlusTree::search(int key) {
    std::vector<RID> results;
    if (root_pid == -1) return results;  // ����

    // �Ӹ���ʼ
    Page page = initialPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, page)) return results;
    BPlusTreeNode node = deserializeNode(page);

    // һ·�����ߵ�Ҷ��
    while (!node.isLeaf) {
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) ++i;
        int childPid = node.children[i];
        Page childPage = initialPage(DATA_PAGE, childPid);
        if (!fm.readPage(file_id, childPid, childPage)) return results;
        node = deserializeNode(childPage);
    }

    // �ڵ�ǰҶ���в��ҵ��� key ��λ�ã����ռ���Ӧ�� ridLists
    for (int i = 0; i < node.keyCount; ++i) {
        if (node.keys[i] == key) {
            for (const RID& r : node.ridLists[i]) results.push_back(r);
        }
        else if (node.keys[i] > key) {
            return results;
        }
    }

    // ������Ҷ�������Ҳ��ҿ��ֲܷ��ں���Ҷ�ӵ���ͬ key
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

// ==================== ��Χ���� [keyLow, keyHigh] ====================
std::vector<RID> BPlusTree::searchRange(int keyLow, int keyHigh) {
    std::vector<RID> results;
    if (root_pid == -1) return results; // ����
    if (keyLow > keyHigh) return results;

    // �ҵ����� keyLow ��Ҷ�ӣ����һ�� >= keyLow ��Ҷ�ӣ�
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

    // �Ӹ�Ҷ�ӿ�ʼ���ұ���Ҷ�������ռ���Χ�ڵ����� RID
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


// ---------- IO ��װ ----------
BPlusTreeNode BPlusTree::loadNode(int pid) {
    Page p = initialPage(DATA_PAGE, pid);
    if (!fm.readPage(file_id, pid, p)) {
        // �����ʧ�ܣ����ؿսڵ㣨page_id ����Ϊ pid �Ա�д�أ�
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

// ---------- ���� ----------
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


// ================= ���� =================
// ================== ������� ==================
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

// ================== �ݹ���� ==================
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



// === ɾ����� ===
void BPlusTree::remove(int key, const RID& rid) {
    std::lock_guard<std::mutex> lock(treeMutex);

    if (root_pid == -1) return; // ����

    deleteEntry(root_pid, -1, -1, key, rid);

    Page rootPage = initialPage(DATA_PAGE, root_pid);
    fm.readPage(file_id, root_pid, rootPage);
    BPlusTreeNode root = deserializeNode(rootPage);

    if (!root.isLeaf && root.keyCount == 0) {
        int newRootPid = root.children[0];
        root_pid = newRootPid;
    }
}


// deleteEntry: �� node_pid ������ɾ�� (key, rid)
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

// Ҷ�����紦��child �Ǳ������Ҷ�ӽڵ㣬parent �Ѿ� load��parentIndex �� child �� parent.children �е�����
void BPlusTree::handleLeafUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex) {
    int idx = parentIndex; // child �� parent.children[idx]

    // �������ֵ�
    if (idx > 0) {
        BPlusTreeNode left = loadNode(parent.children[idx - 1]);
        if (borrowFromLeftLeaf(left, child, parent, idx - 1)) {
            saveNode(left); saveNode(child); saveNode(parent);
            return;
        }
        // ����ϲ� left + child
        mergeLeafs(left, child, parent, idx - 1);
        saveNode(left); saveNode(parent);
        return;
    }

    // �������ֵ�
    if (idx + 1 < (int)parent.children.size()) {
        BPlusTreeNode right = loadNode(parent.children[idx + 1]);
        if (borrowFromRightLeaf(child, right, parent, idx)) {
            saveNode(right); saveNode(child); saveNode(parent);
            return;
        }
        // �ϲ� child + right
        mergeLeafs(child, right, parent, idx);
        saveNode(child); saveNode(parent);
        return;
    }
    // ���û���ֵܣ������ϲ�Ӧ��������ֱ�ӷ���
}

// �ڲ��ڵ����紦��child �Ǳ�������ڲ��ڵ㣬parent �� load��parentIndex Ϊ child �� parent.children ������
void BPlusTree::handleInternalUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex) {
    int idx = parentIndex;

    // �������ֵ�
    if (idx > 0) {
        BPlusTreeNode left = loadNode(parent.children[idx - 1]);
        if (borrowFromLeftInternal(left, child, parent, idx - 1)) {
            saveNode(left); saveNode(child); saveNode(parent);
            return;
        }
        // �ϲ� left + child
        mergeInternals(left, child, parent, idx - 1);
        saveNode(left); saveNode(parent);
        return;
    }

    // �������ֵ�
    if (idx + 1 < (int)parent.children.size()) {
        BPlusTreeNode right = loadNode(parent.children[idx + 1]);
        if (borrowFromRightInternal(child, right, parent, idx)) {
            saveNode(right); saveNode(child); saveNode(parent);
            return;
        }
        // �ϲ� child + right
        mergeInternals(child, right, parent, idx);
        saveNode(child); saveNode(parent);
        return;
    }
}

// ========== �� / �ϲ� Ҷ�� ==========

// ����裨left �����ֵܣ�right �ǵ�ǰ child�� parentIndex ָ left �� parent.children ��������
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

// ���ҽ裨left �ǵ�ǰ child��right �����ֵܣ� parentIndex �� left �� parent.children ��������
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

// �ϲ��ҵ���left <- left + right��parentIndex ָ left �� parent.children ������
void BPlusTree::mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
    left.ridLists.insert(left.ridLists.end(), right.ridLists.begin(), right.ridLists.end());
    left.keyCount = (int)left.keys.size();

    left.nextLeaf = right.nextLeaf;

    parent.keys.erase(parent.keys.begin() + parentIndex);
    parent.children.erase(parent.children.begin() + parentIndex + 1);
    parent.keyCount = (int)parent.keys.size();
}

// ========== �� / �ϲ� �ڲ��ڵ� ==========

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

// �ϲ��ڲ��ڵ㣺 left <- left + sep + right
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



// ================= ��ӡ�� =================
void BPlusTree::printTree() {
    Page rootPage = initialPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, rootPage)) {
        std::cerr << "�޷���ȡ���ڵ� page " << root_pid << std::endl;
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

    // ������ڵ�ҳ
    int rootPid = fm.allocatePage(fileId, INDEX_PAGE);

    // ��ʼ�����ڵ㣨Ҷ�ӽڵ㣩
    BPlusTreeNode root;
    root.isLeaf = true;
    root.page_id = rootPid;
    root.keyCount = 0;
    root.nextLeaf = -1;

    // �����ڵ����л�д��ҳ
    Page rootPage = initialPage(INDEX_PAGE, rootPid);
    rootPage.id = rootPid;
    serializeNode(root, rootPage);
    fm.writePage(fileId, rootPage);

    std::cout << "Index created in file_id=" << fileId
        << " root page=" << rootPid << std::endl;

    return rootPid;
}

// B+���������٣��ͷ�����ռ��ҳ��
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

    //cout << "ɾ�� 20...\n";
    //tree.remove(20);
    //tree.printTree();

    //cout << "ɾ�� 70...\n";
    //tree.remove(70);
    //tree.printTree();
    return 0;
}*/

/*int main() {
    std::string indexFileName = "students";

    // ---------------- �����ļ������� ----------------
    FileManager fm("./");                 // ���ݿ�Ŀ¼
    int file_id = fm.openFile(indexFileName);

    // ---------------- ��ʼ�� B+Tree ----------------
    BPlusTree tree(fm, file_id, 4 , 0);      // degree = 4
    int root_pid = tree.createIndex(fm, file_id);  // ���ظ�ҳ��
    std::cout << "Index created. Root page: " << root_pid << std::endl;
    tree.setRootPid(root_pid);           // ���øմ����ĸ�ҳ
    tree.printTree();

    // ---------------- ������� ----------------
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

    // ---------------- ɾ������ ----------------
    std::cout << "\n--- Removing keys ---\n";
    tree.remove(20, { 1,2 });
    tree.remove(40, { 2,2 });
    tree.remove(70, { 4,1 });
    tree.printTree();

    // ---------------- ��ѯ���� ----------------
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

    // ---------------- �������� ----------------
    std::cout << "\n--- Destroying index ---\n";
    tree.destroyIndex(fm, file_id, root_pid);  // �ݹ��ͷ����нڵ�
    std::cout << "Index destroyed.\n";

    return 0;
}*/


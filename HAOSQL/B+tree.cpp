#include "B+tree.h"
#include <iostream>
#include <algorithm>
#include <cstring>

// ================= ���캯�� =================
BPlusTree::BPlusTree(FileManager& fm, int file_id, int degree, int root_pid)
    : fm(fm), file_id(file_id), degree(degree), root_pid(root_pid) {}

// ================= ���л� =================
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

// ================= �����л� =================
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

// ================= ���� =================
bool BPlusTree::search(int key, RID& rid) {
    if (root_pid == -1) return false;  // ����

    // �Ӹ���ʼ
    Page rootPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, rootPage)) return false;

    BPlusTreeNode node = deserializeNode(rootPage);

    // һ·������
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

    // ��Ҷ�Ӳ��� key
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
    if (root_pid == -1) return results; // ����

    // ���ҵ� keyLow ���ڵ�Ҷ��
    Page rootPage(DATA_PAGE, root_pid);
    if (!fm.readPage(file_id, root_pid, rootPage)) return results;

    BPlusTreeNode node = deserializeNode(rootPage);

    // һ·�����ҵ�Ҷ��
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

    // ����Ҷ�������ռ� [keyLow, keyHigh] ��Χ�ļ�
    while (true) {
        for (int i = 0; i < node.keyCount; i++) {
            int k = node.keys[i];
            if (k >= keyLow && k <= keyHigh) {
                results.push_back(node.rids[i]);
            }
            else if (k > keyHigh) {
                return results; // ��ǰ����
            }
        }
        if (node.nextLeaf == -1) break; // �������һ��Ҷ��
        Page nextPage(DATA_PAGE, node.nextLeaf);
        if (!fm.readPage(file_id, node.nextLeaf, nextPage)) break;
        node = deserializeNode(nextPage);
    }

    return results;
}


// ---------- IO ��װ ----------
BPlusTreeNode BPlusTree::loadNode(int pid) {
    Page p(DATA_PAGE, pid);
    if (!fm.readPage(file_id, pid, p)) {
        // �����ʧ�ܣ����ؿսڵ㣨page_id ����Ϊ pid �Ա�д�أ�
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

// ---------- ���� ----------
int BPlusTree::minKeysLeaf() const {
    // leaf ��� key = degree - 1
    // ͨ����С = ceil((degree-1)/2) => ��ȡ (degree-1+1)/2 = degree/2
    return degree / 2;
}
int BPlusTree::minKeysInternal() const {
    // internal ��С keys = ceil(degree/2) - 1
    int minChildren = (degree + 1) / 2;
    return minChildren - 1;
}
int BPlusTree::findChildIndex(const BPlusTreeNode& parent, int childPid) const {
    for (int i = 0; i < (int)parent.children.size(); ++i) if (parent.children[i] == childPid) return i;
    return -1;
}

// ================= ���� =================
// ================== ������� ==================
void BPlusTree::insert(int key, const RID& rid) {
    if (fm.getPageCount(file_id) == 0) {
        // �������½���Ҷ��
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

    // �ݹ����
    int newChildPid = -1;
    int upKey = -1;
    bool hasSplit = insertRecursive(root_pid, key, rid, upKey, newChildPid);

    if (hasSplit) {
        // �������ѣ��½���
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

// ================== �ݹ���� ==================
bool BPlusTree::insertRecursive(int nodePid, int key, const RID& rid, int& upKey, int& newChildPid) {
    // ����ǰ�ڵ�
    Page page(DATA_PAGE, nodePid);
    fm.readPage(file_id, nodePid, page);
    BPlusTreeNode node = deserializeNode(page);

    if (node.isLeaf) {
        // -------- Ҷ�ӽڵ���� --------
        auto it = lower_bound(node.keys.begin(), node.keys.end(), key);
        int idx = it - node.keys.begin();
        node.keys.insert(it, key);
        node.rids.insert(node.rids.begin() + idx, rid);
        node.keyCount++;

        if (node.keyCount < degree) {
            // û����������ֱ��д��
            Page p(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false; // û����
        }

        // ����
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

        // д��
        Page oldPage(DATA_PAGE, node.page_id);
        serializeNode(node, oldPage);
        fm.writePage(file_id, oldPage);

        Page newPage(DATA_PAGE, newLeaf.page_id);
        serializeNode(newLeaf, newPage);
        fm.writePage(file_id, newPage);

        // ���ط�����Ϣ
        upKey = newLeaf.keys[0];
        newChildPid = newLeaf.page_id;
        return true;
    }
    else {
        // -------- �ڲ��ڵ� --------
        int i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) i++;

        int childPid = node.children[i];
        int childUpKey, childNewPid;
        bool childSplit = insertRecursive(childPid, key, rid, childUpKey, childNewPid);

        if (!childSplit) {
            return false;
        }

        // �ӽڵ���ѣ������µ� key �� child
        node.keys.insert(node.keys.begin() + i, childUpKey);
        node.children.insert(node.children.begin() + i + 1, childNewPid);
        node.keyCount++;

        if (node.keyCount < degree) {
            Page p(DATA_PAGE, node.page_id);
            serializeNode(node, p);
            fm.writePage(file_id, p);
            return false;
        }

        // �ڲ��ڵ����
        int mid = node.keyCount / 2;
        upKey = node.keys[mid];

        int newPageId = fm.getPageCount(file_id);
        BPlusTreeNode newInternal;
        newInternal.isLeaf = false;
        newInternal.page_id = newPageId;

        // �Ұ벿��
        newInternal.keys.assign(node.keys.begin() + mid + 1, node.keys.end());
        newInternal.children.assign(node.children.begin() + mid + 1, node.children.end());
        newInternal.keyCount = newInternal.keys.size();

        // ��벿��
        node.keys.resize(mid);
        node.children.resize(mid + 1);
        node.keyCount = node.keys.size();

        // д��
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

// === ɾ����� ===
void BPlusTree::remove(int key) {
    if (root_pid == -1) return; // ����

    // �Ӹ���ʼ�ݹ�ɾ�������� parent_pid = -1
    deleteEntry(root_pid, -1, -1, key);

    // ������ڵ����ڲ��ڵ����Ѿ����ˣ���Ҫ���� root_pid
    Page rootPage(DATA_PAGE, root_pid);
    fm.readPage(file_id, root_pid, rootPage);
    BPlusTreeNode root = deserializeNode(rootPage);

    if (!root.isLeaf && root.keyCount == 0) {
        // ����ɾ�� -> ����Ψһ�ӽڵ�Ϊ�¸�
        int newRootPid = root.children[0];
        root_pid = newRootPid;
    }
}


// deleteEntry: �� node_pid ������ɾ�� key�� parent_pid/childIndex ��ʾ���׺� child �ڸ����е�������root �� parent_pid = -1��
void BPlusTree::deleteEntry(int node_pid, int parent_pid, int childIndex, int key) {
    BPlusTreeNode node = loadNode(node_pid);

    if (node.isLeaf) {
        // ɾ�� key�������ڣ�
        auto it = std::lower_bound(node.keys.begin(), node.keys.end(), key);
        if (it == node.keys.end() || *it != key) {
            // key �����ڣ�ֱ�ӷ���
            return;
        }
        int idx = it - node.keys.begin();
        node.keys.erase(node.keys.begin() + idx);
        node.rids.erase(node.rids.begin() + idx);
        node.keyCount = (int)node.keys.size();
        saveNode(node);

        // �� node �Ǹ������
        if (node.page_id == root_pid) return;

        // �������
        if (node.keyCount < minKeysLeaf()) {
            // ��Ҫ�����ף�parent ������ڣ�
            if (parent_pid >= 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                handleLeafUnderflow(node, parent, childIndex);
                saveNode(parent);
            }
        }
        else {
            // ������ underflow��������Ҫ���� parent �ָ�����childIndex > 0��
            if (parent_pid >= 0 && childIndex > 0) {
                BPlusTreeNode parent = loadNode(parent_pid);
                parent.keys[childIndex - 1] = node.keys.front();
                saveNode(parent);
            }
        }
    }
    else {
        // �ڲ��ڵ㣺�ҵ����� child
        int i = 0;
        while (i < node.keyCount && key >= node.keys[i]) ++i;
        int childPid = node.children[i];
        // �ݹ�ɾ��
        deleteEntry(childPid, node_pid, i, key);

        // ���ص�ǰ node�����ӵݹ��п��ܱ��޸ģ�
        node = loadNode(node_pid);

        // �ڲ��ڵ���ܶ�ʧ��һ�� key������Ӻϲ���
        if (node.page_id == root_pid) {
            // special: root shrink handled after top-level deletion
        }
        else {
            if (node.keyCount < minKeysInternal()) {
                // �����ڲ�����
                if (parent_pid >= 0) {
                    BPlusTreeNode parent = loadNode(parent_pid);
                    handleInternalUnderflow(node, parent, childIndex);
                    saveNode(parent);
                }
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

// ���ҽ裨left �ǵ�ǰ child�� right �����ֵܣ� parentIndex �� left �� parent.children ��������
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

// �ϲ��ҵ���left <- left + right��parentIndex ָ left �� parent.children ������
void BPlusTree::mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
    left.rids.insert(left.rids.end(), right.rids.begin(), right.rids.end());
    left.keyCount = (int)left.keys.size();
    left.nextLeaf = right.nextLeaf;

    // �޸� parent��ɾ�� separator parent.keys[parentIndex]��ɾ�� right child
    parent.keys.erase(parent.keys.begin() + parentIndex);
    parent.children.erase(parent.children.begin() + parentIndex + 1);
    parent.keyCount = (int)parent.keys.size();
}

// ========== �� / �ϲ� �ڲ��ڵ� ==========

// ����裨left �����ֵܣ� right �ǵ�ǰ child�� parentIndex ָ left �� parent.children��
bool BPlusTree::borrowFromLeftInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (left.keyCount <= minKeysInternal()) return false;

    // �ƶ� left ����� key �� parent, parent �� sep ���Ƶ� right.front
    int leftLastKey = left.keys.back();
    int leftLastChild = left.children.back();
    left.keys.pop_back(); left.children.pop_back(); left.keyCount--;

    // parent.keys[parentIndex] �ŵ� right ��ǰ��
    int sep = parent.keys[parentIndex];
    right.keys.insert(right.keys.begin(), sep);
    right.children.insert(right.children.begin(), leftLastChild);
    right.keyCount++;

    // parent �� new sep �� leftLastKey
    parent.keys[parentIndex] = leftLastKey;
    return true;
}

bool BPlusTree::borrowFromRightInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex) {
    if (right.keyCount <= minKeysInternal()) return false;

    int sep = parent.keys[parentIndex];
    // sep ���Ƶ� left
    left.keys.push_back(sep);
    left.children.push_back(right.children.front());
    left.keyCount++;

    // parent �� new sep �� right.keys.front()
    parent.keys[parentIndex] = right.keys.front();

    // remove from right
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
    // �ǵ�����root_pid
    Page rootPage(DATA_PAGE, root_pid);
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
    // ������ڵ�ҳ
    int rootPid = fm.allocatePage(fileId, INDEX_PAGE);

    // ��ʼ�����ڵ㣨Ҷ�ӽڵ㣩
    BPlusTreeNode root;
    root.isLeaf = true;
    root.page_id = rootPid;
    root.keyCount = 0;
    root.nextLeaf = -1;

    // �����ڵ����л�д��ҳ
    Page rootPage(INDEX_PAGE, rootPid);
    serializeNode(root, rootPage);   // ʹ������ BPlusTree ���л�����
    fm.writePage(fileId, rootPage);

    std::cout << "Index created in file_id=" << fileId
        << " root page=" << rootPid << std::endl;

    return rootPid;
}

// B+���������٣��ͷ�����ռ��ҳ��
void BPlusTree::destroyIndex(FileManager& fm, int root_pid, int file_id) {
    if (root_pid < 0) return;

    // ��ȡ��ǰ�ڵ�
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
        // Ҷ�ӽڵ�
        node.rids.resize(node.keyCount);
        for (int i = 0; i < node.keyCount; i++) {
            memcpy(&node.rids[i], buf, sizeof(RID)); buf += sizeof(RID);
        }
        memcpy(&node.nextLeaf, buf, sizeof(int)); buf += sizeof(int);

        // �ͷ�Ҷ�ӽڵ�ҳ
        fm.freePage(file_id, node.page_id);
    }
    else {
        // �ڲ��ڵ�
        node.children.resize(node.keyCount + 1);
        for (int i = 0; i < node.keyCount + 1; i++) {
            memcpy(&node.children[i], buf, sizeof(int)); buf += sizeof(int);
        }

        // �ݹ������ӽڵ�
        for (int child_pid : node.children) {
            destroyIndex(fm, child_pid, file_id);
        }

        // �ͷ��ڲ��ڵ�ҳ
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

    //cout << "ɾ�� 20...\n";
    //tree.remove(20);
    //tree.printTree();

    //cout << "ɾ�� 70...\n";
    //tree.remove(70);
    //tree.printTree();
    return 0;
}*/

int main() {
    std::string indexFileName = "students";

    // ---------------- �����ļ������� ----------------
    FileManager fm("./");                 // ���ݿ�Ŀ¼
    int file_id = fm.openFile(indexFileName);

    // ---------------- ��ʼ�� B+Tree ----------------
    BPlusTree tree(fm, file_id, 5 , 0);      // degree = 4
    int root_pid = tree.createIndex(fm, file_id);  // ���ظ�ҳ��
    std::cout << "Index created. Root page: " << root_pid << std::endl;
    tree.setRootPid(root_pid);           // ���øմ����ĸ�ҳ
    tree.printTree();

    // ---------------- ������� ----------------
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

    // ---------------- ɾ������ ----------------
    std::cout << "\n--- Removing keys ---\n";
    tree.remove(20);
    tree.remove(40);
    tree.remove(70);
    tree.printTree();

    // ---------------- ��ѯ���� ----------------
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

    // ---------------- �������� ----------------
    std::cout << "\n--- Destroying index ---\n";
    tree.destroyIndex(fm, file_id, root_pid);  // �ݹ��ͷ����нڵ�
    std::cout << "Index destroyed.\n";

    return 0;
}


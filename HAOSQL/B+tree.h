#include <vector>
#include "dataType.h"
#include "page.h"
#include "file_manager.h"

// B+���ڵ�
struct BPlusTreeNode {
    bool isLeaf;                 // �Ƿ���Ҷ�ӽڵ�
    int keyCount;                // ��ǰ��ŵ� key ����
    int page_id;                 // ��ǰ�ڵ�� page_id�����ڴ��̣�

    // �ڲ��ڵ�
    std::vector<int> keys;       // key �б��������У�
    std::vector<int> children;   // ������ڲ��ڵ㣺child page_id �б�

    // Ҷ�ӽڵ�
    std::vector<std::vector<RID>> ridLists;
    int nextLeaf;                // Ҷ�ӽڵ�ָ����һ��Ҷ�ӽڵ�� page_id (-1 ��ʾĩβ)

    BPlusTreeNode() : isLeaf(true), keyCount(0), page_id(-1), nextLeaf(-1) {}

}; 

class BPlusTree {
public:
    BPlusTree(FileManager& fm, int file_id, int degree, int root_pid);

    // ��������
    void insert(int key, const RID& rid);
    void remove(int key, const RID& rid);
    vector<RID> search(int key);
    // ��Χ���� [keyLow, keyHigh]
    std::vector<RID> searchRange(int keyLow, int keyHigh);

    int createIndex(FileManager& fm, int fileId);
    void destroyIndex(FileManager& fm, int root_pid, int file_id);

    void setRootPid(int rootid);

    // ��ӡ��
    void printTree();

private:
    FileManager& fm;
    int file_id;
    int degree;
    int root_pid;  // ���ڵ� page id

    // ���л�/�����л�
    void serializeNode(const BPlusTreeNode& node, Page& page);
    BPlusTreeNode deserializeNode(const Page& page);

    bool insertRecursive(int nodePid, int key, const RID& rid, int& upKey, int& newChildPid);


    // === ���� IO ��װ ===
    BPlusTreeNode loadNode(int pid);
    void saveNode(const BPlusTreeNode& node);


    // ����
    int minKeysLeaf() const;
    int minKeysInternal() const;
    int findChildIndex(const BPlusTreeNode& parent, int childPid) const;


    // === ɾ����أ���ʹ�� parentMap��ͨ���ݹ鴫 parent ��Ϣ��===
    void deleteEntry(int node_pid, int parent_pid, int childIndex, int key, const RID& rid);

    // underflow �������ڵ��� load��
    void handleLeafUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex);
    void handleInternalUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex);

    // ��/�ϲ���Ҷ�ӣ�
    bool borrowFromLeftLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    bool borrowFromRightLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    void mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);

    // ��/�ϲ����ڲ���
    bool borrowFromLeftInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    bool borrowFromRightInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    void mergeInternals(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);

    // ����
    void printNode(const BPlusTreeNode& node, int level);
};

#pragma once

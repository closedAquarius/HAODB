#include <vector>
#include "dataType.h"
#include "page.h"
#include "file_manager.h"

// B+树节点
struct BPlusTreeNode {
    bool isLeaf;                 // 是否是叶子节点
    int keyCount;                // 当前存放的 key 数量
    int page_id;                 // 当前节点的 page_id（用于存盘）

    // 内部节点
    std::vector<int> keys;       // key 列表（升序排列）
    std::vector<int> children;   // 如果是内部节点：child page_id 列表

    // 叶子节点
    std::vector<std::vector<RID>> ridLists;
    int nextLeaf;                // 叶子节点指向下一个叶子节点的 page_id (-1 表示末尾)

    BPlusTreeNode() : isLeaf(true), keyCount(0), page_id(-1), nextLeaf(-1) {}

}; 

class BPlusTree {
public:
    BPlusTree(FileManager& fm, int file_id, int degree, int root_pid);

    // 基本操作
    void insert(int key, const RID& rid);
    void remove(int key, const RID& rid);
    vector<RID> search(int key);
    // 范围查找 [keyLow, keyHigh]
    std::vector<RID> searchRange(int keyLow, int keyHigh);

    int createIndex(FileManager& fm, int fileId);
    void destroyIndex(FileManager& fm, int root_pid, int file_id);

    void setRootPid(int rootid);

    // 打印树
    void printTree();

private:
    FileManager& fm;
    int file_id;
    int degree;
    int root_pid;  // 根节点 page id

    // 序列化/反序列化
    void serializeNode(const BPlusTreeNode& node, Page& page);
    BPlusTreeNode deserializeNode(const Page& page);

    bool insertRecursive(int nodePid, int key, const RID& rid, int& upKey, int& newChildPid);


    // === 基本 IO 封装 ===
    BPlusTreeNode loadNode(int pid);
    void saveNode(const BPlusTreeNode& node);


    // 工具
    int minKeysLeaf() const;
    int minKeysInternal() const;
    int findChildIndex(const BPlusTreeNode& parent, int childPid) const;


    // === 删除相关（不使用 parentMap，通过递归传 parent 信息）===
    void deleteEntry(int node_pid, int parent_pid, int childIndex, int key, const RID& rid);

    // underflow 处理（父节点已 load）
    void handleLeafUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex);
    void handleInternalUnderflow(BPlusTreeNode& child, BPlusTreeNode& parent, int parentIndex);

    // 借/合并（叶子）
    bool borrowFromLeftLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    bool borrowFromRightLeaf(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    void mergeLeafs(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);

    // 借/合并（内部）
    bool borrowFromLeftInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    bool borrowFromRightInternal(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);
    void mergeInternals(BPlusTreeNode& left, BPlusTreeNode& right, BPlusTreeNode& parent, int parentIndex);

    // 辅助
    void printNode(const BPlusTreeNode& node, int level);
};

#pragma once

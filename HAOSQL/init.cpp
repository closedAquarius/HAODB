// init_bplustree.cpp
#include <iostream>
#include <vector>
#include <cstring>
#include "file_manager.h"
#include "page.h"

struct RID {
    int page_id;
    int slot_id;
};

struct BPlusTreeNode {
    bool isLeaf;
    int keyCount;
    int page_id;
    std::vector<int> keys;
    std::vector<int> children;
    std::vector<RID> rids;
    int nextLeaf;
    BPlusTreeNode() : isLeaf(true), keyCount(0), page_id(-1), nextLeaf(-1) {}
};

/*// 把 node 序列化到一个字节缓冲区（不做额外对齐）
void serializeNode(const BPlusTreeNode& node, std::vector<char>& out) {
    out.clear();
    out.reserve(4096);
    auto append = [&](const void* src, size_t n) {
        const char* p = reinterpret_cast<const char*>(src);
        out.insert(out.end(), p, p + n);
        };

    append(&node.isLeaf, sizeof(node.isLeaf));
    append(&node.keyCount, sizeof(node.keyCount));
    append(&node.page_id, sizeof(node.page_id));

    // keys
    for (int i = 0; i < node.keyCount; ++i) append(&node.keys[i], sizeof(int));

    if (node.isLeaf) {
        // rids
        for (int i = 0; i < node.keyCount; ++i) append(&node.rids[i], sizeof(RID));
        append(&node.nextLeaf, sizeof(node.nextLeaf));
    }
    else {
        // children (keyCount + 1)
        for (int i = 0; i < node.keyCount + 1; ++i) append(&node.children[i], sizeof(int));
    }
}*/

// 从 page.data 反序列化 BPlusTreeNode
static BPlusTreeNode deserializeNodeFromPage(const Page& page) {
    BPlusTreeNode node;
    const char* buf = page.getDataConst();
    size_t offset = 0;

    std::memcpy(&node.isLeaf, buf + offset, sizeof(node.isLeaf)); offset += sizeof(node.isLeaf);
    std::memcpy(&node.keyCount, buf + offset, sizeof(node.keyCount)); offset += sizeof(node.keyCount);
    std::memcpy(&node.page_id, buf + offset, sizeof(node.page_id)); offset += sizeof(node.page_id);

    node.keys.resize(node.keyCount);
    for (int i = 0; i < node.keyCount; ++i) {
        std::memcpy(&node.keys[i], buf + offset, sizeof(int)); offset += sizeof(int);
    }

    if (node.isLeaf) {
        node.rids.resize(node.keyCount);
        for (int i = 0; i < node.keyCount; ++i) {
            std::memcpy(&node.rids[i], buf + offset, sizeof(RID)); offset += sizeof(RID);
        }
        std::memcpy(&node.nextLeaf, buf + offset, sizeof(node.nextLeaf)); offset += sizeof(node.nextLeaf);
    }
    else {
        node.children.resize(node.keyCount + 1);
        for (int i = 0; i < node.keyCount + 1; ++i) {
            std::memcpy(&node.children[i], buf + offset, sizeof(int)); offset += sizeof(int);
        }
    }
    return node;
}



/*int main() {
    // 1) 创建 FileManager，db_dir 用当前目录（或指定目录）
    FileManager fm("."); // 注意：你的实现需要 FileManager(const std::string& db_dir)
    int file_id = fm.openFile("students_idx"); // 生成 ./students_idx.tbl

    // 2) 构造示例节点（与我们讨论的例子一致）
    BPlusTreeNode root;
    root.isLeaf = false;
    root.keyCount = 2;
    root.page_id = 0;
    root.keys = { 30, 60 };
    root.children = { 1, 2, 3 };

    BPlusTreeNode leaf1;
    leaf1.isLeaf = true;
    leaf1.keyCount = 2;
    leaf1.page_id = 1;
    leaf1.keys = { 10, 20 };
    leaf1.rids = { {1,1}, {1,2} };
    leaf1.nextLeaf = 2;

    BPlusTreeNode leaf2;
    leaf2.isLeaf = true;
    leaf2.keyCount = 2;
    leaf2.page_id = 2;
    leaf2.keys = { 40, 50 };
    leaf2.rids = { {2,1}, {2,2} };
    leaf2.nextLeaf = 3;

    BPlusTreeNode leaf3;
    leaf3.isLeaf = true;
    leaf3.keyCount = 2;
    leaf3.page_id = 3;
    leaf3.keys = { 70, 80 };
    leaf3.rids = { {3,1}, {3,2} };
    leaf3.nextLeaf = -1;

    // 缓冲区和 Page
    std::vector<char> buf;
    // Page 0
    Page p0(DATA_PAGE, 0);
    serializeNode(root, buf);
    p0.setData(buf.data(), buf.size());
    fm.writePage(file_id, p0);

    // Page 1
    Page p1(DATA_PAGE, 1);
    serializeNode(leaf1, buf);
    p1.setData(buf.data(), buf.size());
    fm.writePage(file_id, p1);

    // Page 2
    Page p2(DATA_PAGE, 2);
    serializeNode(leaf2, buf);
    p2.setData(buf.data(), buf.size());
    fm.writePage(file_id, p2);

    // Page 3
    Page p3(DATA_PAGE, 3);
    serializeNode(leaf3, buf);
    p3.setData(buf.data(), buf.size());
    fm.writePage(file_id, p3);

    std::cout << "写入 4 个 B+ 树节点到 students_idx.tbl（pages 0..3）\n";

    // 3) 读回并打印验证
    Page read0(DATA_PAGE, 0);
    fm.readPage(file_id, 0, read0);
    BPlusTreeNode r = deserializeNodeFromPage(read0);
    std::cout << "Root (page " << r.page_id << ") keys: ";
    for (int k : r.keys) std::cout << k << " ";
    std::cout << "\n";

    for (int pid = 1; pid <= 3; ++pid) {
        Page rp(DATA_PAGE, pid);
        fm.readPage(file_id, pid, rp);
        BPlusTreeNode ln = deserializeNodeFromPage(rp);
        std::cout << "Leaf page " << ln.page_id << " keys: ";
        for (int k : ln.keys) std::cout << k << " ";
        std::cout << "; nextLeaf=" << ln.nextLeaf << "\n";
    }

    return 0;
}*/

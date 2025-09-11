#pragma once
#include <list>
#include "file_manager.h"

using namespace std;

// ========== 页对象 ==========
using PageId = int;
struct Page {
    PageId id;
    bool dirty;
    int pin_count;
    vector<char> data;      // 实际的存储页

    Page(size_t page_size);
};

// ========== LRU淘汰策略 ==========
class LRUReplacer {
    size_t capacity;
    list<PageId> lru_list;
    unordered_map<PageId, list<PageId>::iterator> pos;
public:
    LRUReplacer(size_t c);
    void access(PageId id);
    PageId victim();
    void erase(PageId id);
};

// ========== 缓冲池管理 ==========
class BufferPoolManager {
    size_t pool_size;
    vector<Page> frames;
    unordered_map<PageId, int> page_table; // page_id -> frame_id
    LRUReplacer replacer;
public:
    BufferPoolManager(size_t pool_size);

    Page* fetchPage(PageId id);
    void unpinPage(PageId id, bool is_dirty);
    void flushPage(PageId id);
    Page* newPage(PageId id);

    void test();
};
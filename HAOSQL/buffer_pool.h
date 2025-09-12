#pragma once
#include <list>
#include "page.h"

using namespace std;

// ========== LRUÃ‘Ã≠≤ﬂ¬‘ ==========
class LRUReplacer {
    size_t capacity;
    list<PageId> lru_list;
    unordered_map<PageId, list<PageId>::iterator> pos;
public:
    LRUReplacer(size_t c);
    void access(PageId id);
    void push(PageId id);
    PageId victim();
    void erase(PageId id);
};

// ========== ª∫≥Â≥ÿπ‹¿Ì ==========
class BufferPoolManager {
    size_t pool_size;
    vector<Page> frames;
    unordered_map<PageId, int> page_table; // page_id -> frame_id
    LRUReplacer replacer;
    DiskManager* disk;
public:
    BufferPoolManager(size_t pool_size, DiskManager* d);

    Page* fetchPage(PageId id);
    void unpinPage(PageId id, bool is_dirty);
    void flushPage(PageId id);
    Page* newPage(PageId id);

    void test();
};
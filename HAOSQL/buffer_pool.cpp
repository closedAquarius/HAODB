#include "buffer_pool.h"

using namespace std;

Page::Page(size_t page_size) : id(-1), dirty(false), pin_count(0), data(page_size) {}

LRUReplacer::LRUReplacer(size_t c) :capacity(c) {}
// 将最近使用的页移动到链表头，表头为最近使用的，表尾为最久未使用的
void LRUReplacer::access(PageId id) {
    if (pos.count(id)) {
        lru_list.erase(pos[id]);
    }

    lru_list.push_front(id);
    pos[id] = lru_list.begin();

    // 检测容量
    if (lru_list.size() > capacity) {
        PageId old = lru_list.back();
        lru_list.pop_back();
        pos.erase(old);
    }
}
// 选出最久没用的受害者
PageId LRUReplacer::victim() {
    if (lru_list.empty()) return -1;
    PageId id = lru_list.back();
    lru_list.pop_back();
    pos.erase(id);
    return id;
}

void LRUReplacer::erase(PageId id) {
    if (pos.count(id)) {
        lru_list.erase(pos[id]);
        pos.erase(id);
    }
}


BufferPoolManager::BufferPoolManager(size_t pool_size)
    : pool_size(pool_size), frames(pool_size, Page(PAGE_SIZE)), replacer(pool_size) {
}

// 读取页
Page* BufferPoolManager::fetchPage(PageId id) {
    // 1. 查找缓存
    if (page_table.count(id)) {
        int frame_id = page_table[id];
        replacer.access(id);
        frames[frame_id].pin_count++;

        cout << "Page " << id << "in frame " << frame_id << " (hit,  pin_count= " << frames[frame_id].pin_count << ")\n";
        return &frames[frame_id];
    }

    // 2. 没命中，找淘汰对象
    PageId victim_id = -1;
    while (true) {
        PageId cand = replacer.victim();
        if (cand == -1) break;
        int fid = page_table[cand];
        if (frames[fid].pin_count == 0) {
            victim_id = cand;
            break;
        }
    }
    int frame_id = -1;

    if (victim_id != -1) {
        frame_id = page_table[victim_id];
        if (frames[frame_id].dirty) {
            // TODO: 写回磁盘
            cout << "Flush dirty page " << victim_id << " to disk.\n";
        }
        page_table.erase(victim_id);
    }
    else {
        // 如果缓冲池还没满
        for (size_t i = 0; i < pool_size; i++) {
            if (frames[i].id == -1) { frame_id = i; break; }
        }
    }

    if (frame_id == -1) return nullptr; // 没有可用 frame

    // 3. 从磁盘加载新页
    frames[frame_id].id = id;
    frames[frame_id].dirty = false;
    frames[frame_id].pin_count = 1;
    // TODO: 从磁盘写入
    fill(frames[frame_id].data.begin(), frames[frame_id].data.end(), 0);

    page_table[id] = frame_id;
    replacer.access(id);

    cout << "Load page " << id << " from disk.\n";
    return &frames[frame_id];
}

// 解除 pin
void BufferPoolManager::unpinPage(PageId id, bool is_dirty) {
    if (!page_table.count(id)) return;

    int frame_id = page_table[id];
    Page& page = frames[frame_id];

    if (page.pin_count > 0) page.pin_count--;
    if (is_dirty) page.dirty = true;

    cout << "Unpin page " << id << "(pin_count=" << page.pin_count << ")\n";

    if (page.pin_count == 0) {
        replacer.access(id);        // 可被替换
        // TODO: 放到队尾（不是 access）
        // 确保不重复
        //replacer.erase(id);
        //replacer.lru_list.push_back(id);
        //replacer.pos[id] = --replacer.lru_list.end();
    }
}

// 脏页写回磁盘
void BufferPoolManager::flushPage(PageId id) {
    if (!page_table.count(id)) return;

    int frame_id = page_table[id];
    Page& page = frames[frame_id];

    if (page.dirty) {
        // TODO: 写回磁盘
        cout << "Flush page " << id << " to disk.\n";
        page.dirty = false;
    }
}

// 分配新页
Page* BufferPoolManager::newPage(PageId id) {
    return fetchPage(id); // 简化实现，直接调用 fetchPage
}

void BufferPoolManager::test() {
    DiskManager dm("2.test.db");
    DataPage p(PageType::DATA_PAGE);
    
    dm.readPage(0,p);

    cout << p.rawData();
}
#include "buffer_pool.h"

using namespace std;

LRUReplacer::LRUReplacer(size_t c) :capacity(c) {}
// �����ʹ�õ�ҳ�ƶ�������ͷ����ͷΪ���ʹ�õģ���βΪ���δʹ�õ�
void LRUReplacer::access(PageId id) {
    if (pos.count(id)) {
        lru_list.erase(pos[id]);
    }

    lru_list.push_front(id);
    pos[id] = lru_list.begin();

    // �������
    if (lru_list.size() > capacity) {
        PageId old = lru_list.back();
        lru_list.pop_back();
        pos.erase(old);
    }
}
// ��ҳ�ͷţ�ȷ������ʹ��
void LRUReplacer::push(PageId id) {
    // ȷ�����ظ�
    lru_list.erase(pos[id]);
    
    lru_list.push_back(id);
    pos[id] = --lru_list.end();
}
// ѡ�����û�õ��ܺ���
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


BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* d)
    : pool_size(pool_size), replacer(pool_size), disk(d) {
    for (int i = 0; i < pool_size; i++) {
        frames.push_back(Page(PageType::DATA_PAGE));
    }
}

// ��ȡҳ
Page* BufferPoolManager::fetchPage(PageId id) {
    // 1. ���һ���
    if (page_table.count(id)) {
        int frame_id = page_table[id];
        replacer.access(id);
        frames[frame_id].pin_count++;

        cout << "Page " << id << "in frame " << frame_id << " (hit,  pin_count= " << frames[frame_id].pin_count << ")\n";
        return &frames[frame_id];
    }

    // 2. û���У�����̭����
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
            // д�ش���
            disk->writePage(frame_id, frames[frame_id]);
            cout << "Flush dirty page " << victim_id << " to disk.\n";
        }
        page_table.erase(victim_id);
    }
    else {
        // �������ػ�û��
        for (size_t i = 0; i < pool_size; i++) {
            if (frames[i].id == -1) { frame_id = i; break; }
        }
    }

    if (frame_id == -1) return nullptr; // û�п��� frame

    // 3. �Ӵ��̼�����ҳ
    frames[frame_id].id = id;
    frames[frame_id].dirty = false;
    frames[frame_id].pin_count = 1;
    // TODO: �Ӵ���д��
    disk->readPage(id, frames[frame_id]);

    page_table[id] = frame_id;
    replacer.access(id);

    cout << "Load page " << id << " from disk.\n";
    return &frames[frame_id];
}

// ��� pin
void BufferPoolManager::unpinPage(PageId id, bool is_dirty) {
    if (!page_table.count(id)) return;

    int frame_id = page_table[id];
    Page& page = frames[frame_id];

    if (page.pin_count > 0) page.pin_count--;
    if (is_dirty) page.dirty = true;

    cout << "Unpin page " << id << "(pin_count=" << page.pin_count << ")\n";

    if (page.pin_count == 0) {
        replacer.push(id);
    }
}

// ��ҳд�ش���
void BufferPoolManager::flushPage(PageId id) {
    if (!page_table.count(id)) return;

    int frame_id = page_table[id];
    Page& page = frames[frame_id];

    if (page.dirty) {
        // TODO: д�ش���
        disk->writePage(id, page);
        cout << "Flush page " << id << " to disk.\n";
        page.dirty = false;
    }
}

// ������ҳ
Page* BufferPoolManager::newPage(PageId id) {
    return fetchPage(id); // ��ʵ�֣�ֱ�ӵ��� fetchPage
}

// ����BufferPoolManager�Ļ�������
void BufferPoolManager::test() {
    DiskManager dm("1.test.db");
    Page p(PageType::DATA_PAGE);
    
    dm.readPage(0,p);

    cout << p.rawData();
}
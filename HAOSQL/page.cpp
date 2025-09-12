#include "page.h"
using namespace std;

Page::Page(PageType t, uint32_t id) : id(id), dirty(false), pin_count(0) {
    std::memset(data, 0, PAGE_SIZE);
    PageHeader* ph = reinterpret_cast<PageHeader*>(data);
    ph->type = t;
    ph->page_id = id;
    ph->free_offset = sizeof(PageHeader);
    ph->slot_count = 0;
}

// ҳ�ռ�̶����ֶ�����ҳͷ
PageHeader* Page::header() {
    return reinterpret_cast<PageHeader*>(data);
}
const PageHeader* Page::header() const {
    return reinterpret_cast<const PageHeader*>(data);
}

// ҳ�ռ�̶����ֶ���ȡ idx λ�õ�Slot����
Slot* Page::getSlot(int idx) {
    if (idx < 0 || idx >= header()->slot_count) {
        throw out_of_range("Invalid slot index");
    }
    return reinterpret_cast<Slot*>(
        data + PAGE_SIZE - (idx + 1) * sizeof(Slot)
        );
}
const Slot* Page::getSlot(int idx) const {
    if (idx < 0 || idx >= header()->slot_count) {
        throw std::out_of_range("Invalid slot index");
    }
    return reinterpret_cast<const Slot*>(
        data + PAGE_SIZE - (idx + 1) * sizeof(Slot)
        );
}

// ����һ����¼������ slot �±�
int Page::insertRecord(const char* record, uint16_t len) {
    int slotCount = header()->slot_count;
    uint16_t freeOffset = header()->free_offset;

    // �����Ŀ¼λ��
    uint16_t slotDirPos = PAGE_SIZE - (slotCount + 1) * sizeof(Slot);

    // ���ռ��Ƿ��㹻
    if (freeOffset + len > slotDirPos) {
        throw runtime_error("Not enough space in page");
    }

    // �������ݵ�������
    memcpy(data + freeOffset, record, len);

    // ���� slot
    Slot* slot = reinterpret_cast<Slot*>(data + slotDirPos);
    slot->offset = freeOffset;
    slot->length = len;

    // ���� header
    header()->free_offset += len;
    header()->slot_count++;

    return slotCount; // ���� slot �±�
}

// ��ȡ��¼
string Page::readRecord(int idx) const {
    const Slot* slot = getSlot(idx);
    return string(data + slot->offset, slot->length);
}

// ɾ����¼�����ɾ����
void Page::deleteRecord(int idx) {
    Slot* slot = getSlot(idx);
    slot->length = 0; // �߼�ɾ��
}

// ѹ�����������Ƴ��ն���
void Page::compact() {
    uint16_t newOffset = sizeof(PageHeader);
    std::vector<std::pair<int, Slot>> validSlots;
    int idx;
    Slot slotCopy = {};

    // �ռ�������Ч slot
    for (int i = 0; i < header()->slot_count; i++) {
        Slot* slot = getSlot(i);
        if (slot->length > 0) {
            validSlots.push_back({ i, *slot });
        }
    }

    // ������������
    for (auto& p : validSlots) {
        idx = p.first;
        slotCopy = p.second;
        if (slotCopy.offset != newOffset) {
            memmove(data + newOffset, data + slotCopy.offset, slotCopy.length);
        }
        Slot* slot = getSlot(idx);
        slot->offset = newOffset;
        slot->length = slotCopy.length;
        newOffset += slotCopy.length;
    }

    // ���� free_offset
    header()->free_offset = newOffset;
}

char* Page::rawData() { return data; }
const char* Page::rawData() const { return data; }


DiskManager::DiskManager(std::string d) : diskName(d) {}
DiskManager* DiskManager::readPage(int pageId, Page& pageData) {
    std::ifstream f(diskName, std::ios::in | std::ios::binary);
    char* data = new(char[PAGE_SIZE]);

    if (!f.is_open()) {
        std::cerr << "[ERROR] Disk " << diskName << " open failed for read.\n";
        return this;
    }

    // ��λ���ļ��е�offset
    f.seekg(pageId * PAGE_SIZE, std::ios::beg);

    // �Ӵ��̶�ȡ PAGE_SIZE �ֽڵ��ڴ�
    f.read(data, PAGE_SIZE);
    // ʣ�಻�� PAGE_SIZE ,��0����
    if (f.gcount() < (int)PAGE_SIZE) {
        memset(data + f.gcount(), 0, PAGE_SIZE - f.gcount());
    }

    f.close();

    memcpy(pageData.rawData(), data, PAGE_SIZE);

    return this;
}

DiskManager* DiskManager::writePage(int pageId, Page& pageData) {
    std::ofstream f(diskName, std::ios::out | std::ios::binary);
    
    std::streampos pos = static_cast<std::streampos>(pageId) * PAGE_SIZE;
    f.seekp(pos, std::ios::beg);

    f.write(pageData.rawData(), PAGE_SIZE);
    f.flush();

    if (!f.good()) {
        throw std::runtime_error("DiskManager::writePage failed at pageId=" + pageId);
    }

    return this;
}
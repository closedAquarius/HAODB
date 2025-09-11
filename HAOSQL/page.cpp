#include "page.h"
using namespace std;

DataPage::DataPage(PageType type) {
    memset(data, 0, PAGE_SIZE);
    header()->type = type;
    header()->page_id = 0;
    header()->free_offset = sizeof(PageHeader);
    header()->slot_count = 0;

}

// ҳ�ռ�̶����ֶ�����ҳͷ
PageHeader* DataPage::header() {
    return reinterpret_cast<PageHeader*>(data);
}
const PageHeader* DataPage::header() const {
    return reinterpret_cast<const PageHeader*>(data);
}

// ҳ�ռ�̶����ֶ���ȡ idx λ�õ�Slot����
Slot* DataPage::getSlot(int idx) {
    if (idx < 0 || idx >= header()->slot_count) {
        throw out_of_range("Invalid slot index");
    }
    return reinterpret_cast<Slot*>(
        data + PAGE_SIZE - (idx + 1) * sizeof(Slot)
        );
}
const Slot* DataPage::getSlot(int idx) const {
    if (idx < 0 || idx >= header()->slot_count) {
        throw std::out_of_range("Invalid slot index");
    }
    return reinterpret_cast<const Slot*>(
        data + PAGE_SIZE - (idx + 1) * sizeof(Slot)
        );
}

// ����һ����¼������ slot �±�
int DataPage::insertRecord(const char* record, uint16_t len) {
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
string DataPage::readRecord(int idx) const {
    const Slot* slot = getSlot(idx);
    return string(data + slot->offset, slot->length);
}

// ɾ����¼�����ɾ����
void DataPage::deleteRecord(int idx) {
    Slot* slot = getSlot(idx);
    slot->length = 0; // �߼�ɾ��
}

// ѹ�����������Ƴ��ն���
void DataPage::compact() {
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

char* DataPage::rawData() { return data; }
const char* DataPage::rawData() const { return data; }
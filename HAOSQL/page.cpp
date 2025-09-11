#include "page.h"
using namespace std;

DataPage::DataPage(PageType type) {
    memset(data, 0, PAGE_SIZE);
    header()->type = type;
    header()->page_id = 0;
    header()->free_offset = sizeof(PageHeader);
    header()->slot_count = 0;

}

// 页空间固定，手动控制页头
PageHeader* DataPage::header() {
    return reinterpret_cast<PageHeader*>(data);
}
const PageHeader* DataPage::header() const {
    return reinterpret_cast<const PageHeader*>(data);
}

// 页空间固定，手动获取 idx 位置的Slot数据
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

// 插入一条记录，返回 slot 下标
int DataPage::insertRecord(const char* record, uint16_t len) {
    int slotCount = header()->slot_count;
    uint16_t freeOffset = header()->free_offset;

    // 计算槽目录位置
    uint16_t slotDirPos = PAGE_SIZE - (slotCount + 1) * sizeof(Slot);

    // 检查空间是否足够
    if (freeOffset + len > slotDirPos) {
        throw runtime_error("Not enough space in page");
    }

    // 拷贝数据到数据区
    memcpy(data + freeOffset, record, len);

    // 更新 slot
    Slot* slot = reinterpret_cast<Slot*>(data + slotDirPos);
    slot->offset = freeOffset;
    slot->length = len;

    // 更新 header
    header()->free_offset += len;
    header()->slot_count++;

    return slotCount; // 返回 slot 下标
}

// 读取记录
string DataPage::readRecord(int idx) const {
    const Slot* slot = getSlot(idx);
    return string(data + slot->offset, slot->length);
}

// 删除记录（标记删除）
void DataPage::deleteRecord(int idx) {
    Slot* slot = getSlot(idx);
    slot->length = 0; // 逻辑删除
}

// 压缩数据区（移除空洞）
void DataPage::compact() {
    uint16_t newOffset = sizeof(PageHeader);
    std::vector<std::pair<int, Slot>> validSlots;
    int idx;
    Slot slotCopy = {};

    // 收集所有有效 slot
    for (int i = 0; i < header()->slot_count; i++) {
        Slot* slot = getSlot(i);
        if (slot->length > 0) {
            validSlots.push_back({ i, *slot });
        }
    }

    // 重新排列数据
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

    // 更新 free_offset
    header()->free_offset = newOffset;
}

char* DataPage::rawData() { return data; }
const char* DataPage::rawData() const { return data; }
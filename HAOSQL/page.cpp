#include "page.h"

Page::Page(PageType t, uint32_t id) {
    header.type = t;
    header.page_id = id;
    header.free_offset = 0;
    header.record_count = 0;
    std::memset(data, 0, sizeof(data));
    std::memset(slots, 0, sizeof(slots));
}

// 插入一条记录，返回是否成功
bool Page::insertRecord(const std::string& record) {
    int len = record.size();
    // 检查是否有足够空间存放记录 + slot
    if (header.free_offset + len > sizeof(data) || header.record_count >= 100) {
        return false; // 空间不足
    }

    // 记录写入 Data 区
    std::memcpy(data + header.free_offset, record.c_str(), len);

    // 更新 slot 信息
    slots[header.record_count].offset = header.free_offset;
    slots[header.record_count].length = len;

    // 更新页头
    header.free_offset += len;
    header.record_count++;

    return true;
}

// 根据 slot_id 获取记录
std::string Page::getRecord(int slot_id) const {
    if (slot_id >= header.record_count) return "";
    const Slot& s = slots[slot_id];
    return std::string(data + s.offset, s.length);
}

// 写页到磁盘（只写固定大小页，不写指针）
void Page::writeToDisk(const std::string& filename) {
    std::fstream fs(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!fs.is_open()) {
        // 文件不存在就创建
        fs.open(filename, std::ios::out | std::ios::binary);
        fs.close();
        fs.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }
    fs.seekp(header.page_id * PAGE_SIZE);
    // 先写页头
    fs.write(reinterpret_cast<char*>(&header), sizeof(header));
    // 写 slot 数组
    fs.write(reinterpret_cast<char*>(slots), sizeof(slots));
    // 写 data 区
    fs.write(data, sizeof(data));
    fs.close();
}

// 从磁盘读取页
Page Page::readFromDisk(const std::string& filename, uint32_t page_id) {
    Page page(DATA_PAGE, page_id);
    std::ifstream fs(filename, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        std::cerr << "File not found: " << filename << std::endl;
        return page;
    }

    fs.seekg(page_id * PAGE_SIZE);
    fs.read(reinterpret_cast<char*>(&page.header), sizeof(page.header));
    fs.read(reinterpret_cast<char*>(page.slots), sizeof(page.slots));
    fs.read(page.data, sizeof(page.data));
    fs.close();
    return page;
}

// 打印页信息（调试用）
void Page::printInfo() const {
    std::cout << "Page ID: " << header.page_id
        << ", Type: " << header.type
        << ", Records: " << header.record_count
        << ", Free Offset: " << header.free_offset << std::endl;
    for (int i = 0; i < header.record_count; ++i) {
        std::cout << "Slot " << i << ": offset=" << slots[i].offset
            << ", length=" << slots[i].length
            << ", data=" << getRecord(i) << std::endl;
    }
}

char* Page::getData() {
    return data;
}
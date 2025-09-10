#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

const size_t PAGE_SIZE = 4096;

enum PageType {
    DATA_PAGE,
    INDEX_PAGE,
    META_PAGE
};

// 页头信息
struct PageHeader {
    PageType type;        // 页类型
    uint32_t page_id;     // 页号
    uint16_t free_offset; // 数据区下一个可写偏移
    uint16_t record_count;// 当前记录数
};

// Slot结构，用于记录页内每条记录偏移和长度
struct Slot {
    uint16_t offset;  // Data区偏移
    uint16_t length;  // 记录长度
};

class Page {
public:
    PageHeader header;
    Slot slots[100];          // 最多100条记录（可调整或动态）
    char data[PAGE_SIZE - sizeof(PageHeader) - sizeof(slots)];
public:
    Page(PageType t = DATA_PAGE, uint32_t id = 0);

    bool insertRecord(const std::string& record);
    std::string getRecord(int slot_id) const;

    void writeToDisk(const std::string& filename);
    static Page readFromDisk(const std::string& filename, uint32_t page_id);

    void printInfo() const;
};

#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

// 页大小固定为4KB
const size_t PAGE_SIZE = 4096;

enum PageType {
    DATA_PAGE,
    INDEX_PAGE,
    META_PAGE
};

// 页头信息
struct PageHeader {
    PageType type;          // 页类型
    uint32_t page_id;       // 页号
    uint16_t free_offset;   // 数据区下一个可写位置(从头往后生长)
    uint16_t slot_count;    // 槽数量

};

// 槽结构，位于页尾
struct Slot {
    uint16_t offset;  // 数据区偏移
    uint16_t length;  // 记录长度
};

using PageId = int;
class Page {
public:
    char data[PAGE_SIZE];   //整个页空间
public:
    PageId id;
    bool dirty;
    int pin_count;

    Page(PageType type);

    // 页空间固定，手动控制页头
    PageHeader* header();
    const PageHeader* header() const;

    // 页空间固定，手动获取 idx 位置的Slot数据
    Slot* getSlot(int idx);
    const Slot* getSlot(int idx) const;

    int insertRecord(const char* record, uint16_t len);
    std::string readRecord(int idx) const;
    void deleteRecord(int idx);
    void compact();

    char* rawData();
    const char* rawData() const;
};

class DiskManager {
private:
    std::string diskName;
public:
    DiskManager(std::string d);
    DiskManager* readPage(int pageId, Page& pageData);
    DiskManager* writePage(int pageId, Page& pageData);
};
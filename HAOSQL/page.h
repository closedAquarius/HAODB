#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

// ҳ��С�̶�Ϊ4KB
const size_t PAGE_SIZE = 4096;

enum PageType {
    DATA_PAGE,
    INDEX_PAGE,
    META_PAGE
};

// ҳͷ��Ϣ
struct PageHeader {
    PageType type;          // ҳ����
    uint32_t page_id;       // ҳ��
    uint16_t free_offset;   // ��������һ����дλ��(��ͷ��������)
    uint16_t slot_count;    // ������

};

// �۽ṹ��λ��ҳβ
struct Slot {
    uint16_t offset;  // ������ƫ��
    uint16_t length;  // ��¼����
};

using PageId = int;
class Page {
public:
    char data[PAGE_SIZE];   //����ҳ�ռ�
public:
    PageId id;
    bool dirty;
    int pin_count;

    Page(PageType type);

    // ҳ�ռ�̶����ֶ�����ҳͷ
    PageHeader* header();
    const PageHeader* header() const;

    // ҳ�ռ�̶����ֶ���ȡ idx λ�õ�Slot����
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
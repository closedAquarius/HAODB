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

// ҳͷ��Ϣ
struct PageHeader {
    PageType type;        // ҳ����
    uint32_t page_id;     // ҳ��
    uint16_t free_offset; // ��������һ����дƫ��
    uint16_t record_count;// ��ǰ��¼��
};

// Slot�ṹ�����ڼ�¼ҳ��ÿ����¼ƫ�ƺͳ���
struct Slot {
    uint16_t offset;  // Data��ƫ��
    uint16_t length;  // ��¼����
};

class Page {
public:
    PageHeader header;
    Slot slots[100];          // ���100����¼���ɵ�����̬��
    char data[PAGE_SIZE - sizeof(PageHeader) - sizeof(slots)];
public:
    Page(PageType t = DATA_PAGE, uint32_t id = 0);

    bool insertRecord(const std::string& record);
    std::string getRecord(int slot_id) const;

    void writeToDisk(const std::string& filename);
    static Page readFromDisk(const std::string& filename, uint32_t page_id);

    void printInfo() const;
};

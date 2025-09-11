#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

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

class DataPage {
public:
    char data[PAGE_SIZE];   //����ҳ�ռ�
public:
    DataPage(PageType type);

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

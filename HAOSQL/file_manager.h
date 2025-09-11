// file_manager.h
#pragma once
#include <fstream>
#include <unordered_map>
#include <iostream>
#include <sys/stat.h>
#include "page.h"

class FileManager {
public:
    FileManager(const std::string& db_dir);

    // �򿪻򴴽���ռ��ļ�
    int openFile(const std::string& tablespace_name);

    // �Ӵ��̶�һҳ
    bool readPage(int file_id, uint32_t page_id, Page& page);

    // дһҳ������
    bool writePage(int file_id, const Page& page);

    // ��ȡ�ļ���С��ҳ����
    uint32_t getPageCount(int file_id);

    int allocatePage(int file_id, PageType type);     // ����һ����ҳ��INDEX_PAGE / DATA_PAGE��
    void freePage(int file_id, uint32_t pageId);      // �ͷ�ĳ��ҳ

    void readPageHeader(int file_id, uint32_t pageId, PageHeader& ph);
    void writePageHeader(int file_id, uint32_t pageId, const PageHeader& ph);

private:
    std::string db_dir;
    std::unordered_map<int, std::fstream*> open_files;
    int next_file_id = 1;
};
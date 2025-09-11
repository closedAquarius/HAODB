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
    bool readPage(int file_id, uint32_t page_id, DataPage& page);

    // дһҳ������
    bool writePage(int file_id, const DataPage& page);

    // ��ȡ�ļ���С��ҳ����
    uint32_t getPageCount(int file_id);

private:
    std::string db_dir;
    std::unordered_map<int, std::fstream*> open_files;
    int next_file_id = 1;
};

class DiskManager {
private:
    std::string diskName;
public:
    DiskManager(std::string d);
    DiskManager* readPage(int pageId, DataPage& pageData);
    DiskManager* writePage(int pageId, DataPage& pageData);
};
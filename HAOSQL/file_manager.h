// file_manager.h
#pragma once
#include "page.h"
#include <string>
#include <fstream>
#include <unordered_map>

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

private:
    std::string db_dir;
    std::unordered_map<int, std::fstream*> open_files;
    int next_file_id = 1;
};
#pragma once

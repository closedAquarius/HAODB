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

    // 打开或创建表空间文件
    int openFile(const std::string& tablespace_name);

    // 从磁盘读一页
    bool readPage(int file_id, uint32_t page_id, DataPage& page);

    // 写一页到磁盘
    bool writePage(int file_id, const DataPage& page);

    // 获取文件大小（页数）
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
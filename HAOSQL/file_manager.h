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
    bool readPage(int file_id, uint32_t page_id, Page& page);

    // 写一页到磁盘
    bool writePage(int file_id, const Page& page);

    // 获取文件大小（页数）
    uint32_t getPageCount(int file_id);

    int allocatePage(int file_id, PageType type);     // 分配一个新页（INDEX_PAGE / DATA_PAGE）
    void freePage(int file_id, uint32_t pageId);      // 释放某个页

    void readPageHeader(int file_id, uint32_t pageId, PageHeader& ph);
    void writePageHeader(int file_id, uint32_t pageId, const PageHeader& ph);

private:
    std::string db_dir;
    std::unordered_map<int, std::fstream*> open_files;
    int next_file_id = 1;
};
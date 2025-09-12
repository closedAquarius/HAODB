// file_manager.cpp
#include "file_manager.h"

FileManager::FileManager(const std::string& db_dir) : db_dir(db_dir) {}

int FileManager::openFile(const std::string& tablespace_name) {
    std::string filename = db_dir + "/" + tablespace_name;
    auto fs = new std::fstream(filename,
        std::ios::in | std::ios::out | std::ios::binary);

    if (!fs->is_open()) {
        // 文件不存在则创建
        fs->open(filename, std::ios::out | std::ios::binary);
        fs->close();
        fs->open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }

    int file_id = next_file_id++;
    open_files[file_id] = fs;
    return file_id;
}

bool FileManager::readPage(int file_id, uint32_t page_id, Page& page) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return false;
    std::fstream* fs = it->second;

    fs->seekg(page_id * PAGE_SIZE, std::ios::beg);
    fs->read(page.rawData(), PAGE_SIZE);
    return fs->good();
}

bool FileManager::writePage(int file_id, const Page& page) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return false;
    std::fstream* fs = it->second;

    fs->seekp(page.header()->page_id * PAGE_SIZE, std::ios::beg);
    fs->write(page.rawData(), PAGE_SIZE);
    fs->flush();

    return fs->good();
}

uint32_t FileManager::getPageCount(int file_id) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return 0;
    std::fstream* fs = it->second;

    fs->seekg(0, std::ios::end);
    std::streampos size = fs->tellg();
    return size / PAGE_SIZE;
}
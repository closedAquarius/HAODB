// file_manager.cpp
#include "file_manager.h"

FileManager::FileManager(const std::string& db_dir) : db_dir(db_dir) {}

int FileManager::openFile(const std::string& tablespace_name) {
    std::string filename = db_dir + "/" + tablespace_name + ".tbl";
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

    fs->seekg(page_id * PAGE_SIZE);
    fs->read(reinterpret_cast<char*>(&page.header), sizeof(page.header));
    fs->read(reinterpret_cast<char*>(page.slots), sizeof(page.slots));
    fs->read(page.data, sizeof(page.data));
    return true;
}

bool FileManager::writePage(int file_id, const Page& page) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return false;
    std::fstream* fs = it->second;

    fs->seekp(page.header.page_id * PAGE_SIZE);
    fs->write(reinterpret_cast<const char*>(&page.header), sizeof(page.header));
    fs->write(reinterpret_cast<const char*>(page.slots), sizeof(page.slots));
    fs->write(page.data, sizeof(page.data));
    fs->flush();
    return true;
}

uint32_t FileManager::getPageCount(int file_id) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return 0;
    std::fstream* fs = it->second;

    fs->seekg(0, std::ios::end);
    std::streampos size = fs->tellg();
    return size / PAGE_SIZE;
}

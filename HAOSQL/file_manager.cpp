// file_manager.cpp
#include "file_manager.h"

FileManager::FileManager(const std::string& db_dir) : db_dir(db_dir) {}

int FileManager::openFile(const std::string& tablespace_name) {
    std::string filename = db_dir + "/" + tablespace_name + ".tbl";
    auto fs = new std::fstream(filename,
        std::ios::in | std::ios::out | std::ios::binary);

    if (!fs->is_open()) {
        // �ļ��������򴴽�
        fs->open(filename, std::ios::out | std::ios::binary);
        fs->close();
        fs->open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }

    int file_id = next_file_id++;
    open_files[file_id] = fs;
    return file_id;
}

bool FileManager::readPage(int file_id, uint32_t page_id, DataPage& page) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return false;
    std::fstream* fs = it->second;

    fs->seekg(page_id * PAGE_SIZE, std::ios::beg);
    fs->read(page.rawData(), PAGE_SIZE);
    return fs->good();
}

bool FileManager::writePage(int file_id, const DataPage& page) {
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


DiskManager::DiskManager(std::string d) : diskName(d) {}
DiskManager* DiskManager::readPage(int pageId, DataPage& pageData) {
    std::ifstream f(diskName, std::ios::in | std::ios::binary);
    char* data = new(char[PAGE_SIZE]);

    if (!f.is_open()) {
        std::cerr << "[ERROR] Disk " << diskName << " open failed for read.\n";
        return this;
    }

    // ��λ���ļ��е�offset
    f.seekg(pageId * PAGE_SIZE, std::ios::beg);

    // �Ӵ��̶�ȡ PAGE_SIZE �ֽڵ��ڴ�
    f.read(data, PAGE_SIZE);
    // ʣ�಻�� PAGE_SIZE ,��0����
    if (f.gcount() < (int)PAGE_SIZE) {
        memset(data + f.gcount(), 0, PAGE_SIZE - f.gcount());
    }

    f.close();

    memcpy(pageData.rawData(), data, PAGE_SIZE);

    return this;
}

DiskManager* DiskManager::writePage(int pageId, DataPage& pageData) {

    return this;
}
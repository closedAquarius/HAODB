// file_manager.cpp
#include "file_manager.h"

FileManager::FileManager(const std::string& db_dir) : db_dir(db_dir) {}

int FileManager::openFile(const std::string& tablespace_name) {
    std::cout << "param=" << tablespace_name << std::endl;
    std::cout << "db_dir=" << db_dir << std::endl;
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


void FileManager::readPageHeader(int file_id, uint32_t pageId, PageHeader& ph) {
    Page p;  // 不指定类型，因为我们是要读取已有页
    readPage(file_id, pageId, p);
    std::memcpy(&ph, p.data, sizeof(PageHeader));
}

void FileManager::writePageHeader(int file_id, uint32_t pageId, const PageHeader& ph) {
    Page p;  // 新建一个 Page
    p.id = pageId;
    std::memcpy(p.data, &ph, sizeof(PageHeader));
    writePage(file_id, p);
}

uint32_t FileManager::getPageCount(int file_id) {
    auto it = open_files.find(file_id);
    if (it == open_files.end()) return 0;
    std::fstream* fs = it->second;

    fs->seekg(0, std::ios::end);
    std::streampos size = fs->tellg();

    // 文件大小除以 PAGE_SIZE 得到已有页数
    return static_cast<uint32_t>(size / PAGE_SIZE);
}

int FileManager::allocatePage(int file_id, PageType type) {
    uint32_t pageCount = getPageCount(file_id);

    // 遍历已有页，找空闲页（从 0 开始，保证所有页都能复用）
    for (uint32_t pid = 0; pid < pageCount; pid++) {
        PageHeader ph;
        readPageHeader(file_id, pid, ph);
        if (ph.type == FREE_PAGE) {
            ph.type = type;
            ph.free_offset = sizeof(PageHeader);
            ph.slot_count = 0;
            writePageHeader(file_id, pid, ph);
            return pid;   // 直接返回复用的页号
        }
    }

    // 没有空闲页 → 在文件末尾扩展新页
    PageHeader newPage;
    newPage.type = type;
    newPage.page_id = pageCount;
    newPage.free_offset = sizeof(PageHeader);
    newPage.slot_count = 0;
    writePageHeader(file_id, pageCount, newPage);

    return pageCount;  // 新分配的页号
}

void FileManager::freePage(int file_id, uint32_t pageId) {
    PageHeader ph;
    ph.type = FREE_PAGE;
    ph.page_id = pageId;
    ph.free_offset = sizeof(PageHeader);
    ph.slot_count = 0;
    writePageHeader(file_id, pageId, ph);
}
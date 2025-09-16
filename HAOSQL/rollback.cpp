#include "rollback.h"

char* Log::rawData() {
    // 1. 计算总大小
    // sizeof(time_t) = 8
    // sizeof(uint16_t) = 2
    // sizeof(OperationType) = 4
    size_t total_size = 18;

    // 2. 动态分配内存
    char* buffer = new char[total_size];

    // 3. 逐个复制成员数据到缓冲区
    size_t offset = 0;

    // 复制 pageId
    memcpy(buffer + offset, &pageId, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    // 复制 slotId
    memcpy(buffer + offset, &slotId, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    // 复制 len
    memcpy(buffer + offset, &len, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    // 复制 type
    memcpy(buffer + offset, &type, sizeof(int));

    return buffer;
}
const char* Log::rawData() const {
    // 调用非const版本的rawData()，并返回const指针
    return const_cast<Log*>(this)->rawData();
}

// 从二进制数据重建 Log 结构体
Log fromRawData(const char* buffer) {
    Log log;
    size_t offset = 0;

    // 从缓冲区中按序复制固定长度的成员
    memcpy(&log.pageId, buffer + offset, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    memcpy(&log.slotId, buffer + offset, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    memcpy(&log.len, buffer + offset, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    memcpy(&log.type, buffer + offset, sizeof(int));

    return log;
}

void writeLog(int type, int pageId, int slotId) {
    // 假设日志文件名为 "database.log"
    std::ofstream logFile("database.log", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not open log file." << std::endl;
        return;
    }

    Log newLog;
    newLog.type = type;
    newLog.pageId = pageId;
    newLog.slotId = slotId;

    logFile.write(newLog.rawData(), 18);
    logFile.close();
}

// 核心函数：根据日志记录撤销单个操作
void undo(const Log& log, BufferPoolManager* bpm) {
    Page* page = bpm->fetchPage(log.pageId);
    if (!page) {
        throw std::runtime_error("Failed to fetch page for rollback.");
    }

    switch (log.type) {
    case 1:
        // 撤销插入：删除该记录
        page->deleteRecord(log.slotId);
        std::cout << "Rollback INSERT: Deleted record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;

    case 2:
        // 撤销删除：恢复该记录
        page->getSlot(log.slotId)->length = log.len;
        std::cout << "Rollback DELETE: Restored record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;
    case 4:
        // 撤销插入：删除该记录
        page->deleteRecord(log.slotId);
        std::cout << "Rollback INSERT: Deleted record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;

    case 3:
        // 撤销删除：恢复该记录
        page->getSlot(log.slotId)->length = log.len;
        std::cout << "Rollback DELETE: Restored record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;
    }

    bpm->unpinPage(log.pageId, true); // 撤销操作后，将页标记为脏
    bpm->flushPage(log.pageId);    // 手动写回
}

void undos(vector<Log>& logs, BufferPoolManager* bpm) {
    for (auto& l : logs) {
        undo(l, bpm);
    }
}

//// 遍历日志文件并执行回滚
//void startRollback() {
//    std::ifstream logFile("database.log", std::ios::in | std::ios::binary | std::ios::ate);
//    if (!logFile.is_open()) {
//        std::cerr << "Log file not found, no rollback needed.\n";
//        return;
//    }
//
//    std::streamsize fileSize = logFile.tellg();
//    logFile.seekg(0, std::ios::beg);
//
//    while (logFile.tellg() < fileSize) {
//        // 读取固定大小的头部
//        size_t fixed_header_size = sizeof(Log) - sizeof(std::string);
//        char* headerBuffer = new(char[fixed_header_size]);
//        if (!logFile.read(headerBuffer, fixed_header_size)) {
//            break;
//        }
//
//        Log log;
//        memcpy(&log.date, headerBuffer, sizeof(time_t));
//        memcpy(&log.pageId, headerBuffer + sizeof(time_t), sizeof(int));
//        memcpy(&log.slotId, headerBuffer + sizeof(time_t) + sizeof(int), sizeof(int));
//        memcpy(&log.len, headerBuffer + sizeof(time_t) + sizeof(int) * 2, sizeof(int));
//        memcpy(&log.type, headerBuffer + sizeof(time_t) + sizeof(int) * 3, sizeof(OperationType));
//
//        undo(log); // 执行撤销
//
//        delete[] headerBuffer;
//    }
//
//    logFile.close();
//}
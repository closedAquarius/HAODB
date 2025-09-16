#include "rollback.h"

char* Log::rawData() {
    // 1. �����ܴ�С
    // sizeof(time_t) = 8
    // sizeof(uint16_t) = 2
    // sizeof(OperationType) = 4
    size_t total_size = 18;

    // 2. ��̬�����ڴ�
    char* buffer = new char[total_size];

    // 3. ������Ƴ�Ա���ݵ�������
    size_t offset = 0;

    // ���� pageId
    memcpy(buffer + offset, &pageId, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    // ���� slotId
    memcpy(buffer + offset, &slotId, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    // ���� len
    memcpy(buffer + offset, &len, SIZEOFUINT16_T);
    offset += SIZEOFUINT16_T;

    // ���� type
    memcpy(buffer + offset, &type, sizeof(int));

    return buffer;
}
const char* Log::rawData() const {
    // ���÷�const�汾��rawData()��������constָ��
    return const_cast<Log*>(this)->rawData();
}

// �Ӷ����������ؽ� Log �ṹ��
Log fromRawData(const char* buffer) {
    Log log;
    size_t offset = 0;

    // �ӻ������а����ƹ̶����ȵĳ�Ա
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
    // ������־�ļ���Ϊ "database.log"
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

// ���ĺ�����������־��¼������������
void undo(const Log& log, BufferPoolManager* bpm) {
    Page* page = bpm->fetchPage(log.pageId);
    if (!page) {
        throw std::runtime_error("Failed to fetch page for rollback.");
    }

    switch (log.type) {
    case 1:
        // �������룺ɾ���ü�¼
        page->deleteRecord(log.slotId);
        std::cout << "Rollback INSERT: Deleted record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;

    case 2:
        // ����ɾ�����ָ��ü�¼
        page->getSlot(log.slotId)->length = log.len;
        std::cout << "Rollback DELETE: Restored record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;
    case 4:
        // �������룺ɾ���ü�¼
        page->deleteRecord(log.slotId);
        std::cout << "Rollback INSERT: Deleted record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;

    case 3:
        // ����ɾ�����ָ��ü�¼
        page->getSlot(log.slotId)->length = log.len;
        std::cout << "Rollback DELETE: Restored record at (" << log.pageId << ", " << log.slotId << ")\n";
        break;
    }

    bpm->unpinPage(log.pageId, true); // ���������󣬽�ҳ���Ϊ��
    bpm->flushPage(log.pageId);    // �ֶ�д��
}

void undos(vector<Log>& logs, BufferPoolManager* bpm) {
    for (auto& l : logs) {
        undo(l, bpm);
    }
}

//// ������־�ļ���ִ�лع�
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
//        // ��ȡ�̶���С��ͷ��
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
//        undo(log); // ִ�г���
//
//        delete[] headerBuffer;
//    }
//
//    logFile.close();
//}
#pragma once
#include <unordered_map>
#include <set>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

class LockManager {
public:
    enum LockType { SHARED, EXCLUSIVE };

    struct LockRequest {
        int tx_id;
        LockType type;
    };

    struct LockEntry {
        std::set<int> shared_txns;           // ��ǰ���й�����������ID
        int exclusive_txn = -1;              // ��ǰ����������������ID (-1��ʾ��)
        std::queue<LockRequest> waiting_queue; // �ȴ���������
    };

private:
    std::unordered_map<std::string, LockEntry> lock_table; // key: ���ݶ���ID (����/��ID)
    std::mutex table_mutex; // ȫ�������� lock_table
    std::condition_variable cv;

public:
    LockManager() = default;
    ~LockManager() = default;

    // ��ȡ������
    void acquireSharedLock(int tx_id, const std::string& object_id);

    // ��ȡ������
    void acquireExclusiveLock(int tx_id, const std::string& object_id);

    // �ͷ�������е�������
    void releaseLocks(int tx_id);

    // ���ԣ���ӡ����
    void printLockTable();
};

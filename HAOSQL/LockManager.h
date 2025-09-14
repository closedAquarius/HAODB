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
        std::set<int> shared_txns;           // 当前持有共享锁的事务ID
        int exclusive_txn = -1;              // 当前持有排他锁的事务ID (-1表示无)
        std::queue<LockRequest> waiting_queue; // 等待锁的请求
    };

private:
    std::unordered_map<std::string, LockEntry> lock_table; // key: 数据对象ID (表名/行ID)
    std::mutex table_mutex; // 全局锁保护 lock_table
    std::condition_variable cv;

public:
    LockManager() = default;
    ~LockManager() = default;

    // 获取共享锁
    void acquireSharedLock(int tx_id, const std::string& object_id);

    // 获取排他锁
    void acquireExclusiveLock(int tx_id, const std::string& object_id);

    // 释放事务持有的所有锁
    void releaseLocks(int tx_id);

    // 调试：打印锁表
    void printLockTable();
};

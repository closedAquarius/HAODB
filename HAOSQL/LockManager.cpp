#include "LockManager.h"
#include <iostream>

void LockManager::acquireSharedLock(int tx_id, const std::string& object_id) {
    std::unique_lock<std::mutex> lk(table_mutex);
    LockEntry& entry = lock_table[object_id];

    while (entry.exclusive_txn != -1 && entry.exclusive_txn != tx_id) {
        entry.waiting_queue.push({ tx_id, SHARED });
        cv.wait(lk, [&]() {
            return entry.exclusive_txn == -1 || entry.exclusive_txn == tx_id;
            });
    }
    entry.shared_txns.insert(tx_id);
}

void LockManager::acquireExclusiveLock(int tx_id, const std::string& object_id) {
    std::unique_lock<std::mutex> lk(table_mutex);
    LockEntry& entry = lock_table[object_id];

    while ((entry.exclusive_txn != -1 && entry.exclusive_txn != tx_id) ||
        (!entry.shared_txns.empty() && !(entry.shared_txns.size() == 1 && entry.shared_txns.count(tx_id)))) {
        entry.waiting_queue.push({ tx_id, EXCLUSIVE });
        cv.wait(lk, [&]() {
            return (entry.exclusive_txn == -1 || entry.exclusive_txn == tx_id) &&
                (entry.shared_txns.empty() || (entry.shared_txns.size() == 1 && entry.shared_txns.count(tx_id)));
            });
    }
    entry.exclusive_txn = tx_id;
    entry.shared_txns.erase(tx_id); // Éý¼¶ËøÊ±ÒÆ³ý¹²ÏíËø
}

void LockManager::releaseLocks(int tx_id) {
    std::unique_lock<std::mutex> lk(table_mutex);
    for (auto& pair : lock_table) {
        LockEntry& entry = pair.second;

        entry.shared_txns.erase(tx_id);

        if (entry.exclusive_txn == tx_id) {
            entry.exclusive_txn = -1;
        }

        while (!entry.waiting_queue.empty()) {
            LockRequest req = entry.waiting_queue.front();
            bool can_acquire = false;
            if (req.type == SHARED) {
                if (entry.exclusive_txn == -1) can_acquire = true;
            }
            else { // EXCLUSIVE
                if (entry.exclusive_txn == -1 && entry.shared_txns.empty()) can_acquire = true;
            }

            if (can_acquire) {
                entry.waiting_queue.pop();
                cv.notify_all();
            }
            else {
                break;
            }
        }
    }
}

void LockManager::printLockTable() {
    std::unique_lock<std::mutex> lk(table_mutex);
    std::cout << "Lock Table Status:\n";
    for (auto& pair : lock_table) {
        std::cout << "Object: " << pair.first
            << " Shared: [";
        for (auto tx : pair.second.shared_txns) std::cout << tx << " ";
        std::cout << "] Exclusive: " << pair.second.exclusive_txn << "\n";
    }
}

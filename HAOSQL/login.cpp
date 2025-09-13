#include "login.h"
#include <iostream>


std::string current_username;

LoginManager::LoginManager(FileManager& fm, const std::string& db_dir)
    : fm(fm), current_user("") {
    // 打开用户表文件
    fileId = fm.openFile("users.dat");

    // 如果是新文件（0 页），先分配一页作为用户表第一页
    if (fm.getPageCount(fileId) == 0) {
        fm.allocatePage(fileId, META_PAGE);  // 0 号页
    }
}

// 检查用户名是否已存在
bool LoginManager::checkUserExists(const std::string& username) {
    uint32_t pageCount = fm.getPageCount(fileId);

    for (uint32_t pid = 0; pid < pageCount; pid++) {
        Page page;
        if (!fm.readPage(fileId, pid, page)) continue;

        for (int i = 0; i < page.header()->slot_count; i++) {
            std::string rec = page.readRecord(i);
            size_t pos = rec.find(':');
            if (pos != std::string::npos) {
                std::string user = rec.substr(0, pos);
                if (user == username) {
                    return true;
                }
            }
        }
    }
    return false;
}

// 注册新用户
bool LoginManager::registerUser(const std::string& username, const std::string& password) {
    if (checkUserExists(username)) {
        std::cerr << "User already exists: " << username << "\n";
        return false;
    }

    std::string record = username + ":" + password;
    uint32_t pageCount = fm.getPageCount(fileId);

    // 遍历已有页，尝试插入
    for (uint32_t pid = 0; pid < pageCount; pid++) {
        Page page;
        if (!fm.readPage(fileId, pid, page)) continue;

        try {
            page.insertRecord(record.c_str(), record.size());
            return fm.writePage(fileId, page);
        }
        catch (...) {
            // 当前页放不下，跳过
        }
    }

    // 所有页都满了，新建一页
    int newPid = fm.allocatePage(fileId, META_PAGE);
    Page newPage(META_PAGE, newPid);
    newPage.insertRecord(record.c_str(), record.size());
    return fm.writePage(fileId, newPage);
}

// 登录
bool LoginManager::loginUser(const std::string& username, const std::string& password) {
    uint32_t pageCount = fm.getPageCount(fileId);

    for (uint32_t pid = 0; pid < pageCount; pid++) {
        Page page;
        if (!fm.readPage(fileId, pid, page)) continue;

        for (int i = 0; i < page.header()->slot_count; i++) {
            std::string rec = page.readRecord(i);
            size_t pos = rec.find(':');
            if (pos != std::string::npos) {
                std::string user = rec.substr(0, pos);
                std::string pass = rec.substr(pos + 1);
                if (user == username && pass == password) {
                    current_user = username;
                    std::cout << "Login success: " << username << "\n";
                    current_username = username;
                    return true;
                }
            }
        }
    }

    std::cerr << "Login failed for user: " << username << "\n";
    return false;
}

// 登出
void LoginManager::logout() {
    current_user.clear();
    current_username = "";
    std::cout << "Logout success.\n";
}

// 当前用户
std::string LoginManager::currentUser() const {
    return current_user;
}

void LoginManager::dumpAllUsers() {
    uint32_t pageCount = fm.getPageCount(fileId);
    std::cout << "=== Users in users.dat (" << pageCount << " pages) ===\n";

    for (uint32_t pid = 0; pid < pageCount; pid++) {
        Page page;
        if (!fm.readPage(fileId, pid, page)) continue;

        std::cout << "[Page " << pid << "] slot_count=" << page.header()->slot_count << "\n";
        for (int i = 0; i < page.header()->slot_count; i++) {
            std::string rec = page.readRecord(i);
            if (rec.empty()) continue;

            size_t pos = rec.find(':');
            if (pos != std::string::npos) {
                std::string user = rec.substr(0, pos);
                std::string pass = rec.substr(pos + 1);
                std::cout << "  User=" << user << "  Pass=" << pass << "\n";
            }
        }
    }
    std::cout << "=== End Users ===\n";
}

/*int main() {
    FileManager fm("HAODB");
    LoginManager lm(fm, "HAODB");

    lm.registerUser("alice", "1234");
    lm.registerUser("bob", "abcd");

    lm.loginUser("alice", "1234");
    std::cout << "Current user: " << lm.currentUser() << "\n";

    lm.logout();
    lm.dumpAllUsers();   // ✅ 打印所有用户
    std::cout << "Current user: " << lm.currentUser() << "\n";
}*/
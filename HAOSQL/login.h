#pragma once
#include <string>
#include "file_manager.h"
#include "page.h"

extern std::string current_username;

class LoginManager {
public:
    LoginManager(FileManager& fm, const std::string& db_dir);

    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);
    void logout();
    void dumpAllUsers();  // 打印所有用户
    std::string currentUser() const;

private:
    FileManager& fm;
    int fileId;
    std::string current_user;

    bool checkUserExists(const std::string& username);
};

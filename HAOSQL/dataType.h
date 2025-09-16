#pragma once

#include <string>
#include <vector>
#include <chrono>

// 词法分析结果
struct Token {
    int type;       // 种别码
    std::string value;   // 单词
    int line;       // 行号
    int column;     // 列号
};

// 语义分析结果
struct Quadruple {
    std::string op;     // 操作符 (CREATE, SELECT, INSERT, DELETE, UPDATE, RESULT, etc.)
    std::string arg1;   // 参数1
    std::string arg2;   // 参数2
    std::string result; // 结果
};


// RID：行标识符
struct RID {
    uint32_t page_id;   // 数据页编号
    uint16_t slot_id;   // 数据行在页内的槽位

    // 定义相等比较（必要的）
    bool operator==(const RID& other) const {
        return page_id == other.page_id && slot_id == other.slot_id;
    }

    // 如果需要排序，可以加 <
    bool operator<(const RID& other) const {
        if (page_id != other.page_id) return page_id < other.page_id;
        return slot_id < other.slot_id;
    }
};

// 全局变量
extern std::string USER_NAME;  // 用户名
void SET_USER_Name(std::string& name);
extern std::string DBName;  // 数据库名称
void setDBName(const std::string& name);
extern std::string LOG_PATH;  // 日志路径
void SET_LOG_PATH(std::string path);
extern std::uint32_t LAST_LSN;  // WAL日志lsn序号
void SET_LSN(uint32_t lsn);
struct WALDataRecord
{
    uint16_t before_page_id;
    uint16_t before_slot_id;
    uint16_t before_length;
    uint16_t after_page_id;
    uint16_t after_slot_id;
    uint16_t after_length;
    std::string sql;
    std::vector<Quadruple> quas;
};
extern WALDataRecord WAL_DATA_RECORD;  // 日志记录
void SET_BEFORE_WAL_RECORD(uint16_t pid, uint16_t sid, uint16_t len);
void SET_AFTER_WAL_RECORD(uint16_t pid, uint16_t sid, uint16_t len);
void SET_SQL_QUAS(std::string sql, std::vector<Quadruple> quas);
class SQLTimer {
public:
    SQLTimer() : running(false) {}

    // 开始计时
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
        running = true;
    }

    // 停止计时
    void stop() {
        end_time = std::chrono::high_resolution_clock::now();
        running = false;
    }

    // 获取耗时（毫秒）
    long long elapsed_ms() const {
        auto end_point = running ? std::chrono::high_resolution_clock::now() : end_time;
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_point - start_time).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
    bool running;
};
extern SQLTimer GLOBAL_TIMER;  // 每次执行计时器


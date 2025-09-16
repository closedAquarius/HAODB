#pragma once

#include <string>
#include <vector>
#include <chrono>

// �ʷ��������
struct Token {
    int type;       // �ֱ���
    std::string value;   // ����
    int line;       // �к�
    int column;     // �к�
};

// ����������
struct Quadruple {
    std::string op;     // ������ (CREATE, SELECT, INSERT, DELETE, UPDATE, RESULT, etc.)
    std::string arg1;   // ����1
    std::string arg2;   // ����2
    std::string result; // ���
};


// RID���б�ʶ��
struct RID {
    uint32_t page_id;   // ����ҳ���
    uint16_t slot_id;   // ��������ҳ�ڵĲ�λ

    // ������ȱȽϣ���Ҫ�ģ�
    bool operator==(const RID& other) const {
        return page_id == other.page_id && slot_id == other.slot_id;
    }

    // �����Ҫ���򣬿��Լ� <
    bool operator<(const RID& other) const {
        if (page_id != other.page_id) return page_id < other.page_id;
        return slot_id < other.slot_id;
    }
};

// ȫ�ֱ���
extern std::string USER_NAME;  // �û���
void SET_USER_Name(std::string& name);
extern std::string DBName;  // ���ݿ�����
void setDBName(const std::string& name);
extern std::string LOG_PATH;  // ��־·��
void SET_LOG_PATH(std::string path);
extern std::uint32_t LAST_LSN;  // WAL��־lsn���
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
extern WALDataRecord WAL_DATA_RECORD;  // ��־��¼
void SET_BEFORE_WAL_RECORD(uint16_t pid, uint16_t sid, uint16_t len);
void SET_AFTER_WAL_RECORD(uint16_t pid, uint16_t sid, uint16_t len);
void SET_SQL_QUAS(std::string sql, std::vector<Quadruple> quas);
class SQLTimer {
public:
    SQLTimer() : running(false) {}

    // ��ʼ��ʱ
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
        running = true;
    }

    // ֹͣ��ʱ
    void stop() {
        end_time = std::chrono::high_resolution_clock::now();
        running = false;
    }

    // ��ȡ��ʱ�����룩
    long long elapsed_ms() const {
        auto end_point = running ? std::chrono::high_resolution_clock::now() : end_time;
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_point - start_time).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
    bool running;
};
extern SQLTimer GLOBAL_TIMER;  // ÿ��ִ�м�ʱ��


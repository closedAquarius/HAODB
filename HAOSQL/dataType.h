#pragma once

#include <string>

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

// ȫ�ֱ������洢���ݿ�����
extern std::string DBName;
void setDBName(const std::string& name);
extern std::uint32_t LAST_LSN; 
void SET_LSN(uint32_t lsn);



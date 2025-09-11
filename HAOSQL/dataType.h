#pragma once

#include <string>
using namespace std;

// �ʷ��������
struct Token {
    int type;       // �ֱ���
    string value;   // ����
    int line;       // �к�
    int column;     // �к�
};

// ����������
struct Quadruple {
    string op;     // ������ (CREATE, SELECT, INSERT, DELETE, UPDATE, RESULT, etc.)
    string arg1;   // ����1
    string arg2;   // ����2
    string result; // ���
};


// RID���б�ʶ��
struct RID {
    uint32_t page_id;   // ����ҳ���
    uint16_t slot_id;   // ��������ҳ�ڵĲ�λ
};
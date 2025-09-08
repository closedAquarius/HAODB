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

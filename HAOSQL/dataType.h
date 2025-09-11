#pragma once

#include <string>
using namespace std;

// 词法分析结果
struct Token {
    int type;       // 种别码
    string value;   // 单词
    int line;       // 行号
    int column;     // 列号
};

// 语义分析结果
struct Quadruple {
    string op;     // 操作符 (CREATE, SELECT, INSERT, DELETE, UPDATE, RESULT, etc.)
    string arg1;   // 参数1
    string arg2;   // 参数2
    string result; // 结果
};


// RID：行标识符
struct RID {
    uint32_t page_id;   // 数据页编号
    uint16_t slot_id;   // 数据行在页内的槽位
};
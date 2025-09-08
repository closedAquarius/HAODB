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

#pragma once
#ifndef LEXER_H
#define LEXER_H

#include <iostream>
#include <vector>
#include <unordered_set>
#include <cctype>
#include <iomanip>
#include <string>
#include "dataType.h"

using namespace std;

// 词法分析器类
class Lexer {
private:
    string input;                      // 输入的 SQL 源代码
    int pos;                           // 当前扫描位置
    int line;                          // 行号
    int column;                        // 列号
    vector<Token> tokens;              // 扫描得到的 token 序列

    unordered_set<string> keywords;    // 关键字集合
    unordered_set<char> operators;     // 运算符集合
    unordered_set<char> delimiters;    // 界符集合

    string readWord();                 // 读取标识符/关键字
    string readNumber();               // 读取常数
    string readString();

public:
    Lexer(string src);                 // 构造函数
    vector<Token> analyze();           // 执行词法分析并返回结果
};

class LexicalError : public std::runtime_error {
public:
    explicit LexicalError(const std::string& msg) : std::runtime_error(msg) {}
};
#endif // LEXER_H

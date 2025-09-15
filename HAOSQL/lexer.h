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

// �ʷ���������
class Lexer {
private:
    string input;                      // ����� SQL Դ����
    int pos;                           // ��ǰɨ��λ��
    int line;                          // �к�
    int column;                        // �к�
    vector<Token> tokens;              // ɨ��õ��� token ����

    unordered_set<string> keywords;    // �ؼ��ּ���
    unordered_set<char> operators;     // ���������
    unordered_set<char> delimiters;    // �������

    string readWord();                 // ��ȡ��ʶ��/�ؼ���
    string readNumber();               // ��ȡ����
    string readString();

public:
    Lexer(string src);                 // ���캯��
    vector<Token> analyze();           // ִ�дʷ����������ؽ��
};

class LexicalError : public std::runtime_error {
public:
    explicit LexicalError(const std::string& msg) : std::runtime_error(msg) {}
};
#endif // LEXER_H

#pragma once
#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>
#include <iomanip>
#include <map>

#include "dataType.h"

using namespace std;

// ������ս��
typedef struct NOterminal
{
    string name;
    int first_number;   // FIRST�����ж�����ʼΪ0
    int follow_number;  // FOLLOW�����ж�����ʼΪ0
    vector<string> FIRST;    // FIRST ����
    vector<string> FOLLOW;   // FOLLOW����
    struct NOterminal* next;
} noterminal;

// �����ս��
typedef struct Terminal
{
    string name;            // ��ǰ���ַ���
    struct Terminal* next;
} terminal;

// �������ʽ
typedef struct PRODUCTION
{
    string source;              // �����Ŀ�ʼ
    vector<string> result;      // �����Ľ��
    struct PRODUCTION* next;    // ָ����һ��
} production;

class SQLParser {
private:
    int amount;
    vector<string> TEST_STACK;
    vector<Token> input_tokens;
    vector<string> terminal_all;
    using PredictionTable = std::map<std::pair<std::string, std::string>, int>;

    terminal TERMINAL_HEAD, * current_terminal;
    noterminal NOTERMINAL_HEAD, * current_noterminal;
    production PRODUCTION_HEAD, * current_production;

public:
    // ���캯��
    SQLParser();

    // ��Ҫ�ӿں���
    int start_parser(vector<Token>& tokens);

    // �ķ���ȡ����
    size_t read_grammar(const char* filename);

    // ���Ժ͵��Ժ���
    size_t test(void);
    void Test_read(void);

    // ջ��������
    size_t STACK_FULL();
    void STACK_POP(void);
    size_t STACK_EMPTY();
    void init_stack(void);
    void STACK_PUSH(string source);

    // �﷨�������ĺ���
    void prediction(void);
    void test_follow(void);
    void prediction_table(void);
    void analyze_input(vector<Token>& tokens, PredictionTable& table, vector<production*>& production_list);
    void print_analysis_step(const vector<string>& input_symbols, int input_pos);
    void compute_first_of_string(const vector<string>& symbols, vector<string>& first_set);

    // �ķ�������
    void eliminate_left_recursion(void);
    void remove_production(production* target);
    bool has_left_recursion();
    size_t find_first(noterminal* this_noterminal, production* this_production);
    size_t find_follow(noterminal* this_noterminal, production* this_production);

    // ���ݽṹ��������
    void insert_to_terminal(string get);
    void insert_to_noterminal(string get);
    void insert_to_production(string source, vector<string>& result);

    // ��������
    void load_terminals_from_tokens(vector<Token>& tokens);
    noterminal* get_noterminal(string X);
    bool is_terminal_string(string X);
    size_t add_to_set(vector<string>& set, string item);
    string token_to_terminal(const Token& token);

    // ��������
    void emergency(int model);
};

#endif // SQL_PARSER_H#pragma once

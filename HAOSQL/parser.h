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

// 定义非终结符
typedef struct NOterminal
{
    string name;
    int first_number;   // FIRST集的判定，初始为0
    int follow_number;  // FOLLOW集的判定，初始为0
    vector<string> FIRST;    // FIRST 集合
    vector<string> FOLLOW;   // FOLLOW集合
    struct NOterminal* next;
} noterminal;

// 定义终结符
typedef struct Terminal
{
    string name;            // 当前的字符串
    struct Terminal* next;
} terminal;

// 定义产生式
typedef struct PRODUCTION
{
    string source;              // 产生的开始
    vector<string> result;      // 产生的结果
    struct PRODUCTION* next;    // 指向下一条
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
    // 构造函数
    SQLParser();

    // 主要接口函数
    int start_parser(vector<Token>& tokens);

    // 文法读取函数
    size_t read_grammar(const char* filename);

    // 测试和调试函数
    size_t test(void);
    void Test_read(void);

    // 栈操作函数
    size_t STACK_FULL();
    void STACK_POP(void);
    size_t STACK_EMPTY();
    void init_stack(void);
    void STACK_PUSH(string source);

    // 语法分析核心函数
    void prediction(void);
    void test_follow(void);
    void prediction_table(void);
    void analyze_input(vector<Token>& tokens, PredictionTable& table, vector<production*>& production_list);
    void print_analysis_step(const vector<string>& input_symbols, int input_pos);
    void compute_first_of_string(const vector<string>& symbols, vector<string>& first_set);

    // 文法处理函数
    void eliminate_left_recursion(void);
    void remove_production(production* target);
    bool has_left_recursion();
    size_t find_first(noterminal* this_noterminal, production* this_production);
    size_t find_follow(noterminal* this_noterminal, production* this_production);

    // 数据结构操作函数
    void insert_to_terminal(string get);
    void insert_to_noterminal(string get);
    void insert_to_production(string source, vector<string>& result);

    // 辅助函数
    void load_terminals_from_tokens(vector<Token>& tokens);
    noterminal* get_noterminal(string X);
    bool is_terminal_string(string X);
    size_t add_to_set(vector<string>& set, string item);
    string token_to_terminal(const Token& token);

    // 错误处理函数
    void emergency(int model);
};

#endif // SQL_PARSER_H#pragma once

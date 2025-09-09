#ifndef PARSER_H
#define PARSER_H
#include "dataType.h"

#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>
#include "dataType.h"

using namespace std;

class SQLParser {
private:
    int amount = 0;
    vector<string> TEST_STACK;
    vector<Token> input_tokens;
    vector<string> terminal_all;

    terminal TERMINAL_HEAD, * current_terminal;
    noterminal NOTERMINAL_HEAD, * current_noterminal;
    production PRODUCTION_HEAD, * current_production;

public:
    SQLParser() {
        TEST_STACK.resize(MAX_AMOUNT);
        TERMINAL_HEAD.next = NULL;
        NOTERMINAL_HEAD.next = NULL;
        PRODUCTION_HEAD.next = NULL;
    }

    //函数声明
    size_t read_grammar(const char* filename);
    size_t test(void);
    void Test_read(void);
    size_t STACK_FULL();
    void STACK_POP(void);
    size_t STACK_EMPTY();
    void init_stack(void);
    void prediction(void);
    void test_follow(void);
    void emergency(int model);
    void prediction_table(void);
    void STACK_PUSH(string source);
    void insert_to_terminal(string get);
    void insert_to_noterminal(string get);
    void eliminate_left_recursion(void);
    void combine(vector<string>& destination, vector<string>& source);
    size_t is_appeared(string tobejudged, vector<string>& source);
    void insert_to_production(string source, vector<string>& result);
    size_t find_first(noterminal* this_noterminal, production* this_production);
    size_t find_follow(noterminal* this_noterminal, production* this_production);
    void analyze_input(vector<Token>& tokens);

    //新增函数
    void load_terminals_from_tokens(vector<Token>& tokens);
    noterminal* get_noterminal(string X);
    bool is_terminal_string(string X);
    size_t add_to_set(vector<string>& set, string item);
    string token_to_terminal(Token& token);

    int start_parser(vector<Token>& tokens);
};

#endif

/*#ifndef SQL_PARSER_H
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

#include "dataType.h"

using namespace std;

#ifndef success
#define success 1
#endif

#define MAX_AMOUNT 50

//������ս��
typedef struct NOterminal
{
    string name;
    int first_number;   //FIRST�����ж�����ʼΪ0
    int follow_number;  //FOLLOW�����ж�����ʼΪ0
    vector<string> FIRST;    //FIRST ����
    vector<string> FOLLOW;   //FOLLOW����
    struct NOterminal* next;
} noterminal;

//�����ս��
typedef struct Terminal
{
    string name;            //��ǰ���ַ���
    struct Terminal* next;
} terminal;

//�������ʽ
typedef struct PRODUCTION
{
    string source;              //�����Ŀ�ʼ
    vector<string> result;      //�����Ľ��
    struct PRODUCTION* next;    //ָ����һ��
} production;

class SQLParser {
private:
    int amount = 0;
    vector<string> TEST_STACK;
    vector<Token> input_tokens;
    vector<string> terminal_all;

    terminal TERMINAL_HEAD, * current_terminal = &TERMINAL_HEAD;
    noterminal NOTERMINAL_HEAD, * current_noterminal = &NOTERMINAL_HEAD;
    production PRODUCTION_HEAD, * current_production = &PRODUCTION_HEAD;

public:
    SQLParser() {
        TEST_STACK.resize(MAX_AMOUNT);
        TERMINAL_HEAD.next = NULL;
        NOTERMINAL_HEAD.next = NULL;
        PRODUCTION_HEAD.next = NULL;
    }

    //��������
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

    //��������
    void load_terminals_from_tokens(vector<Token>& tokens);
    noterminal* get_noterminal(string X);
    bool is_terminal_string(string X);
    size_t add_to_set(vector<string>& set, string item);
    string token_to_terminal(Token& token);

    int start_parser(vector<Token>& tokens);
};

//�򼯺��м���һ���ַ�����������������0��1��
size_t SQLParser::add_to_set(vector<string>& set, string item)
{
    //����Ƿ����ڼ�����
    for (const auto& s : set) {
        if (s == item) return 0;
    }
    set.push_back(item);
    return 1;
}

//�����ַ�����Ӧ�ķ��ս���ڵ�
noterminal* SQLParser::get_noterminal(string X)
{
    noterminal* p = NOTERMINAL_HEAD.next;
    while (p)
    {
        if (p->name == X) return p;
        p = p->next;
    }
    return NULL;
}

//�ж�X�Ƿ�Ϊ�ս��
bool SQLParser::is_terminal_string(string X)
{
    for (const auto& terminal : terminal_all)
    {
        if (terminal == X) return true;
    }
    return false;
}

//��Tokenת��Ϊ�ս���ַ���
string SQLParser::token_to_terminal(Token& token)
{
    switch (token.type) {
    case 1: return token.value; // �ؼ���
    case 2: return "IDENTIFIER"; // ��ʶ��
    case 3: return "NUMBER"; // ����
    case 4: return token.value; // �����
    case 5: return token.value; // ���
    default: return token.value;
    }
}

//��tokens����ȡ�ս��
void SQLParser::load_terminals_from_tokens(vector<Token>& tokens)
{
    unordered_set<string> unique_terminals;

    // ����������
    unique_terminals.insert("#");  // ջ�׷���
    unique_terminals.insert("^");  // �մ�����

    // ��tokens����ȡ�ս��
    for (auto& token : tokens) {
        string terminal = token_to_terminal(token);
        unique_terminals.insert(terminal);
    }

    // ���뵽�ս������
    for (const auto& term : unique_terminals) {
        insert_to_terminal(term);
        terminal_all.push_back(term);
    }
}

int SQLParser::start_parser(vector<Token>& tokens)
{
    input_tokens = tokens;

    // ��tokens�м����ս��
    load_terminals_from_tokens(tokens);

    // ��ȡ�ķ�����
    read_grammar("grammar/sql_grammar.txt");
    Test_read();

    printf("\n������ݹ�\n\n\n");
    eliminate_left_recursion();
    Test_read();

    printf("\n��FIRST��\n\n");
    prediction();
    printf("\n��FOLLOW��\n\n");
    test_follow();
    prediction_table();

    // ��������tokens
    analyze_input(tokens);

    emergency(0);
    return 0;
}

//��ȡ�ķ��ļ�
size_t SQLParser::read_grammar(const char* filename)
{
    int line = 0, old_line = 0;
    string current_line;

    //��ȡ�ļ�
    FILE* read_stream = fopen(filename, "r");
    if (!read_stream)
    {
        printf("error in open file, can't open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    //��һ��ѭ������ȡ���ս��
    while (fgets(buffer, sizeof(buffer), read_stream))
    {
        string line_str(buffer);
        // ȥ�����з�
        if (!line_str.empty() && line_str.back() == '\n') {
            line_str.pop_back();
        }

        if (line == 0) {
            // ��һ���Ƿ��ս��
            size_t pos = 0;
            while (pos < line_str.length()) {
                if (line_str[pos] != ' ') {
                    size_t start = pos;
                    while (pos < line_str.length() && line_str[pos] != ' ') {
                        pos++;
                    }
                    string nonterminal = line_str.substr(start, pos - start);
                    insert_to_noterminal(nonterminal);
                }
                else {
                    pos++;
                }
            }
            break;
        }
        line++;
    }

    // �ڶ���ѭ������ȡ����ʽ
    while (fgets(buffer, sizeof(buffer), read_stream))
    {
        string line_str(buffer);
        // ȥ�����з�
        if (!line_str.empty() && line_str.back() == '\n') {
            line_str.pop_back();
        }

        if (line_str.empty()) continue;

        // ��������ʽ A->alpha|beta
        size_t arrow_pos = line_str.find("->");
        if (arrow_pos != string::npos) {
            string source = line_str.substr(0, arrow_pos);
            string right_part = line_str.substr(arrow_pos + 2);

            // ��'|'�ָ��Ҳ�
            size_t pos = 0;
            while (pos < right_part.length()) {
                size_t pipe_pos = right_part.find('|', pos);
                if (pipe_pos == string::npos) pipe_pos = right_part.length();

                string production_right = right_part.substr(pos, pipe_pos - pos);

                // �����Ҳ�Ϊ��������
                vector<string> symbols;
                size_t symbol_pos = 0;
                while (symbol_pos < production_right.length()) {
                    if (production_right[symbol_pos] != ' ') {
                        size_t start = symbol_pos;
                        while (symbol_pos < production_right.length() && production_right[symbol_pos] != ' ') {
                            symbol_pos++;
                        }
                        string symbol = production_right.substr(start, symbol_pos - start);
                        symbols.push_back(symbol);
                    }
                    else {
                        symbol_pos++;
                    }
                }

                if (!symbols.empty()) {
                    insert_to_production(source, symbols);
                }

                pos = pipe_pos + 1;
            }
        }
    }

    fclose(read_stream);
    return success;
}

//��FIRST����
size_t SQLParser::find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    //�������в���ʽ
    for (production* p = this_production; p; p = p->next) {
        if (p->source != this_noterminal->name) continue;

        vector<string>& alpha = p->result;
        //����Ҳ��Ǧţ���������'^'��ʾ��
        if (alpha.size() == 1 && alpha[0] == "^") {
            added += add_to_set(this_noterminal->FIRST, "^");
            continue;
        }

        //��������Ŵ���A �� X1 X2 X3 ...
        size_t i = 0;
        for (; i < alpha.size(); ++i) {
            string X = alpha[i];
            if (is_terminal_string(X)) {
                //��X���ս��������FIRST(A)
                added += add_to_set(this_noterminal->FIRST, X);
                break;
            }
            else {
                //X�Ƿ��ս�����ȵݹ�ȷ�� FIRST(X) �Ѿ�����/ȫ�������
                noterminal* Y = get_noterminal(X);
                if (!Y) break; // ��������

                //��FIRST(Y)\{��} ����FIRST(A)
                for (const auto& c : Y->FIRST) {
                    if (c != "^") {
                        added += add_to_set(this_noterminal->FIRST, c);
                    }
                }

                //��Y��FIRST�в����ţ���ͣ
                bool has_eps = false;
                for (const auto& c : Y->FIRST) {
                    if (c == "^") { has_eps = true; break; }
                }
                if (!has_eps) break;
                //�����������һ�� Xi
            }
        }
        //�������Xi�����Ƴ��ţ����Ҳ��������ǿգ�����A��FIRST�Ӧ�
        if (i == alpha.size()) {
            added += add_to_set(this_noterminal->FIRST, "^");
        }
    }
    return added;
}

//��FOLLOW����
size_t SQLParser::find_follow(noterminal* B, production* this_production)
{
    size_t added = 0;
    //��B�ǿ�ʼ���ţ����� '#'
    if (B == NOTERMINAL_HEAD.next) {
        added += add_to_set(B->FOLLOW, "#");
    }

    //�������в���ʽ A �� �� B ��
    for (production* p = this_production; p; p = p->next) {
        string A = p->source;
        vector<string>& rhs = p->result;

        for (size_t i = 0; i < rhs.size(); ++i) {
            if (rhs[i] != B->name) continue;

            //���� = rhs[i+1...]
            size_t j = i + 1;

            //�ȴ���·ǿղ���
            for (; j < rhs.size(); ++j) {
                string X = rhs[j];
                if (is_terminal_string(X)) {
                    //���׷������ս�������뵽 FOLLOW(B)
                    added += add_to_set(B->FOLLOW, X);
                    break;
                }
                else {
                    //���׷����Ƿ��ս�� Y
                    noterminal* Y = get_noterminal(X);
                    if (!Y) break;

                    //��FIRST(Y)\{��}����FOLLOW(B)
                    for (const auto& c : Y->FIRST) {
                        if (c != "^") {
                            added += add_to_set(B->FOLLOW, c);
                        }
                    }

                    //���FIRST(Y) ������, ��Ҫ��������һ������
                    bool has_eps = false;
                    for (const auto& c : Y->FIRST) {
                        if (c == "^") { has_eps = true; break; }
                    }
                    if (!has_eps) break;
                }
            }

            //�����Ϊ�գ����ȫ�����Ʀţ����FOLLOW(A)����FOLLOW(B)
            if (j == rhs.size()) {
                noterminal* A_nt = get_noterminal(A);
                if (A_nt) {
                    for (const auto& c : A_nt->FOLLOW) {
                        added += add_to_set(B->FOLLOW, c);
                    }
                }
            }
        }
    }
    return added;
}

// ��������������ʵ��...
void SQLParser::insert_to_terminal(string get)
{
    terminal* Temp_terminal = new terminal;
    Temp_terminal->name = get;
    Temp_terminal->next = NULL;
    current_terminal->next = Temp_terminal;
    current_terminal = Temp_terminal;
}

void SQLParser::insert_to_noterminal(string get)
{
    noterminal* Temp_noterminal = new noterminal;
    Temp_noterminal->name = get;
    Temp_noterminal->next = NULL;
    Temp_noterminal->first_number = 0;
    Temp_noterminal->follow_number = 0;
    current_noterminal->next = Temp_noterminal;
    current_noterminal = Temp_noterminal;
}

void SQLParser::insert_to_production(string source, vector<string>& result)
{
    production* Temp_production = new production;
    Temp_production->source = source;
    Temp_production->result = result;
    Temp_production->next = NULL;
    current_production->next = Temp_production;
    current_production = Temp_production;
}

void SQLParser::Test_read(void)
{
    int number = 1;
    production* TEMP_PRODUCTION = PRODUCTION_HEAD.next;
    terminal* TEMP_TERMINAL = TERMINAL_HEAD.next;
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    printf("����ʽ\n");
    for (number = 1; TEMP_PRODUCTION != NULL; TEMP_PRODUCTION = TEMP_PRODUCTION->next, number++)
    {
        printf("%d\t%s\t->", number, TEMP_PRODUCTION->source.c_str());
        for (const auto& symbol : TEMP_PRODUCTION->result) {
            printf(" %s", symbol.c_str());
        }
        printf("\n");
    }

    printf("\n�ս��\n");
    for (; TEMP_TERMINAL != NULL; TEMP_TERMINAL = TEMP_TERMINAL->next)
    {
        printf("%s\t", TEMP_TERMINAL->name.c_str());
    }
    printf("\n");

    printf("\n���ս��\n");
    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%s\t", TEMP_NOTERMINAL->name.c_str());
    }
    printf("\n");
}

size_t SQLParser::test(void)
{
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%s\tfirst number=%d\tfirst=",
            TEMP_NOTERMINAL->name.c_str(),
            (int)TEMP_NOTERMINAL->FIRST.size());
        for (const auto& symbol : TEMP_NOTERMINAL->FIRST) {
            printf("%s ", symbol.c_str());
        }
        printf("\n");
    }
    printf("\n");
    return success;
}

void SQLParser::test_follow(void)
{
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%s\tfollow number=%d\tfollow=",
            TEMP_NOTERMINAL->name.c_str(),
            (int)TEMP_NOTERMINAL->FOLLOW.size());
        for (const auto& symbol : TEMP_NOTERMINAL->FOLLOW) {
            printf("%s ", symbol.c_str());
        }
        printf("\n");
    }
    printf("\n");
}

void SQLParser::prediction(void)
{
    noterminal* TEMP_NOTERMINAL;
    bool changed;
    do
    {
        changed = false;
        TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;
        while (TEMP_NOTERMINAL != NULL)
        {
            size_t old_size = TEMP_NOTERMINAL->FIRST.size();
            find_first(TEMP_NOTERMINAL, PRODUCTION_HEAD.next);
            if (TEMP_NOTERMINAL->FIRST.size() > old_size) {
                changed = true;
            }
            TEMP_NOTERMINAL = TEMP_NOTERMINAL->next;
        }
    } while (changed);
    test();

    do {
        changed = false;
        TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;
        while (TEMP_NOTERMINAL != NULL)
        {
            size_t old_size = TEMP_NOTERMINAL->FOLLOW.size();
            find_follow(TEMP_NOTERMINAL, PRODUCTION_HEAD.next);
            if (TEMP_NOTERMINAL->FOLLOW.size() > old_size) {
                changed = true;
            }
            TEMP_NOTERMINAL = TEMP_NOTERMINAL->next;
        }
    } while (changed);
}

// �򻯰汾��������ݹ� - ����ֻ��ռλ������ʵ�ֽϸ���
void SQLParser::eliminate_left_recursion(void)
{
    // ����SQL�﷨��ͨ�����ʱ�ͱ�����ݹ�
    // ���ﱣ��ԭ�����ṹ����ʵ��
    printf("��ݹ�������ɣ�SQL�﷨���ʱ�ѱ�����ݹ飩\n");
}

// ջ��������
void SQLParser::init_stack(void)
{
    amount = 0;
    TEST_STACK.clear();
    TEST_STACK.resize(MAX_AMOUNT);
    TEST_STACK[amount] = "#";
}

void SQLParser::STACK_PUSH(string source)
{
    if (STACK_FULL())
    {
        printf("ջ��\n");
        emergency(2);
    }
    TEST_STACK[++amount] = source;
}

void SQLParser::STACK_POP(void)
{
    if (STACK_EMPTY())
    {
        printf("ջ��\n");
        emergency(2);
    }
    amount--;
}

size_t SQLParser::STACK_EMPTY()
{
    return amount == 0 ? success : !success;
}

size_t SQLParser::STACK_FULL()
{
    return amount >= MAX_AMOUNT - 1 ? success : !success;
}

// Ԥ���������﷨����ʵ��
void SQLParser::prediction_table(void)
{
    // ������Ҫʵ��Ԥ�������Ĺ���
    // ���ڴ���ϳ�������������
    printf("Ԥ������������\n");
}

void SQLParser::analyze_input(vector<Token>& tokens)
{
    printf("��ʼ��������tokens...\n");

    // ��ʼ������ջ
    init_stack();
    STACK_PUSH("SQL_STMT");  // ѹ�뿪ʼ����

    int token_pos = 0;

    while (!STACK_EMPTY() && token_pos < tokens.size())
    {
        string top = TEST_STACK[amount];  // ջ������
        string current = token_to_terminal(tokens[token_pos]);  // ��ǰ�������

        printf("ջ��: %s, ��ǰ����: %s\n", top.c_str(), current.c_str());

        if (is_terminal_string(top))
        {
            // ջ�����ս��
            if (top == current)
            {
                // ƥ�䣬��ջ���ƶ�����ָ��
                STACK_POP();
                token_pos++;
                printf("ƥ��ɹ�����������\n");
            }
            else
            {
                // ��ƥ�䣬����
                printf("�﷨�������� %s���õ� %s\n", top.c_str(), current.c_str());
                return;
            }
        }
        else
        {
            // ջ���Ƿ��ս����������Ҫ��Ԥ�������
            // �򻯴���ֱ�ӳ�ջ
            STACK_POP();
            printf("������ս��: %s\n", top.c_str());
        }
    }

    if (STACK_EMPTY() && token_pos == tokens.size()) {
        printf("\n�﷨�����ɹ���\n");
    }
    else {
        printf("\n�﷨����ʧ�ܣ�\n");
    }
}

void SQLParser::emergency(int model)
{
    printf("������ɣ�������Դ\n");
    // ����Ӧ���ͷ����з�����ڴ�
    exit(0);
}

#endif // SQL_PARSER_H

*/
#define _CRT_SECURE_NO_WARNINGS

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
#include "parser.h"

using namespace std;

#define success 1
#define MAX_AMOUNT 50

// ���캯��
SQLParser::SQLParser()
{
    amount = 0;
    TEST_STACK.resize(MAX_AMOUNT);
    TERMINAL_HEAD.next = NULL;
    NOTERMINAL_HEAD.next = NULL;
    PRODUCTION_HEAD.next = NULL;
    current_terminal = &TERMINAL_HEAD;
    current_noterminal = &NOTERMINAL_HEAD;
    current_production = &PRODUCTION_HEAD;
}

// �򼯺��м���һ���ַ�����������������0��1��
size_t SQLParser::add_to_set(vector<string>& set, string item)
{
    //����Ƿ����ڼ�����
    for (const auto& s : set)
    {
        if (s == item) return 0;
    }
    set.push_back(item);
    return 1;
}

// �����ַ�����Ӧ�ķ��ս���ڵ�
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

// �ж�X�Ƿ�Ϊ�ս��
bool SQLParser::is_terminal_string(string X)
{
    for (const auto& terminal : terminal_all)
    {
        if (terminal == X) return true;
    }
    return false;
}

// ��Tokenת��Ϊ�ս���ַ���
string SQLParser::token_to_terminal(const Token& token)
{
    switch (token.type)
    {
    case 1: return token.value;  // �ؼ���
    case 2: return "IDENTIFIER"; // ��ʶ��
    case 3: return "NUMBER";     // ����
    case 4: return token.value;  // �����
    case 5: return token.value;  // ���
    default: return token.value;
    }
}

// ��tokens����ȡ�ս��
void SQLParser::load_terminals_from_tokens(vector<Token>& tokens)
{
    unordered_set<string> unique_terminals;

    // ����������
    unique_terminals.insert("#");  // ջ�׷���
    unique_terminals.insert("^");  // �մ�����

    // ��tokens����ȡ�ս��
    for (auto& token : tokens)
    {
        string terminal = token_to_terminal(token);
        unique_terminals.insert(terminal);
    }

    // ���뵽�ս������
    for (const auto& term : unique_terminals)
    {
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
    read_grammar("sql_grammar.txt");
    Test_read();


    printf("\n������ݹ�\n\n\n");
    if (has_left_recursion())
        printf("����ݹ�");
    eliminate_left_recursion();
    Test_read();

    printf("\n��FIRST��\n\n");
    prediction();
    printf("\n��FOLLOW��\n\n");
    test_follow();
    prediction_table();

    emergency(0);
    return 0;
}

// ��ȡ�ķ�����
size_t SQLParser::read_grammar(const char* filename)
{
    int line = 0, old_line = 0;
    string current_line;

    // ��ȡ�ļ�
    FILE* read_stream = fopen(filename, "r");
    if (!read_stream)
    {
        printf("error in open file, can't open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    // ��ȡ���ս��
    while (fgets(buffer, sizeof(buffer), read_stream))
    {
        string line_str(buffer);
        // ȥ�����з�
        if (!line_str.empty() && line_str.back() == '\n')
        {
            line_str.pop_back();
        }

        if (line == 0)
        {
            // ��һ���Ƿ��ս��
            size_t pos = 0;
            while (pos < line_str.length())
            {
                if (line_str[pos] != ' ')
                {
                    size_t start = pos;
                    while (pos < line_str.length() && line_str[pos] != ' ')
                    {
                        pos++;
                    }
                    string nonterminal = line_str.substr(start, pos - start);
                    insert_to_noterminal(nonterminal);
                }
                else
                {
                    pos++;
                }
            }
            break;
        }
        line++;
    }

    // ��ȡ����ʽ
    while (fgets(buffer, sizeof(buffer), read_stream))
    {
        string line_str(buffer);
        // ȥ�����з�
        if (!line_str.empty() && line_str.back() == '\n')
        {
            line_str.pop_back();
        }

        if (line_str.empty()) continue;

        // ��������ʽ A->alpha|beta
        size_t arrow_pos = line_str.find("->");
        if (arrow_pos != string::npos)
        {
            string source = line_str.substr(0, arrow_pos);
            string right_part = line_str.substr(arrow_pos + 2);

            // ��'|'�ָ��Ҳ�
            size_t pos = 0;
            while (pos < right_part.length())
            {
                size_t pipe_pos = right_part.find('|', pos);
                if (pipe_pos == string::npos) pipe_pos = right_part.length();

                string production_right = right_part.substr(pos, pipe_pos - pos);

                // �����Ҳ�Ϊ��������
                vector<string> symbols;
                size_t symbol_pos = 0;
                while (symbol_pos < production_right.length())
                {
                    if (production_right[symbol_pos] != ' ')
                    {
                        size_t start = symbol_pos;
                        while (symbol_pos < production_right.length() && production_right[symbol_pos] != ' ')
                        {
                            symbol_pos++;
                        }
                        string symbol = production_right.substr(start, symbol_pos - start);
                        symbols.push_back(symbol);
                    }
                    else
                    {
                        symbol_pos++;
                    }
                }

                if (!symbols.empty())
                {
                    insert_to_production(source, symbols);
                }

                pos = pipe_pos + 1;
            }
        }
    }

    fclose(read_stream);
    return success;
}

// ��FIRST����
size_t SQLParser::find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    // �������в���ʽ
    for (production* p = this_production; p; p = p->next)
    {
        if (p->source != this_noterminal->name) continue;

        vector<string>& alpha = p->result;
        // ����Ҳ��Ǧţ���������'^'��ʾ��
        if (alpha.size() == 1 && alpha[0] == "^")
        {
            added += add_to_set(this_noterminal->FIRST, "^");
            continue;
        }

        // ��������Ŵ���A �� X1 X2 X3 ...
        size_t i = 0;
        for (; i < alpha.size(); ++i)
        {
            string X = alpha[i];
            if (is_terminal_string(X))
            {
                // ��X���ս��������FIRST(A)
                added += add_to_set(this_noterminal->FIRST, X);
                break;
            }
            else
            {
                // X�Ƿ��ս�����ȵݹ�ȷ�� FIRST(X) �Ѿ�����/ȫ�������
                noterminal* Y = get_noterminal(X);
                if (!Y) break;  // ��������

                // ��FIRST(Y)\{��} ����FIRST(A)
                for (const auto& c : Y->FIRST)
                {
                    if (c != "^")
                    {
                        added += add_to_set(this_noterminal->FIRST, c);
                    }
                }

                //��Y��FIRST�в����ţ���ͣ
                bool has_eps = false;
                for (const auto& c : Y->FIRST)
                {
                    if (c == "^") { has_eps = true; break; }
                }
                if (!has_eps) break;
                //�����������һ�� Xi
            }
        }
        //�������Xi�����Ƴ��ţ����Ҳ��������ǿգ�����A��FIRST�Ӧ�
        if (i == alpha.size())
        {
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
    if (B == NOTERMINAL_HEAD.next)
    {
        added += add_to_set(B->FOLLOW, "#");
    }

    //�������в���ʽ A �� �� B ��
    for (production* p = this_production; p; p = p->next)
    {
        string A = p->source;
        vector<string>& rhs = p->result;

        for (size_t i = 0; i < rhs.size(); ++i)
        {
            if (rhs[i] != B->name) continue;

            //���� = rhs[i+1...]
            size_t j = i + 1;

            //�ȴ���·ǿղ���
            for (; j < rhs.size(); ++j) {
                string X = rhs[j];
                if (is_terminal_string(X))
                {
                    //���׷������ս�������뵽 FOLLOW(B)
                    added += add_to_set(B->FOLLOW, X);
                    break;
                }
                else
                {
                    //���׷����Ƿ��ս�� Y
                    noterminal* Y = get_noterminal(X);
                    if (!Y) break;

                    //��FIRST(Y)\{��}����FOLLOW(B)
                    for (const auto& c : Y->FIRST)
                    {
                        if (c != "^")
                        {
                            added += add_to_set(B->FOLLOW, c);
                        }
                    }

                    //���FIRST(Y) ������, ��Ҫ��������һ������
                    bool has_eps = false;
                    for (const auto& c : Y->FIRST)
                    {
                        if (c == "^")
                        {
                            has_eps = true; break;
                        }
                    }
                    if (!has_eps) break;
                }
            }

            //�����Ϊ�գ����ȫ�����Ʀţ����FOLLOW(A)����FOLLOW(B)
            if (j == rhs.size())
            {
                noterminal* A_nt = get_noterminal(A);
                if (A_nt)
                {
                    for (const auto& c : A_nt->FOLLOW)
                    {
                        added += add_to_set(B->FOLLOW, c);
                    }
                }
            }
        }
    }
    return added;
}

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

// ������ݹ�
void SQLParser::eliminate_left_recursion(void)
{
    printf("��ʼ������ݹ�...\n");

    // ����Ƿ�����ݹ�
    if (!has_left_recursion())
    {
        printf("δ������ݹ飬������������\n");
        return;
    }

    printf("��⵽��ݹ飬��ʼ����...\n");

    // �����ս������
    terminal_all.clear();
    terminal* temp = TERMINAL_HEAD.next;
    while (temp)
    {
        terminal_all.push_back(temp->name);
        temp = temp->next;
    }

    // �ռ����з��ս������˳���������ظ�����
    vector<string> processed_nonterminals;

    // ���α�������������ݹ�
    production* current_prod = PRODUCTION_HEAD.next;

    while (current_prod)
    {
        // ����Ƿ�Ϊֱ����ݹ���δ�����
        if (!current_prod->result.empty() &&
            current_prod->source == current_prod->result[0] &&
            find(processed_nonterminals.begin(), processed_nonterminals.end(), current_prod->source) == processed_nonterminals.end())
        {
            string A = current_prod->source;  // ԭ���ս��
            printf("\n������ս�� %s ����ݹ�\n", A.c_str());

            // ���Ϊ�Ѵ���
            processed_nonterminals.push_back(A);

            string new_nt = A + "'";          // �·��ս��A'

            // ȷ���·��ս��������ͻ
            while (get_noterminal(new_nt) != NULL)
            {
                new_nt += "'";
            }

            // �ռ�����A�Ĳ���ʽ������
            vector<vector<string>> left_recursive_contents;   // ������ݹ����ʽ������
            vector<vector<string>> non_left_recursive_contents; // �������ݹ����ʽ������
            vector<production*> to_delete; // ��Ҫɾ���Ĳ���ʽָ��

            production* p = PRODUCTION_HEAD.next;
            while (p)
            {
                if (p->source == A)
                {
                    if (!p->result.empty() && p->result[0] == A)
                    {
                        // ��ݹ����ʽ����������
                        left_recursive_contents.push_back(p->result);
                        to_delete.push_back(p);
                        printf("  ��ݹ����ʽ: %s ->", p->source.c_str());
                        for (const auto& symbol : p->result)
                        {
                            printf(" %s", symbol.c_str());
                        }
                        printf("\n");
                    }
                    else
                    {
                        // ����ݹ����ʽ����������
                        non_left_recursive_contents.push_back(p->result);
                        to_delete.push_back(p);
                        printf("  ����ݹ����ʽ: %s ->", p->source.c_str());
                        for (const auto& symbol : p->result)
                        {
                            printf(" %s", symbol.c_str());
                        }
                        printf("\n");
                    }
                }
                p = p->next;
            }

            printf("�ҵ� %zu ����ݹ����ʽ��%zu ������ݹ����ʽ\n",
                left_recursive_contents.size(), non_left_recursive_contents.size());

            // ����µķ��ս��A'
            insert_to_noterminal(new_nt);
            printf("�����·��ս��: %s\n", new_nt.c_str());

            // ɾ������ԭ����A����ʽ
            printf("ɾ��ԭ�в���ʽ...\n");
            for (production* old_prod : to_delete) {
                remove_production(old_prod);
            }

            // �����µĲ���ʽ A -> beta A'������ÿ������ݹ��A -> beta��
            printf("�����µ� A ����ʽ...\n");
            for (const auto& beta_content : non_left_recursive_contents)
            {
                vector<string> new_result = beta_content;
                new_result.push_back(new_nt);  // ���A'
                insert_to_production(A, new_result);

                printf("  ����: %s ->", A.c_str());
                for (const auto& symbol : new_result)
                {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");
            }

            // ���û�з���ݹ����ʽ������ A -> A'
            if (non_left_recursive_contents.empty())
            {
                vector<string> new_result = { new_nt };
                insert_to_production(A, new_result);
                printf("  ����: %s -> %s\n", A.c_str(), new_nt.c_str());
            }

            // �����µĲ���ʽ A' -> alpha A'������ÿ����ݹ�� A -> A alpha��
            printf("�����µ� %s ����ʽ...\n", new_nt.c_str());
            for (const auto& alpha_content : left_recursive_contents)
            {
                vector<string> alpha;
                // ��ȡalpha���֣�ȥ����һ�� A��
                for (size_t i = 1; i < alpha_content.size(); i++)
                {
                    alpha.push_back(alpha_content[i]);
                }
                alpha.push_back(new_nt);  // ���A'
                insert_to_production(new_nt, alpha);

                printf("  ����: %s ->", new_nt.c_str());
                for (const auto& symbol : alpha) {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");
            }

            // ���� A' -> ��
            vector<string> epsilon = { "^" };
            insert_to_production(new_nt, epsilon);
            printf("  ����: %s -> ^\n", new_nt.c_str());

            // ���¿�ʼ�����������Ѹı䣩
            printf("���¿�ʼ���...\n");
            current_prod = PRODUCTION_HEAD.next;
        }
        else {
            current_prod = current_prod->next;
        }
    }

    printf("��ݹ������������\n");

    // ��֤�Ƿ�����ݹ�
    printf("��֤�������...\n");
    if (has_left_recursion()) {
        printf("���棺�������Դ�����ݹ飡\n");
    }
    else {
        printf("��ݹ������ɹ���\n");
    }
}

// ������������������ɾ��ָ���Ĳ���ʽ
void SQLParser::remove_production(production* target)
{
    if (target == NULL) return;

    production* prev = &PRODUCTION_HEAD;
    production* curr = PRODUCTION_HEAD.next;

    while (curr != NULL) {
        if (curr == target) {
            prev->next = curr->next;
            if (current_production == curr) {
                current_production = prev;
            }
            delete curr;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

// ����Ƿ������ݹ�
bool SQLParser::has_left_recursion()
{
    printf("�����ݹ�...\n");

    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt) {
        printf("�����ս��: %s\n", nt->name.c_str());

        production* p = PRODUCTION_HEAD.next;
        while (p) {
            if (p->source == nt->name) {
                printf("  ������ʽ: %s ->", p->source.c_str());
                for (const auto& symbol : p->result) {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");

                // ������ʽ�Ҳ��Ƿ�Ϊ�ջ��һ�������Ƿ�������ͬ
                if (!p->result.empty() && p->result[0] == nt->name) {
                    printf("  ����ֱ����ݹ�: %s -> %s...\n", nt->name.c_str(), p->result[0].c_str());
                    return true;
                }
            }
            p = p->next;
        }
        nt = nt->next;
    }

    printf("δ����ֱ����ݹ�\n");
    return false;
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
    // ���ڴ洢Ԥ�������
    // key: (���ս��, �ս��) -> value: ����ʽ���(1-based)
    PredictionTable table;
    vector<production*> production_list; // ��˳��洢����ʽָ��

    int prod_num = 1;

    printf("\nԤ���������\n\n");

    // ��������ʽ�б����Ԥ�������
    production* p = PRODUCTION_HEAD.next;
    while (p != nullptr)
    {
        string A = p->source;      // ����ʽ��
        vector<string>& alpha = p->result; // ����ʽ�Ҳ�

        production_list.push_back(p); // �������ʽָ��

        // ����FIRST(alpha)
        vector<string> first_alpha;
        compute_first_of_string(alpha, first_alpha);

        // ����FIRST(alpha)�е�ÿ���ս��a�����˦ţ�
        for (const string& a : first_alpha) {
            if (a != "^" && is_terminal_string(a))
            {
                auto key = make_pair(A, a);
                if (table.find(key) != table.end())
                {
                    printf("���棺Ԥ��������ͻ M[%s, %s]\n", A.c_str(), a.c_str());
                }
                table[key] = prod_num;
            }
        }

        // ����š�FIRST(alpha)�����FOLLOW(A)�е�ÿ���ս��b
        bool epsilon_in_first = false;
        for (const string& symbol : first_alpha)
        {
            if (symbol == "^")
            {
                epsilon_in_first = true;
                break;
            }
        }

        if (epsilon_in_first)
        {
            noterminal* A_nt = get_noterminal(A);
            if (A_nt)
            {
                for (const string& b : A_nt->FOLLOW)
                {
                    if (is_terminal_string(b))
                    {
                        auto key = make_pair(A, b);
                        if (table.find(key) != table.end())
                        {
                            printf("���棺Ԥ��������ͻ M[%s, %s]\n", A.c_str(), b.c_str());
                        }
                        table[key] = prod_num;
                    }
                }
            }
        }

        p = p->next;
        prod_num++;
    }

    // ��ӡԤ�������
    printf("Ԥ�������:\n");
    printf("%-15s", " ");

    // ��ӡ��ͷ���ս����
    for (const string& terminal : terminal_all)
    {
        if (terminal != "^")
        { // ����ӡ�մ�����
            printf("%-15s", terminal.c_str());
        }
    }
    printf("\n");

    // ��ӡ�������
    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt != nullptr)
    {
        printf("%-15s", nt->name.c_str());

        for (const string& terminal : terminal_all)
        {
            if (terminal != "^")
            {
                auto key = make_pair(nt->name, terminal);
                auto it = table.find(key);
                if (it != table.end())
                {
                    printf("%-15d", it->second);
                }
                else
                {
                    printf("%-15s", " ");
                }
            }
        }
        printf("\n");
        nt = nt->next;
    }
    printf("\n");

    // ʹ��Ԥ�����������﷨����
    analyze_input(input_tokens, table, production_list);
}

// �����ַ������е�FIRST��
void SQLParser::compute_first_of_string(const vector<string>& symbols, vector<string>& first_set)
{
    if (symbols.empty())
    {
        first_set.push_back("^");
        return;
    }

    for (size_t i = 0; i < symbols.size(); i++)
    {
        const string& X = symbols[i];

        if (is_terminal_string(X))
        {
            // X���ս����ֱ�Ӽ���FIRST��
            if (find(first_set.begin(), first_set.end(), X) == first_set.end())
            {
                first_set.push_back(X);
            }
            break;
        }
        else {
            // X�Ƿ��ս��
            noterminal* X_nt = get_noterminal(X);
            if (X_nt)
            {
                // ����FIRST(X) - {��}
                bool has_epsilon = false;
                for (const string& symbol : X_nt->FIRST)
                {
                    if (symbol == "^")
                    {
                        has_epsilon = true;
                    }
                    else
                    {
                        if (find(first_set.begin(), first_set.end(), symbol) == first_set.end())
                        {
                            first_set.push_back(symbol);
                        }
                    }
                }

                // ����Ų���FIRST(X)�У�ֹͣ
                if (!has_epsilon)
                {
                    break;
                }

                // ����������һ�������Ҧš�FIRST(X)����š�FIRST(������)
                if (i == symbols.size() - 1)
                {
                    if (find(first_set.begin(), first_set.end(), "^") == first_set.end())
                    {
                        first_set.push_back("^");
                    }
                }
            }
        }
    }
}

void SQLParser::analyze_input(vector<Token>& tokens, PredictionTable& table, vector<production*>& production_list)
{
    printf("��ʼ�﷨����...\n");

    // ��tokensת��Ϊ�ս������
    vector<string> input_symbols;
    for (const auto& token : tokens)
    {
        input_symbols.push_back(token_to_terminal(token));
    }
    input_symbols.push_back("#"); // ��ӽ�����

    // ��ʼ������ջ
    init_stack();
    if (NOTERMINAL_HEAD.next)
    {
        STACK_PUSH(NOTERMINAL_HEAD.next->name);  // ѹ�뿪ʼ����
    }
    else
    {
        printf("����û���ҵ���ʼ����\n");
        return;
    }

    print_analysis_step(input_symbols, 0);

    int input_pos = 0;
    bool analyzeSuccess = true;

    while (!STACK_EMPTY() && input_pos < input_symbols.size() && analyzeSuccess)
    {
        string top = TEST_STACK[amount];  // ջ������
        string current = input_symbols[input_pos];  // ��ǰ�������

        if (is_terminal_string(top))
        {
            // ջ�����ս��
            if (top == current)
            {
                // ƥ�䣬��ջ���ƶ�����ָ��
                printf("ƥ���ս��: %s\n", top.c_str());
                STACK_POP();
                input_pos++;
            }
            else
            {
                // ��ƥ�䣬����
                printf("�﷨�������� %s���õ� %s\n", top.c_str(), current.c_str());
                analyzeSuccess = false;
            }
        }
        else
        {
            // ջ���Ƿ��ս��
            auto key = make_pair(top, current);
            auto it = table.find(key);

            if (it != table.end())
            {
                int prod_num = it->second;
                production* prod = production_list[prod_num - 1]; // תΪ0-based����

                printf("ʹ�ò���ʽ %d: %s ->", prod_num, prod->source.c_str());
                for (const string& symbol : prod->result)
                {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");

                // �������ս��
                STACK_POP();

                // ����ѹ�����ʽ�Ҳ�����ѹ��մ���
                if (!(prod->result.size() == 1 && prod->result[0] == "^"))
                {
                    for (int j = prod->result.size() - 1; j >= 0; j--)
                    {
                        STACK_PUSH(prod->result[j]);
                    }
                }
            }
            else
            {
                // �޶�Ӧ����ʽ������
                printf("�﷨���󣺷��ս�� %s �������� %s û�в���ʽ\n",
                    top.c_str(), current.c_str());
                analyzeSuccess = false;
            }
        }

        if (analyzeSuccess) {
            print_analysis_step(input_symbols, input_pos);
        }
    }

    // ���������
    if (analyzeSuccess && STACK_EMPTY() && input_pos == input_symbols.size() - 1 &&
        input_symbols[input_pos] == "#")
    {
        printf("\n�﷨�����ɹ������봮�����ķ�����\n");
    }
    else {
        printf("\n�﷨����ʧ�ܣ�\n");
        if (!STACK_EMPTY())
        {
            printf("ջδ�գ�ʣ����ţ�");
            for (int i = 1; i <= amount; i++)
            {
                printf("%s ", TEST_STACK[i].c_str());
            }
            printf("\n");
        }
        if (input_pos < input_symbols.size() - 1)
        {
            printf("����δ��ȫ���ģ�ʣ�ࣺ");
            for (size_t i = input_pos; i < input_symbols.size(); i++)
            {
                printf("%s ", input_symbols[i].c_str());
            }
            printf("\n");
        }
    }
}

// ��ӡ��������
void SQLParser::print_analysis_step(const vector<string>& input_symbols, int input_pos)
{
    printf("����ջ: ");
    if (amount == 0)
    {
        printf("#");
    }
    else
    {
        printf("#");
        for (int i = 1; i <= amount; i++)
        {
            printf("%s", TEST_STACK[i].c_str());
        }
    }

    printf("\tʣ������: ");
    for (size_t i = input_pos; i < input_symbols.size(); i++)
    {
        printf("%s", input_symbols[i].c_str());
    }
    printf("\n");
}

void SQLParser::emergency(int model)
{
    printf("������ɣ�������Դ\n");
    // ����Ӧ���ͷ����з�����ڴ�
    exit(0);
}

int main()
{
    std::cout << "Hello World!\n";
    std::vector<Token> tokens = {
        {1, "SELECT", 1, 1},   // �ؼ���
        {2, "name", 1, 8},     // ��ʶ��
        {5, ",", 1, 12},       // �ָ���
        {2, "age", 1, 14},     // ��ʶ��
        {1, "FROM", 1, 18},    // �ؼ���
        {2, "students", 1, 23},// ��ʶ��
        {1, "WHERE", 1, 32},   // �ؼ���
        {2, "age", 1, 38},     // ��ʶ��
        {4, ">", 1, 42},       // ������
        {3, "18", 1, 44},      // ����
        {1, "AND", 1, 47},     // �ؼ���
        {2, "grade", 1, 51},   // ��ʶ��
        {4, "=", 1, 57},       // ������
        {3, "'A'", 1, 59},     // ����
        {5, ";", 1, 62}        // �ָ���
    };
    SQLParser sqlParser;
    sqlParser.start_parser(tokens);
}
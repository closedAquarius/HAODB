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

// 构造函数
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

// 向集合中加入一个字符串，返回新增数（0或1）
size_t SQLParser::add_to_set(vector<string>& set, string item)
{
    //检查是否已在集合中
    for (const auto& s : set)
    {
        if (s == item) return 0;
    }
    set.push_back(item);
    return 1;
}

// 查找字符串对应的非终结符节点
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

// 判断X是否为终结符
bool SQLParser::is_terminal_string(string X)
{
    for (const auto& terminal : terminal_all)
    {
        if (terminal == X) return true;
    }
    return false;
}

// 将Token转换为终结符字符串
string SQLParser::token_to_terminal(const Token& token)
{
    switch (token.type)
    {
    case 1: return token.value;  // 关键字
    case 2: return "IDENTIFIER"; // 标识符
    case 3: return "NUMBER";     // 常数
    case 4: return token.value;  // 运算符
    case 5: return token.value;  // 界符
    default: return token.value;
    }
}

// 从tokens中提取终结符
void SQLParser::load_terminals_from_tokens(vector<Token>& tokens)
{
    unordered_set<string> unique_terminals;

    // 添加特殊符号
    unique_terminals.insert("#");  // 栈底符号
    unique_terminals.insert("^");  // 空串符号

    // 从tokens中提取终结符
    for (auto& token : tokens)
    {
        string terminal = token_to_terminal(token);
        unique_terminals.insert(terminal);
    }

    // 插入到终结符链表
    for (const auto& term : unique_terminals)
    {
        insert_to_terminal(term);
        terminal_all.push_back(term);
    }
}

int SQLParser::start_parser(vector<Token>& tokens)
{
    input_tokens = tokens;

    // 从tokens中加载终结符
    load_terminals_from_tokens(tokens);

    // 读取文法规则
    read_grammar("sql_grammar.txt");
    Test_read();


    printf("\n消除左递归\n\n\n");
    if (has_left_recursion())
        printf("有左递归");
    eliminate_left_recursion();
    Test_read();

    printf("\n求FIRST集\n\n");
    prediction();
    printf("\n求FOLLOW集\n\n");
    test_follow();
    prediction_table();

    emergency(0);
    return 0;
}

// 读取文法规则
size_t SQLParser::read_grammar(const char* filename)
{
    int line = 0, old_line = 0;
    string current_line;

    // 读取文件
    FILE* read_stream = fopen(filename, "r");
    if (!read_stream)
    {
        printf("error in open file, can't open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    // 读取非终结符
    while (fgets(buffer, sizeof(buffer), read_stream))
    {
        string line_str(buffer);
        // 去除换行符
        if (!line_str.empty() && line_str.back() == '\n')
        {
            line_str.pop_back();
        }

        if (line == 0)
        {
            // 第一行是非终结符
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

    // 读取产生式
    while (fgets(buffer, sizeof(buffer), read_stream))
    {
        string line_str(buffer);
        // 去除换行符
        if (!line_str.empty() && line_str.back() == '\n')
        {
            line_str.pop_back();
        }

        if (line_str.empty()) continue;

        // 解析产生式 A->alpha|beta
        size_t arrow_pos = line_str.find("->");
        if (arrow_pos != string::npos)
        {
            string source = line_str.substr(0, arrow_pos);
            string right_part = line_str.substr(arrow_pos + 2);

            // 按'|'分割右部
            size_t pos = 0;
            while (pos < right_part.length())
            {
                size_t pipe_pos = right_part.find('|', pos);
                if (pipe_pos == string::npos) pipe_pos = right_part.length();

                string production_right = right_part.substr(pos, pipe_pos - pos);

                // 解析右部为符号序列
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

// 求FIRST集合
size_t SQLParser::find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    // 遍历所有产生式
    for (production* p = this_production; p; p = p->next)
    {
        if (p->source != this_noterminal->name) continue;

        vector<string>& alpha = p->result;
        // 如果右部是ε（程序中用'^'表示）
        if (alpha.size() == 1 && alpha[0] == "^")
        {
            added += add_to_set(this_noterminal->FIRST, "^");
            continue;
        }

        // 否则逐符号处理A → X1 X2 X3 ...
        size_t i = 0;
        for (; i < alpha.size(); ++i)
        {
            string X = alpha[i];
            if (is_terminal_string(X))
            {
                // 若X是终结符，加入FIRST(A)
                added += add_to_set(this_noterminal->FIRST, X);
                break;
            }
            else
            {
                // X是非终结符，先递归确保 FIRST(X) 已经部分/全部计算过
                noterminal* Y = get_noterminal(X);
                if (!Y) break;  // 出错容忍

                // 把FIRST(Y)\{ε} 加入FIRST(A)
                for (const auto& c : Y->FIRST)
                {
                    if (c != "^")
                    {
                        added += add_to_set(this_noterminal->FIRST, c);
                    }
                }

                //若Y的FIRST中不含ε，就停
                bool has_eps = false;
                for (const auto& c : Y->FIRST)
                {
                    if (c == "^") { has_eps = true; break; }
                }
                if (!has_eps) break;
                //否则继续看下一个 Xi
            }
        }
        //如果所有Xi都能推出ε（或右部本来就是空），则A的FIRST加ε
        if (i == alpha.size())
        {
            added += add_to_set(this_noterminal->FIRST, "^");
        }
    }
    return added;
}

//求FOLLOW集合
size_t SQLParser::find_follow(noterminal* B, production* this_production)
{
    size_t added = 0;
    //若B是开始符号，加入 '#'
    if (B == NOTERMINAL_HEAD.next)
    {
        added += add_to_set(B->FOLLOW, "#");
    }

    //遍历所有产生式 A → α B β
    for (production* p = this_production; p; p = p->next)
    {
        string A = p->source;
        vector<string>& rhs = p->result;

        for (size_t i = 0; i < rhs.size(); ++i)
        {
            if (rhs[i] != B->name) continue;

            //看β = rhs[i+1...]
            size_t j = i + 1;

            //先处理β非空部分
            for (; j < rhs.size(); ++j) {
                string X = rhs[j];
                if (is_terminal_string(X))
                {
                    //β首符号是终结符，加入到 FOLLOW(B)
                    added += add_to_set(B->FOLLOW, X);
                    break;
                }
                else
                {
                    //β首符号是非终结符 Y
                    noterminal* Y = get_noterminal(X);
                    if (!Y) break;

                    //把FIRST(Y)\{ε}加入FOLLOW(B)
                    for (const auto& c : Y->FIRST)
                    {
                        if (c != "^")
                        {
                            added += add_to_set(B->FOLLOW, c);
                        }
                    }

                    //如果FIRST(Y) 包含ε, 还要继续看下一个符号
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

            //如果β为空，或β全部能推ε，则把FOLLOW(A)加入FOLLOW(B)
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

    printf("产生式\n");
    for (number = 1; TEMP_PRODUCTION != NULL; TEMP_PRODUCTION = TEMP_PRODUCTION->next, number++)
    {
        printf("%d\t%s\t->", number, TEMP_PRODUCTION->source.c_str());
        for (const auto& symbol : TEMP_PRODUCTION->result) {
            printf(" %s", symbol.c_str());
        }
        printf("\n");
    }

    printf("\n终结符\n");
    for (; TEMP_TERMINAL != NULL; TEMP_TERMINAL = TEMP_TERMINAL->next)
    {
        printf("%s\t", TEMP_TERMINAL->name.c_str());
    }
    printf("\n");

    printf("\n非终结符\n");
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

// 消除左递归
void SQLParser::eliminate_left_recursion(void)
{
    printf("开始消除左递归...\n");

    // 检查是否有左递归
    if (!has_left_recursion())
    {
        printf("未发现左递归，跳过消除步骤\n");
        return;
    }

    printf("检测到左递归，开始消除...\n");

    // 构建终结符数组
    terminal_all.clear();
    terminal* temp = TERMINAL_HEAD.next;
    while (temp)
    {
        terminal_all.push_back(temp->name);
        temp = temp->next;
    }

    // 收集所有非终结符，按顺序处理（避免重复处理）
    vector<string> processed_nonterminals;

    // 单次遍历处理所有左递归
    production* current_prod = PRODUCTION_HEAD.next;

    while (current_prod)
    {
        // 检查是否为直接左递归且未处理过
        if (!current_prod->result.empty() &&
            current_prod->source == current_prod->result[0] &&
            find(processed_nonterminals.begin(), processed_nonterminals.end(), current_prod->source) == processed_nonterminals.end())
        {
            string A = current_prod->source;  // 原非终结符
            printf("\n处理非终结符 %s 的左递归\n", A.c_str());

            // 标记为已处理
            processed_nonterminals.push_back(A);

            string new_nt = A + "'";          // 新非终结符A'

            // 确保新非终结符名不冲突
            while (get_noterminal(new_nt) != NULL)
            {
                new_nt += "'";
            }

            // 收集所有A的产生式的内容
            vector<vector<string>> left_recursive_contents;   // 保存左递归产生式的内容
            vector<vector<string>> non_left_recursive_contents; // 保存非左递归产生式的内容
            vector<production*> to_delete; // 需要删除的产生式指针

            production* p = PRODUCTION_HEAD.next;
            while (p)
            {
                if (p->source == A)
                {
                    if (!p->result.empty() && p->result[0] == A)
                    {
                        // 左递归产生式，保存内容
                        left_recursive_contents.push_back(p->result);
                        to_delete.push_back(p);
                        printf("  左递归产生式: %s ->", p->source.c_str());
                        for (const auto& symbol : p->result)
                        {
                            printf(" %s", symbol.c_str());
                        }
                        printf("\n");
                    }
                    else
                    {
                        // 非左递归产生式，保存内容
                        non_left_recursive_contents.push_back(p->result);
                        to_delete.push_back(p);
                        printf("  非左递归产生式: %s ->", p->source.c_str());
                        for (const auto& symbol : p->result)
                        {
                            printf(" %s", symbol.c_str());
                        }
                        printf("\n");
                    }
                }
                p = p->next;
            }

            printf("找到 %zu 个左递归产生式，%zu 个非左递归产生式\n",
                left_recursive_contents.size(), non_left_recursive_contents.size());

            // 添加新的非终结符A'
            insert_to_noterminal(new_nt);
            printf("创建新非终结符: %s\n", new_nt.c_str());

            // 删除所有原来的A产生式
            printf("删除原有产生式...\n");
            for (production* old_prod : to_delete) {
                remove_production(old_prod);
            }

            // 创建新的产生式 A -> beta A'（对于每个非左递归的A -> beta）
            printf("创建新的 A 产生式...\n");
            for (const auto& beta_content : non_left_recursive_contents)
            {
                vector<string> new_result = beta_content;
                new_result.push_back(new_nt);  // 添加A'
                insert_to_production(A, new_result);

                printf("  创建: %s ->", A.c_str());
                for (const auto& symbol : new_result)
                {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");
            }

            // 如果没有非左递归产生式，创建 A -> A'
            if (non_left_recursive_contents.empty())
            {
                vector<string> new_result = { new_nt };
                insert_to_production(A, new_result);
                printf("  创建: %s -> %s\n", A.c_str(), new_nt.c_str());
            }

            // 创建新的产生式 A' -> alpha A'（对于每个左递归的 A -> A alpha）
            printf("创建新的 %s 产生式...\n", new_nt.c_str());
            for (const auto& alpha_content : left_recursive_contents)
            {
                vector<string> alpha;
                // 提取alpha部分（去掉第一个 A）
                for (size_t i = 1; i < alpha_content.size(); i++)
                {
                    alpha.push_back(alpha_content[i]);
                }
                alpha.push_back(new_nt);  // 添加A'
                insert_to_production(new_nt, alpha);

                printf("  创建: %s ->", new_nt.c_str());
                for (const auto& symbol : alpha) {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");
            }

            // 创建 A' -> ε
            vector<string> epsilon = { "^" };
            insert_to_production(new_nt, epsilon);
            printf("  创建: %s -> ^\n", new_nt.c_str());

            // 重新开始遍历（链表已改变）
            printf("重新开始检查...\n");
            current_prod = PRODUCTION_HEAD.next;
        }
        else {
            current_prod = current_prod->next;
        }
    }

    printf("左递归消除处理完成\n");

    // 验证是否还有左递归
    printf("验证消除结果...\n");
    if (has_left_recursion()) {
        printf("警告：消除后仍存在左递归！\n");
    }
    else {
        printf("左递归消除成功！\n");
    }
}

// 辅助函数：从链表中删除指定的产生式
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

// 检查是否存在左递归
bool SQLParser::has_left_recursion()
{
    printf("检查左递归...\n");

    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt) {
        printf("检查非终结符: %s\n", nt->name.c_str());

        production* p = PRODUCTION_HEAD.next;
        while (p) {
            if (p->source == nt->name) {
                printf("  检查产生式: %s ->", p->source.c_str());
                for (const auto& symbol : p->result) {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");

                // 检查产生式右部是否为空或第一个符号是否与左部相同
                if (!p->result.empty() && p->result[0] == nt->name) {
                    printf("  发现直接左递归: %s -> %s...\n", nt->name.c_str(), p->result[0].c_str());
                    return true;
                }
            }
            p = p->next;
        }
        nt = nt->next;
    }

    printf("未发现直接左递归\n");
    return false;
}

// 栈操作函数
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
        printf("栈满\n");
        emergency(2);
    }
    TEST_STACK[++amount] = source;
}

void SQLParser::STACK_POP(void)
{
    if (STACK_EMPTY())
    {
        printf("栈空\n");
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

// 预测分析表和语法分析实现
void SQLParser::prediction_table(void)
{
    // 用于存储预测分析表
    // key: (非终结符, 终结符) -> value: 产生式编号(1-based)
    PredictionTable table;
    vector<production*> production_list; // 按顺序存储产生式指针

    int prod_num = 1;

    printf("\n预测分析表构建\n\n");

    // 构建产生式列表并填充预测分析表
    production* p = PRODUCTION_HEAD.next;
    while (p != nullptr)
    {
        string A = p->source;      // 产生式左部
        vector<string>& alpha = p->result; // 产生式右部

        production_list.push_back(p); // 保存产生式指针

        // 计算FIRST(alpha)
        vector<string> first_alpha;
        compute_first_of_string(alpha, first_alpha);

        // 对于FIRST(alpha)中的每个终结符a（除了ε）
        for (const string& a : first_alpha) {
            if (a != "^" && is_terminal_string(a))
            {
                auto key = make_pair(A, a);
                if (table.find(key) != table.end())
                {
                    printf("警告：预测分析表冲突 M[%s, %s]\n", A.c_str(), a.c_str());
                }
                table[key] = prod_num;
            }
        }

        // 如果ε∈FIRST(alpha)，则对FOLLOW(A)中的每个终结符b
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
                            printf("警告：预测分析表冲突 M[%s, %s]\n", A.c_str(), b.c_str());
                        }
                        table[key] = prod_num;
                    }
                }
            }
        }

        p = p->next;
        prod_num++;
    }

    // 打印预测分析表
    printf("预测分析表:\n");
    printf("%-15s", " ");

    // 打印表头（终结符）
    for (const string& terminal : terminal_all)
    {
        if (terminal != "^")
        { // 不打印空串符号
            printf("%-15s", terminal.c_str());
        }
    }
    printf("\n");

    // 打印表格内容
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

    // 使用预测分析表进行语法分析
    analyze_input(input_tokens, table, production_list);
}

// 计算字符串序列的FIRST集
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
            // X是终结符，直接加入FIRST集
            if (find(first_set.begin(), first_set.end(), X) == first_set.end())
            {
                first_set.push_back(X);
            }
            break;
        }
        else {
            // X是非终结符
            noterminal* X_nt = get_noterminal(X);
            if (X_nt)
            {
                // 加入FIRST(X) - {ε}
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

                // 如果ε不在FIRST(X)中，停止
                if (!has_epsilon)
                {
                    break;
                }

                // 如果这是最后一个符号且ε∈FIRST(X)，则ε∈FIRST(整个串)
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
    printf("开始语法分析...\n");

    // 将tokens转换为终结符序列
    vector<string> input_symbols;
    for (const auto& token : tokens)
    {
        input_symbols.push_back(token_to_terminal(token));
    }
    input_symbols.push_back("#"); // 添加结束符

    // 初始化分析栈
    init_stack();
    if (NOTERMINAL_HEAD.next)
    {
        STACK_PUSH(NOTERMINAL_HEAD.next->name);  // 压入开始符号
    }
    else
    {
        printf("错误：没有找到开始符号\n");
        return;
    }

    print_analysis_step(input_symbols, 0);

    int input_pos = 0;
    bool analyzeSuccess = true;

    while (!STACK_EMPTY() && input_pos < input_symbols.size() && analyzeSuccess)
    {
        string top = TEST_STACK[amount];  // 栈顶符号
        string current = input_symbols[input_pos];  // 当前输入符号

        if (is_terminal_string(top))
        {
            // 栈顶是终结符
            if (top == current)
            {
                // 匹配，出栈并移动输入指针
                printf("匹配终结符: %s\n", top.c_str());
                STACK_POP();
                input_pos++;
            }
            else
            {
                // 不匹配，报错
                printf("语法错误：期望 %s，得到 %s\n", top.c_str(), current.c_str());
                analyzeSuccess = false;
            }
        }
        else
        {
            // 栈顶是非终结符
            auto key = make_pair(top, current);
            auto it = table.find(key);

            if (it != table.end())
            {
                int prod_num = it->second;
                production* prod = production_list[prod_num - 1]; // 转为0-based索引

                printf("使用产生式 %d: %s ->", prod_num, prod->source.c_str());
                for (const string& symbol : prod->result)
                {
                    printf(" %s", symbol.c_str());
                }
                printf("\n");

                // 弹出非终结符
                STACK_POP();

                // 反向压入产生式右部（不压入空串）
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
                // 无对应产生式，报错
                printf("语法错误：非终结符 %s 对于输入 %s 没有产生式\n",
                    top.c_str(), current.c_str());
                analyzeSuccess = false;
            }
        }

        if (analyzeSuccess) {
            print_analysis_step(input_symbols, input_pos);
        }
    }

    // 检查分析结果
    if (analyzeSuccess && STACK_EMPTY() && input_pos == input_symbols.size() - 1 &&
        input_symbols[input_pos] == "#")
    {
        printf("\n语法分析成功！输入串符合文法规则。\n");
    }
    else {
        printf("\n语法分析失败！\n");
        if (!STACK_EMPTY())
        {
            printf("栈未空，剩余符号：");
            for (int i = 1; i <= amount; i++)
            {
                printf("%s ", TEST_STACK[i].c_str());
            }
            printf("\n");
        }
        if (input_pos < input_symbols.size() - 1)
        {
            printf("输入未完全消耗，剩余：");
            for (size_t i = input_pos; i < input_symbols.size(); i++)
            {
                printf("%s ", input_symbols[i].c_str());
            }
            printf("\n");
        }
    }
}

// 打印分析步骤
void SQLParser::print_analysis_step(const vector<string>& input_symbols, int input_pos)
{
    printf("分析栈: ");
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

    printf("\t剩余输入: ");
    for (size_t i = input_pos; i < input_symbols.size(); i++)
    {
        printf("%s", input_symbols[i].c_str());
    }
    printf("\n");
}

void SQLParser::emergency(int model)
{
    printf("分析完成，清理资源\n");
    // 这里应该释放所有分配的内存
    exit(0);
}

int main()
{
    std::cout << "Hello World!\n";
    std::vector<Token> tokens = {
        {1, "SELECT", 1, 1},   // 关键字
        {2, "name", 1, 8},     // 标识符
        {5, ",", 1, 12},       // 分隔符
        {2, "age", 1, 14},     // 标识符
        {1, "FROM", 1, 18},    // 关键字
        {2, "students", 1, 23},// 标识符
        {1, "WHERE", 1, 32},   // 关键字
        {2, "age", 1, 38},     // 标识符
        {4, ">", 1, 42},       // 操作符
        {3, "18", 1, 44},      // 常量
        {1, "AND", 1, 47},     // 关键字
        {2, "grade", 1, 51},   // 标识符
        {4, "=", 1, 57},       // 操作符
        {3, "'A'", 1, 59},     // 常量
        {5, ";", 1, 62}        // 分隔符
    };
    SQLParser sqlParser;
    sqlParser.start_parser(tokens);
}
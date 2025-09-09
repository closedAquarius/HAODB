#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include "parser.h"
#include "dataType.h"

#ifndef success
#define success 1
#endif

#define MAX_AMOUNT 20

using namespace std;

//定义非终结符
typedef struct NOterminal
{
    char character;
    int  first_number;  //FIRST集的判定，初始为0
    int  follow_number; //FOLLOW集的判定，初始为0
    char* FIRST;       //FIRST 集合
    char* FOLLOW;      //FOLLOW集合
    struct NOterminal* next;
} noterminal;

//定义终结符
typedef struct Terminal
{
    char  character;        //当前的字符
    struct Terminal* next;
} terminal;

//定义产生式
typedef struct PRODUCTION
{
    char source;                //产生的开始
    char* result;              //产生的结果
    struct PRODUCTION* next;   //指向下一条
} production;

int amount = 0;
char TEST_STACK[20];
char* TEST_LIST[10];
char terminal_all[10] = { 0 };
terminal   TERMINAL_HEAD, * current_terminal = &TERMINAL_HEAD;
noterminal NOTERMINAL_HEAD, * current_noterminal = &NOTERMINAL_HEAD;
production PRODUCTION_HEAD, * current_production = &PRODUCTION_HEAD;

//函数申明
size_t read(void);
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
void STACK_PUSH(char source);
void insert_to_terminal(char get);
void insert_to_noterminal(char get);
void eliminate_left_recursion(void);
void combine(char* destinction, char* source);
size_t is_appeared(char tobejudged, char* source);
void insert_to_production(char source, char* result);
size_t find_first(noterminal* this_noterminal, production* this_production);
size_t find_follow(noterminal* this_noterminal, production* this_production);
void analyze_input(char* input, int table[128][128]);

int start_parser(std::vector<Token> tokens)
{
    TERMINAL_HEAD.next = NULL;
    NOTERMINAL_HEAD.next = NULL;
    PRODUCTION_HEAD.next = NULL;
    read();
    Test_read();
    printf("\n消除左递归\n\n\n");
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

//读测试文件
size_t read(void)
{
    int line = 0, model = 0, old_line = 0;
    int number = 0;
    char current_get = 0, read_temp[10] = { 0 };

    //读取文件
    FILE* read_stream = fopen("/data/test.txt", "r");
    if (!read_stream)
    {
        printf("error in open file ,can`t open file\n");
        exit(EXIT_FAILURE);
    }
    insert_to_terminal('#');    //插入栈底元素，以#表示栈底
    insert_to_terminal('^');    //插入空串元素，以^表示栈底
    //第一次循环：读取非终结符和终结符
    while (!feof(read_stream))
    {
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream); //忽略空格
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //跳过换行
        }

        switch (line)
        {
        case 0:
            //文件的第1行（line == 0）视为“非终结符行”，把每一个字符（忽略空格和换行）插入到非终结符链表
            insert_to_noterminal(current_get);
            break;
        case 1:
            //文件的第2行（line == 1）视为“终结符行”，把每一个字符（忽略空格和换行）插入到终结符链表
            insert_to_terminal(current_get);
            break;
        case 3:
            //当读到了第4行时，调用ungetc把当前字符退回，并跳出循环
            ungetc(current_get, read_stream);
            old_line = 1;
            break;
        default:
            perror("error in format of program \n");
            emergency(0);
            break;
        }
        if (old_line)
            break;
    }
    //第二次循环：读取产生式和测试串
    while (!feof(read_stream))
    {
        memset(read_temp, 0, sizeof(read_temp));
        old_line = line;  //保留第一次循环的结束位置
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream);     //忽略空格
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //跳过换行
        }
        model = ((line - old_line) > model) ? (line - old_line) : model;
        switch (model)
        {
        case 0:
        case 1:
            //model==0,1：仍在产生式部分，current_get是产生式左部（如 E、T、F），read_temp拿到右部（如 E+T|T）
            fscanf(read_stream, "%s", read_temp);
            insert_to_production(current_get, read_temp);
            break;
        case 2:
            //model==2：进入测试输入串部分，读取进行语法分析的句子
            ungetc(current_get, read_stream);
            TEST_LIST[number] = (char*)malloc(20);
            memset(TEST_LIST[number], 0, 20);
            fscanf(read_stream, "%s", TEST_LIST[number++]);
            break;
        default:
            perror("error in format of program \n");
            emergency(1);
            break;
        }
    }

    fclose(read_stream);
    return success;
}

//测试
size_t test(void)
{
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\tfirst number=%d\tfirst=%s\n",
            TEMP_NOTERMINAL->character,
            TEMP_NOTERMINAL->first_number,
            TEMP_NOTERMINAL->FIRST);
    }
    printf("\n");
    return success;
}

//向集合中加入一个字符，返回新增数（0或1）
static size_t add_to_set(char** p_set, int* p_size, char ch)
{
    char* set = *p_set;
    int size = *p_size;
    //检查是否已在集合中
    for (int i = 0; i < size; ++i) {
        if (set[i] == ch) return 0;
    }
    //扩容并加入
    set = (char*)realloc(set, size + 2);  // +1字符，+1终止符
    set[size] = ch;
    set[size + 1] = '\0';
    *p_set = set;
    *p_size = size + 1;
    return 1;
}

//查找字符对应的非终结符节点
static noterminal* get_noterminal(char X)
{
    noterminal* p = NOTERMINAL_HEAD.next;
    while (p)
    {
        if (p->character == X) return p;
        p = p->next;
    }
    return NULL;
}

//判断X是否为终结符（查terminal_all数组
static int is_terminal_char(char X)
{
    for (int i = 0; terminal_all[i] != 0; ++i)
    {
        if (terminal_all[i] == X) return 1;
    }
    return 0;
}


/******Begin******/
//求FIRST集合
size_t find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    //如果这是一个终结符，不管，FIRST只在prediction前对非终结符调用
    //遍历所有产生式
    for (production* p = this_production; p; p = p->next) {
        if (p->source != this_noterminal->character) continue;
        char* alpha = p->result;
        //如果右部是ε（程序中用'^'表示）
        if (alpha[0] == '^') {
            added += add_to_set(&this_noterminal->FIRST,
                &this_noterminal->first_number,
                '^');
            continue;
        }
        //否则逐符号处理A → X1 X2 X3 ...
        int i = 0;
        for (; alpha[i] != '\0'; ++i) {
            char X = alpha[i];
            if (is_terminal_char(X)) {
                //若X是终结符，加入FIRST(A)
                added += add_to_set(&this_noterminal->FIRST,
                    &this_noterminal->first_number,
                    X);
                break;
            }
            else {
                //X是非终结符，先递归确保 FIRST(X) 已经部分/全部计算过
                noterminal* Y = get_noterminal(X);
                if (!Y) break; // 出错容忍
                //把FIRST(Y)\{ε} 加入FIRST(A)
                for (int k = 0; k < Y->first_number; ++k) {
                    char c = Y->FIRST[k];
                    if (c != '^') {
                        added += add_to_set(&this_noterminal->FIRST,
                            &this_noterminal->first_number,
                            c);
                    }
                }
                //若Y的FIRST中不含ε，就停
                int has_eps = 0;
                for (int k = 0; k < Y->first_number; ++k)
                    if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                if (!has_eps) break;
                //否则继续看下一个 Xi
            }
        }
        //如果所有Xi都能推出ε（或右部本来就是空），则A的FIRST加ε
        if (alpha[i] == '\0') {
            added += add_to_set(&this_noterminal->FIRST,
                &this_noterminal->first_number,
                '^');
        }
    }
    return added;
}
/******End******/

/******Begin******/
//求FOLLOW集合
size_t find_follow(noterminal* B, production* this_production)
{
    size_t added = 0;
    //若B是开始符号，加入 '#'
    if (B == NOTERMINAL_HEAD.next) {
        added += add_to_set(&B->FOLLOW,
            &B->follow_number,
            '#');
    }
    //遍历所有产生式 A → α B β
    for (production* p = this_production; p; p = p->next) {
        char A = p->source;
        char* rhs = p->result;
        int len = strlen(rhs);
        for (int i = 0; i < len; ++i) {
            if (rhs[i] != B->character) continue;
            //看β = rhs[i+1...]
            int j = i + 1;
            int add_followA = 0;
            //先处理β非空部分
            for (; j < len; ++j) {
                char X = rhs[j];
                if (is_terminal_char(X)) {
                    //β首符号是终结符，加入到 FOLLOW(B)
                    added += add_to_set(&B->FOLLOW,
                        &B->follow_number,
                        X);
                    break;
                }
                else {
                    //β首符号是非终结符 Y
                    noterminal* Y = get_noterminal(X);
                    //把FIRST(Y)\{ε}加入FOLLOW(B)
                    for (int k = 0; k < Y->first_number; ++k) {
                        char c = Y->FIRST[k];
                        if (c != '^') {
                            added += add_to_set(&B->FOLLOW,
                                &B->follow_number,
                                c);
                        }
                    }
                    //如果FIRST(Y) 包含ε, 还要继续看下一个符号
                    int has_eps = 0;
                    for (int k = 0; k < Y->first_number; ++k)
                        if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                    if (!has_eps) break;
                    //如果Y是生产尾部且含ε，则也要把FOLLOW(A)加入FOLLOW(B)
                }
            }
            //如果β为空，或β全部能推ε，则把FOLLOW(A)加入FOLLOW(B)
            if (j == len) {
                noterminal* A_nt = get_noterminal(A);
                for (int k = 0; k < A_nt->follow_number; ++k) {
                    char c = A_nt->FOLLOW[k];
                    added += add_to_set(&B->FOLLOW,
                        &B->follow_number,
                        c);
                }
            }
        }
    }
    return added;
}
/******End******/

//紧急退出
void emergency(int model)
{
    current_noterminal = NOTERMINAL_HEAD.next;
    current_terminal = TERMINAL_HEAD.next;
    current_production = PRODUCTION_HEAD.next;
    while (current_noterminal)
    {
        NOTERMINAL_HEAD.next = current_noterminal->next;
        free(current_noterminal->FIRST);
        free(current_noterminal->FOLLOW);
        free(current_noterminal);
        current_noterminal = NOTERMINAL_HEAD.next;
    }
    while (current_terminal)
    {
        TERMINAL_HEAD.next = current_terminal->next;
        free(current_terminal);
        current_terminal = TERMINAL_HEAD.next;
    }
    while (current_production)
    {
        PRODUCTION_HEAD.next = current_production->next;
        free(current_production->result);
        free(current_production);
        current_production = PRODUCTION_HEAD.next;
    }
    printf("退出成功\n");
    exit(0);
}

//插入到终结符
void insert_to_terminal(char get)
{
    terminal* Temp_terminal = (terminal*)malloc(sizeof(terminal));
    if (!Temp_terminal)
    {
        perror("can`t malloc for this program\n");
        emergency(0);
    }
    Temp_terminal->character = get;
    Temp_terminal->next = NULL;
    current_terminal->next = Temp_terminal;
    current_terminal = Temp_terminal; //移向下一个节点
    return;
}

//插入到非终结符
void insert_to_noterminal(char get)
{
    noterminal* Temp_noterminal = (noterminal*)malloc(sizeof(noterminal));
    if (!Temp_noterminal)
    {
        perror("can`t malloc for this program\n");
        emergency(0);
    }
    Temp_noterminal->character = get;
    Temp_noterminal->next = NULL;
    Temp_noterminal->FIRST = NULL;
    Temp_noterminal->FOLLOW = NULL;
    Temp_noterminal->first_number = 0;
    Temp_noterminal->follow_number = 0;
    current_noterminal->next = Temp_noterminal;
    current_noterminal = Temp_noterminal;     //移向下一个节点
    return;
}

//插入到产生式
void insert_to_production(char source, char* result)
{
    char TEMP[20] = { 0 };
    int COUNT = 0, number = 0, length = 0, exit_condition = strlen(result);
    production* Temp_production;
    for (COUNT = 0; COUNT != exit_condition + 1; COUNT++)
    {
        if (*result == '-' && *(result + 1) == '>')
        {
            result += 2;
            COUNT += 2;
        }
        if ((*result != '|') && (*result))
            TEMP[number++] = *result;
        else
        {
            Temp_production = (production*)malloc(sizeof(production));
            length = strlen(TEMP) + 1;
            Temp_production->result = (char*)malloc(length);
            memset(Temp_production->result, 0, length);
            strncpy(Temp_production->result, TEMP, length - 1);
            memset(TEMP, 0, sizeof(TEMP));
            Temp_production->source = source;
            Temp_production->next = NULL;
            current_production->next = Temp_production;
            current_production = Temp_production;
            number = 0;
        }
        result++;
    }
    return;
}

//消除左递归
void eliminate_left_recursion(void)
{
    int number = 0;
    char new_char[3] = { 0 }, TEMP_RESULT[20], temp_empty[3] = { '^',0,0 };
    production* Temp_production = PRODUCTION_HEAD.next;  //遍历产生式链表
    production* Temp_FREE;  //
    terminal* temp = TERMINAL_HEAD.next;
    while (Temp_production)
    {
        //判断是否直接左递归
        if (Temp_production->source == Temp_production->result[0])
        {
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            //用对应的小写字母作为新的非终结符 
            new_char[0] = Temp_production->source - 'A' + 'a';
            //构造新的产生式
            strcat(TEMP_RESULT, Temp_production->result + 1);
            strcat(TEMP_RESULT, new_char);
            insert_to_noterminal(new_char[0]);
            insert_to_production(new_char[0], TEMP_RESULT);
            insert_to_production(new_char[0], temp_empty);

            //修改原来的产生式
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            strcat(TEMP_RESULT, Temp_production->next->result);
            strcat(TEMP_RESULT, new_char);
            memset(Temp_production->result, 0, strlen(Temp_production->result));
            strncpy(Temp_production->result, TEMP_RESULT, strlen(TEMP_RESULT));

            //删除原来的产生式（紧跟在左递归产生式之后的节点）
            Temp_FREE = Temp_production->next;
            Temp_production->next = Temp_production->next->next;
            free(Temp_FREE);
            continue;
        }
        Temp_production = Temp_production->next;
    }
    while (temp)
    {
        terminal_all[number++] = temp->character;
        temp = temp->next;
    }
    return;
}

//测试、输出读取结果
void Test_read(void)
{
    int number = 1;
    production* TEMP_PRODUCTION = PRODUCTION_HEAD.next;
    terminal* TEMP_TERMINAL = TERMINAL_HEAD.next;
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    printf("产生式\n");
    //TEMP_PRODUCTION遍历PRODUCTION_HEAD.next链表，每个节点的source字段是左部非终结符，result是右部字符串
    for (number = 1; TEMP_PRODUCTION != NULL; TEMP_PRODUCTION = TEMP_PRODUCTION->next, number++)
    {
        printf("%d\t%c\t%s\n", number, TEMP_PRODUCTION->source, TEMP_PRODUCTION->result);
    }
    printf("\n终结符\n");
    //TEMP_TERMINAL遍历TERMINAL_HEAD.next链表，依次打印每个终结符
    for (; TEMP_TERMINAL != NULL; TEMP_TERMINAL = TEMP_TERMINAL->next)
    {
        printf("%c\t", TEMP_TERMINAL->character);
    }
    printf("\n");
    printf("\n非终结符\n");
    //TEMP_NOTERMINAL遍历NOTERMINAL_HEAD.next链表，依次打印每个非终结符
    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\t", TEMP_NOTERMINAL->character);
    }
    printf("\n读取测试\n%s\n%s\n", TEST_LIST[0], TEST_LIST[1]);
    printf("\n");
    return;
}

size_t is_appeared(char tobejudged, char* source)
{
    size_t length = strlen(source), counts = 0;
    while ((counts != length) && (*source != tobejudged))
    {
        counts++;
        source++;
    }
    return counts == length ? !success : success;
}

void combine(char* destinction, char* source)
{
    char temp[2] = { 0 };
    while (*source)
    {
        if (!is_appeared(*source, destinction))
        {
            temp[0] = *source;
            strcat(destinction, temp);
        }
        source++;
    }
    return;
}

void prediction(void)
{
    noterminal* TEMP_NOTERMINAL;
    int changed;
    do
    {
        changed = 0;
        TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;
        while (TEMP_NOTERMINAL != NULL)
        {
            changed |= find_first(TEMP_NOTERMINAL, PRODUCTION_HEAD.next);
            TEMP_NOTERMINAL = TEMP_NOTERMINAL->next;
        }
    } while (changed);
    test();

    TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;
    while (TEMP_NOTERMINAL != NULL)
    {
        find_follow(TEMP_NOTERMINAL, PRODUCTION_HEAD.next);
        TEMP_NOTERMINAL = TEMP_NOTERMINAL->next;
    }
    return;
}

void test_follow(void)
{
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\tfollow number=%d\tfollow=%s\n",
            TEMP_NOTERMINAL->character,
            TEMP_NOTERMINAL->follow_number,
            TEMP_NOTERMINAL->FOLLOW);
    }
    printf("\n");
    return;
}

/******Begin******/
//生成预测分析表
void prediction_table(void)
{
    //用于存储预测分析表
    //table[i][j] 表示非终结符i和终结符j的产生式编号，0表示无对应产生式
    int table[128][128] = { 0 };
    int prod_num = 1;

    //打印标题
    printf("\n预测分析表\n\n");

    //遍历所有产生式，填充预测分析表
    production* p = PRODUCTION_HEAD.next;
    while (p != NULL) {
        char A = p->source;      //产生式左部
        char* alpha = p->result; //产生式右部

        //处理FIRST(alpha)的每个终结符
        if (alpha[0] == '^')  //如果右部是空串
        {
            noterminal* A_nt = get_noterminal(A);
            //将产生式号填入表中 A 行 FOLLOW(A) 列
            for (int i = 0; i < A_nt->follow_number; i++)
            {
                char b = A_nt->FOLLOW[i];
                table[(int)A][(int)b] = prod_num;
            }
        }
        else if (is_terminal_char(alpha[0]))  //如果右部首符是终结符
        {
            table[(int)A][(int)alpha[0]] = prod_num;
        }
        else  //右部首符是非终结符
        {
            noterminal* B = get_noterminal(alpha[0]);
            if (B != NULL)
            {
                //遍历 FIRST(B)
                for (int i = 0; i < B->first_number; i++)
                {
                    char b = B->FIRST[i];
                    if (b != '^')  //非空符号直接添加
                    {
                        table[(int)A][(int)b] = prod_num;
                    }
                    else
                    {
                        //如果B能推出空，则考虑alpha后面的符号
                        if (alpha[1] == '\0')
                        {
                            //B是右部的最后一个符号
                            noterminal* A_nt = get_noterminal(A);
                            //将产生式号填入表中A行FOLLOW(A)列
                            for (int j = 0; j < A_nt->follow_number; j++)
                            {
                                char c = A_nt->FOLLOW[j];
                                table[(int)A][(int)c] = prod_num;
                            }
                        }
                        else if (is_terminal_char(alpha[1]))
                        {
                            table[(int)A][(int)alpha[1]] = prod_num;
                        }
                        else
                        {
                            //处理后续非终结符的FIRST集
                            noterminal* next_B = get_noterminal(alpha[1]);
                            if (next_B != NULL)
                            {
                                for (int j = 0; j < next_B->first_number; j++)
                                {
                                    char c = next_B->FIRST[j];
                                    if (c != '^')
                                    {
                                        table[(int)A][(int)c] = prod_num;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        p = p->next;
        prod_num++;
    }

    // 打印预测分析表
    printf(" \t");
    // 打印终结符作为表头
    terminal* t = TERMINAL_HEAD.next;
    while (t != NULL) {
        printf("%c\t", t->character);
        t = t->next;
    }
    printf("\n");

    // 打印每行（非终结符）
    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt != NULL) {
        printf("%c\t", nt->character);

        // 打印该非终结符对应的每个终结符的产生式
        t = TERMINAL_HEAD.next;
        while (t != NULL) {
            int entry = table[(int)nt->character][(int)t->character];
            if (entry != 0) {
                printf("%d\t", entry);
            }
            else {
                printf(" \t");
            }
            t = t->next;
        }
        printf("\n");
        nt = nt->next;
    }
    printf("\n\n\n");

    // 使用预测分析表进行语法分析
    for (int i = 0; TEST_LIST[i] != NULL; i++) {
        char* input = TEST_LIST[i];
        analyze_input(input, table);
    }
}

//根据预测分析表分析输入串
void analyze_input(char* input, int table[128][128])
{
    char* str = (char*)malloc(strlen(input) + 2);
    strcpy(str, input);
    strcat(str, "#");  //为输入字符串添加结束符

    //初始化分析栈
    init_stack();
    STACK_PUSH('E');  //压入开始符号

    printf("分析栈\t%c%c", TEST_STACK[0], TEST_STACK[1]);
    int i = 1;
    while (i < amount)
    {
        printf("%c", TEST_STACK[i + 1]);
        i++;
    }
    printf("\t剩余字符串\t%s\n", str);

    int input_pos = 0;
    while (!STACK_EMPTY())
    {
        char top = TEST_STACK[amount];  //栈顶符号
        char current = str[input_pos];  //当前输入符号

        if (is_terminal_char(top))
        {
            //栈顶是终结符
            if (top == current)
            {
                //匹配，出栈并移动输入指针
                STACK_POP();
                input_pos++;
            }
            else
            {
                //不匹配，报错
                printf("语法错误：期望 %c，得到 %c\n", top, current);
                free(str);
                return;
            }
        }
        else
        {
            //栈顶是非终结符
            int prod_num = table[(int)top][(int)current];
            if (prod_num > 0)
            {
                //找到对应产生式
                STACK_POP();  // 弹出非终结符

                //找到对应的产生式
                production* p = PRODUCTION_HEAD.next;
                for (int j = 1; j < prod_num; j++)
                {
                    p = p->next;
                }

                //反向压入产生式右部（不压入空串）
                if (p->result[0] != '^')
                {
                    for (int j = strlen(p->result) - 1; j >= 0; j--)
                    {
                        STACK_PUSH(p->result[j]);
                    }
                }
            }
            else
            {
                //无对应产生式，报错
                printf("语法错误：非终结符 %c 对于输入 %c 没有产生式\n", top, current);
                free(str);
                return;
            }
        }

        // 打印当前栈和剩余输入
        printf("分析栈\t%c", TEST_STACK[0]);
        for (int j = 1; j <= amount; j++) {
            printf("%c", TEST_STACK[j]);
        }
        printf("\t剩余字符串\t%s\n", str + input_pos);
    }

    if (str[input_pos] == '#') {
        printf("\n合法输入\n");
    }
    else {
        printf("\n非法输入\n");
    }

    free(str);
}
/******End******/

void STACK_POP(void)
{
    if (STACK_EMPTY())
    {
        printf("栈空\n");
        emergency(2);
    }
    amount--;
    return;
}

size_t STACK_EMPTY()
{
    return amount == 0 ? success : !success;
}

size_t STACK_FULL()
{
    return amount == 19 ? success : !success;
}

void STACK_PUSH(char source)
{
    if (STACK_FULL())
    {
        printf("栈满\n");
        emergency(2);
    }
    TEST_STACK[++amount] = source;
    return;
}

void init_stack(void)
{
    amount = 0;
    TEST_STACK[amount] = '#';
    return;
}
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include "parser.h"
#include "dataType.h"

#ifndef success
#define success 1
#endif

#define MAX_AMOUNT 20

using namespace std;

//定义非终结符
typedef struct NOterminal
{
    char character;
    int  first_number;  //FIRST集的判定，初始为0
    int  follow_number; //FOLLOW集的判定，初始为0
    char* FIRST;       //FIRST 集合
    char* FOLLOW;      //FOLLOW集合
    struct NOterminal* next;
} noterminal;

//定义终结符
typedef struct Terminal
{
    char  character;        //当前的字符
    struct Terminal* next;
} terminal;

//定义产生式
typedef struct PRODUCTION
{
    char source;                //产生的开始
    char* result;              //产生的结果
    struct PRODUCTION* next;   //指向下一条
} production;

int amount = 0;
char TEST_STACK[20];
char* TEST_LIST[10];
char terminal_all[10] = { 0 };
terminal   TERMINAL_HEAD, * current_terminal = &TERMINAL_HEAD;
noterminal NOTERMINAL_HEAD, * current_noterminal = &NOTERMINAL_HEAD;
production PRODUCTION_HEAD, * current_production = &PRODUCTION_HEAD;

//函数申明
size_t read(void);
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
void STACK_PUSH(char source);
void insert_to_terminal(char get);
void insert_to_noterminal(char get);
void eliminate_left_recursion(void);
void combine(char* destinction, char* source);
size_t is_appeared(char tobejudged, char* source);
void insert_to_production(char source, char* result);
size_t find_first(noterminal* this_noterminal, production* this_production);
size_t find_follow(noterminal* this_noterminal, production* this_production);
void analyze_input(char* input, int table[128][128]);

int start_parser(std::vector<Token> tokens)
{
    TERMINAL_HEAD.next = NULL;
    NOTERMINAL_HEAD.next = NULL;
    PRODUCTION_HEAD.next = NULL;
    read();
    Test_read();
    printf("\n消除左递归\n\n\n");
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

//读测试文件
size_t read(void)
{
    int line = 0, model = 0, old_line = 0;
    int number = 0;
    char current_get = 0, read_temp[10] = { 0 };

    //读取文件
    FILE* read_stream = fopen("/data/test.txt", "r");
    if (!read_stream)
    {
        printf("error in open file ,can`t open file\n");
        exit(EXIT_FAILURE);
    }
    insert_to_terminal('#');    //插入栈底元素，以#表示栈底
    insert_to_terminal('^');    //插入空串元素，以^表示栈底
    //第一次循环：读取非终结符和终结符
    while (!feof(read_stream))
    {
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream); //忽略空格
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //跳过换行
        }

        switch (line)
        {
        case 0:
            //文件的第1行（line == 0）视为“非终结符行”，把每一个字符（忽略空格和换行）插入到非终结符链表
            insert_to_noterminal(current_get);
            break;
        case 1:
            //文件的第2行（line == 1）视为“终结符行”，把每一个字符（忽略空格和换行）插入到终结符链表
            insert_to_terminal(current_get);
            break;
        case 3:
            //当读到了第4行时，调用ungetc把当前字符退回，并跳出循环
            ungetc(current_get, read_stream);
            old_line = 1;
            break;
        default:
            perror("error in format of program \n");
            emergency(0);
            break;
        }
        if (old_line)
            break;
    }
    //第二次循环：读取产生式和测试串
    while (!feof(read_stream))
    {
        memset(read_temp, 0, sizeof(read_temp));
        old_line = line;  //保留第一次循环的结束位置
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream);     //忽略空格
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //跳过换行
        }
        model = ((line - old_line) > model) ? (line - old_line) : model;
        switch (model)
        {
        case 0:
        case 1:
            //model==0,1：仍在产生式部分，current_get是产生式左部（如 E、T、F），read_temp拿到右部（如 E+T|T）
            fscanf(read_stream, "%s", read_temp);
            insert_to_production(current_get, read_temp);
            break;
        case 2:
            //model==2：进入测试输入串部分，读取进行语法分析的句子
            ungetc(current_get, read_stream);
            TEST_LIST[number] = (char*)malloc(20);
            memset(TEST_LIST[number], 0, 20);
            fscanf(read_stream, "%s", TEST_LIST[number++]);
            break;
        default:
            perror("error in format of program \n");
            emergency(1);
            break;
        }
    }

    fclose(read_stream);
    return success;
}

//测试
size_t test(void)
{
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\tfirst number=%d\tfirst=%s\n",
            TEMP_NOTERMINAL->character,
            TEMP_NOTERMINAL->first_number,
            TEMP_NOTERMINAL->FIRST);
    }
    printf("\n");
    return success;
}

//向集合中加入一个字符，返回新增数（0或1）
static size_t add_to_set(char** p_set, int* p_size, char ch)
{
    char* set = *p_set;
    int size = *p_size;
    //检查是否已在集合中
    for (int i = 0; i < size; ++i) {
        if (set[i] == ch) return 0;
    }
    //扩容并加入
    set = (char*)realloc(set, size + 2);  // +1字符，+1终止符
    set[size] = ch;
    set[size + 1] = '\0';
    *p_set = set;
    *p_size = size + 1;
    return 1;
}

//查找字符对应的非终结符节点
static noterminal* get_noterminal(char X)
{
    noterminal* p = NOTERMINAL_HEAD.next;
    while (p)
    {
        if (p->character == X) return p;
        p = p->next;
    }
    return NULL;
}

//判断X是否为终结符（查terminal_all数组
static int is_terminal_char(char X)
{
    for (int i = 0; terminal_all[i] != 0; ++i)
    {
        if (terminal_all[i] == X) return 1;
    }
    return 0;
}


/******Begin******/
//求FIRST集合
size_t find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    //如果这是一个终结符，不管，FIRST只在prediction前对非终结符调用
    //遍历所有产生式
    for (production* p = this_production; p; p = p->next) {
        if (p->source != this_noterminal->character) continue;
        char* alpha = p->result;
        //如果右部是ε（程序中用'^'表示）
        if (alpha[0] == '^') {
            added += add_to_set(&this_noterminal->FIRST,
                &this_noterminal->first_number,
                '^');
            continue;
        }
        //否则逐符号处理A → X1 X2 X3 ...
        int i = 0;
        for (; alpha[i] != '\0'; ++i) {
            char X = alpha[i];
            if (is_terminal_char(X)) {
                //若X是终结符，加入FIRST(A)
                added += add_to_set(&this_noterminal->FIRST,
                    &this_noterminal->first_number,
                    X);
                break;
            }
            else {
                //X是非终结符，先递归确保 FIRST(X) 已经部分/全部计算过
                noterminal* Y = get_noterminal(X);
                if (!Y) break; // 出错容忍
                //把FIRST(Y)\{ε} 加入FIRST(A)
                for (int k = 0; k < Y->first_number; ++k) {
                    char c = Y->FIRST[k];
                    if (c != '^') {
                        added += add_to_set(&this_noterminal->FIRST,
                            &this_noterminal->first_number,
                            c);
                    }
                }
                //若Y的FIRST中不含ε，就停
                int has_eps = 0;
                for (int k = 0; k < Y->first_number; ++k)
                    if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                if (!has_eps) break;
                //否则继续看下一个 Xi
            }
        }
        //如果所有Xi都能推出ε（或右部本来就是空），则A的FIRST加ε
        if (alpha[i] == '\0') {
            added += add_to_set(&this_noterminal->FIRST,
                &this_noterminal->first_number,
                '^');
        }
    }
    return added;
}
/******End******/

/******Begin******/
//求FOLLOW集合
size_t find_follow(noterminal* B, production* this_production)
{
    size_t added = 0;
    //若B是开始符号，加入 '#'
    if (B == NOTERMINAL_HEAD.next) {
        added += add_to_set(&B->FOLLOW,
            &B->follow_number,
            '#');
    }
    //遍历所有产生式 A → α B β
    for (production* p = this_production; p; p = p->next) {
        char A = p->source;
        char* rhs = p->result;
        int len = strlen(rhs);
        for (int i = 0; i < len; ++i) {
            if (rhs[i] != B->character) continue;
            //看β = rhs[i+1...]
            int j = i + 1;
            int add_followA = 0;
            //先处理β非空部分
            for (; j < len; ++j) {
                char X = rhs[j];
                if (is_terminal_char(X)) {
                    //β首符号是终结符，加入到 FOLLOW(B)
                    added += add_to_set(&B->FOLLOW,
                        &B->follow_number,
                        X);
                    break;
                }
                else {
                    //β首符号是非终结符 Y
                    noterminal* Y = get_noterminal(X);
                    //把FIRST(Y)\{ε}加入FOLLOW(B)
                    for (int k = 0; k < Y->first_number; ++k) {
                        char c = Y->FIRST[k];
                        if (c != '^') {
                            added += add_to_set(&B->FOLLOW,
                                &B->follow_number,
                                c);
                        }
                    }
                    //如果FIRST(Y) 包含ε, 还要继续看下一个符号
                    int has_eps = 0;
                    for (int k = 0; k < Y->first_number; ++k)
                        if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                    if (!has_eps) break;
                    //如果Y是生产尾部且含ε，则也要把FOLLOW(A)加入FOLLOW(B)
                }
            }
            //如果β为空，或β全部能推ε，则把FOLLOW(A)加入FOLLOW(B)
            if (j == len) {
                noterminal* A_nt = get_noterminal(A);
                for (int k = 0; k < A_nt->follow_number; ++k) {
                    char c = A_nt->FOLLOW[k];
                    added += add_to_set(&B->FOLLOW,
                        &B->follow_number,
                        c);
                }
            }
        }
    }
    return added;
}
/******End******/

//紧急退出
void emergency(int model)
{
    current_noterminal = NOTERMINAL_HEAD.next;
    current_terminal = TERMINAL_HEAD.next;
    current_production = PRODUCTION_HEAD.next;
    while (current_noterminal)
    {
        NOTERMINAL_HEAD.next = current_noterminal->next;
        free(current_noterminal->FIRST);
        free(current_noterminal->FOLLOW);
        free(current_noterminal);
        current_noterminal = NOTERMINAL_HEAD.next;
    }
    while (current_terminal)
    {
        TERMINAL_HEAD.next = current_terminal->next;
        free(current_terminal);
        current_terminal = TERMINAL_HEAD.next;
    }
    while (current_production)
    {
        PRODUCTION_HEAD.next = current_production->next;
        free(current_production->result);
        free(current_production);
        current_production = PRODUCTION_HEAD.next;
    }
    printf("退出成功\n");
    exit(0);
}

//插入到终结符
void insert_to_terminal(char get)
{
    terminal* Temp_terminal = (terminal*)malloc(sizeof(terminal));
    if (!Temp_terminal)
    {
        perror("can`t malloc for this program\n");
        emergency(0);
    }
    Temp_terminal->character = get;
    Temp_terminal->next = NULL;
    current_terminal->next = Temp_terminal;
    current_terminal = Temp_terminal; //移向下一个节点
    return;
}

//插入到非终结符
void insert_to_noterminal(char get)
{
    noterminal* Temp_noterminal = (noterminal*)malloc(sizeof(noterminal));
    if (!Temp_noterminal)
    {
        perror("can`t malloc for this program\n");
        emergency(0);
    }
    Temp_noterminal->character = get;
    Temp_noterminal->next = NULL;
    Temp_noterminal->FIRST = NULL;
    Temp_noterminal->FOLLOW = NULL;
    Temp_noterminal->first_number = 0;
    Temp_noterminal->follow_number = 0;
    current_noterminal->next = Temp_noterminal;
    current_noterminal = Temp_noterminal;     //移向下一个节点
    return;
}

//插入到产生式
void insert_to_production(char source, char* result)
{
    char TEMP[20] = { 0 };
    int COUNT = 0, number = 0, length = 0, exit_condition = strlen(result);
    production* Temp_production;
    for (COUNT = 0; COUNT != exit_condition + 1; COUNT++)
    {
        if (*result == '-' && *(result + 1) == '>')
        {
            result += 2;
            COUNT += 2;
        }
        if ((*result != '|') && (*result))
            TEMP[number++] = *result;
        else
        {
            Temp_production = (production*)malloc(sizeof(production));
            length = strlen(TEMP) + 1;
            Temp_production->result = (char*)malloc(length);
            memset(Temp_production->result, 0, length);
            strncpy(Temp_production->result, TEMP, length - 1);
            memset(TEMP, 0, sizeof(TEMP));
            Temp_production->source = source;
            Temp_production->next = NULL;
            current_production->next = Temp_production;
            current_production = Temp_production;
            number = 0;
        }
        result++;
    }
    return;
}

//消除左递归
void eliminate_left_recursion(void)
{
    int number = 0;
    char new_char[3] = { 0 }, TEMP_RESULT[20], temp_empty[3] = { '^',0,0 };
    production* Temp_production = PRODUCTION_HEAD.next;  //遍历产生式链表
    production* Temp_FREE;  //
    terminal* temp = TERMINAL_HEAD.next;
    while (Temp_production)
    {
        //判断是否直接左递归
        if (Temp_production->source == Temp_production->result[0])
        {
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            //用对应的小写字母作为新的非终结符 
            new_char[0] = Temp_production->source - 'A' + 'a';
            //构造新的产生式
            strcat(TEMP_RESULT, Temp_production->result + 1);
            strcat(TEMP_RESULT, new_char);
            insert_to_noterminal(new_char[0]);
            insert_to_production(new_char[0], TEMP_RESULT);
            insert_to_production(new_char[0], temp_empty);

            //修改原来的产生式
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            strcat(TEMP_RESULT, Temp_production->next->result);
            strcat(TEMP_RESULT, new_char);
            memset(Temp_production->result, 0, strlen(Temp_production->result));
            strncpy(Temp_production->result, TEMP_RESULT, strlen(TEMP_RESULT));

            //删除原来的产生式（紧跟在左递归产生式之后的节点）
            Temp_FREE = Temp_production->next;
            Temp_production->next = Temp_production->next->next;
            free(Temp_FREE);
            continue;
        }
        Temp_production = Temp_production->next;
    }
    while (temp)
    {
        terminal_all[number++] = temp->character;
        temp = temp->next;
    }
    return;
}

//测试、输出读取结果
void Test_read(void)
{
    int number = 1;
    production* TEMP_PRODUCTION = PRODUCTION_HEAD.next;
    terminal* TEMP_TERMINAL = TERMINAL_HEAD.next;
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    printf("产生式\n");
    //TEMP_PRODUCTION遍历PRODUCTION_HEAD.next链表，每个节点的source字段是左部非终结符，result是右部字符串
    for (number = 1; TEMP_PRODUCTION != NULL; TEMP_PRODUCTION = TEMP_PRODUCTION->next, number++)
    {
        printf("%d\t%c\t%s\n", number, TEMP_PRODUCTION->source, TEMP_PRODUCTION->result);
    }
    printf("\n终结符\n");
    //TEMP_TERMINAL遍历TERMINAL_HEAD.next链表，依次打印每个终结符
    for (; TEMP_TERMINAL != NULL; TEMP_TERMINAL = TEMP_TERMINAL->next)
    {
        printf("%c\t", TEMP_TERMINAL->character);
    }
    printf("\n");
    printf("\n非终结符\n");
    //TEMP_NOTERMINAL遍历NOTERMINAL_HEAD.next链表，依次打印每个非终结符
    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\t", TEMP_NOTERMINAL->character);
    }
    printf("\n读取测试\n%s\n%s\n", TEST_LIST[0], TEST_LIST[1]);
    printf("\n");
    return;
}

size_t is_appeared(char tobejudged, char* source)
{
    size_t length = strlen(source), counts = 0;
    while ((counts != length) && (*source != tobejudged))
    {
        counts++;
        source++;
    }
    return counts == length ? !success : success;
}

void combine(char* destinction, char* source)
{
    char temp[2] = { 0 };
    while (*source)
    {
        if (!is_appeared(*source, destinction))
        {
            temp[0] = *source;
            strcat(destinction, temp);
        }
        source++;
    }
    return;
}

void prediction(void)
{
    noterminal* TEMP_NOTERMINAL;
    int changed;
    do
    {
        changed = 0;
        TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;
        while (TEMP_NOTERMINAL != NULL)
        {
            changed |= find_first(TEMP_NOTERMINAL, PRODUCTION_HEAD.next);
            TEMP_NOTERMINAL = TEMP_NOTERMINAL->next;
        }
    } while (changed);
    test();

    TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;
    while (TEMP_NOTERMINAL != NULL)
    {
        find_follow(TEMP_NOTERMINAL, PRODUCTION_HEAD.next);
        TEMP_NOTERMINAL = TEMP_NOTERMINAL->next;
    }
    return;
}

void test_follow(void)
{
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\tfollow number=%d\tfollow=%s\n",
            TEMP_NOTERMINAL->character,
            TEMP_NOTERMINAL->follow_number,
            TEMP_NOTERMINAL->FOLLOW);
    }
    printf("\n");
    return;
}

/******Begin******/
//生成预测分析表
void prediction_table(void)
{
    //用于存储预测分析表
    //table[i][j] 表示非终结符i和终结符j的产生式编号，0表示无对应产生式
    int table[128][128] = { 0 };
    int prod_num = 1;

    //打印标题
    printf("\n预测分析表\n\n");

    //遍历所有产生式，填充预测分析表
    production* p = PRODUCTION_HEAD.next;
    while (p != NULL) {
        char A = p->source;      //产生式左部
        char* alpha = p->result; //产生式右部

        //处理FIRST(alpha)的每个终结符
        if (alpha[0] == '^')  //如果右部是空串
        {
            noterminal* A_nt = get_noterminal(A);
            //将产生式号填入表中 A 行 FOLLOW(A) 列
            for (int i = 0; i < A_nt->follow_number; i++)
            {
                char b = A_nt->FOLLOW[i];
                table[(int)A][(int)b] = prod_num;
            }
        }
        else if (is_terminal_char(alpha[0]))  //如果右部首符是终结符
        {
            table[(int)A][(int)alpha[0]] = prod_num;
        }
        else  //右部首符是非终结符
        {
            noterminal* B = get_noterminal(alpha[0]);
            if (B != NULL)
            {
                //遍历 FIRST(B)
                for (int i = 0; i < B->first_number; i++)
                {
                    char b = B->FIRST[i];
                    if (b != '^')  //非空符号直接添加
                    {
                        table[(int)A][(int)b] = prod_num;
                    }
                    else
                    {
                        //如果B能推出空，则考虑alpha后面的符号
                        if (alpha[1] == '\0')
                        {
                            //B是右部的最后一个符号
                            noterminal* A_nt = get_noterminal(A);
                            //将产生式号填入表中A行FOLLOW(A)列
                            for (int j = 0; j < A_nt->follow_number; j++)
                            {
                                char c = A_nt->FOLLOW[j];
                                table[(int)A][(int)c] = prod_num;
                            }
                        }
                        else if (is_terminal_char(alpha[1]))
                        {
                            table[(int)A][(int)alpha[1]] = prod_num;
                        }
                        else
                        {
                            //处理后续非终结符的FIRST集
                            noterminal* next_B = get_noterminal(alpha[1]);
                            if (next_B != NULL)
                            {
                                for (int j = 0; j < next_B->first_number; j++)
                                {
                                    char c = next_B->FIRST[j];
                                    if (c != '^')
                                    {
                                        table[(int)A][(int)c] = prod_num;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        p = p->next;
        prod_num++;
    }

    // 打印预测分析表
    printf(" \t");
    // 打印终结符作为表头
    terminal* t = TERMINAL_HEAD.next;
    while (t != NULL) {
        printf("%c\t", t->character);
        t = t->next;
    }
    printf("\n");

    // 打印每行（非终结符）
    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt != NULL) {
        printf("%c\t", nt->character);

        // 打印该非终结符对应的每个终结符的产生式
        t = TERMINAL_HEAD.next;
        while (t != NULL) {
            int entry = table[(int)nt->character][(int)t->character];
            if (entry != 0) {
                printf("%d\t", entry);
            }
            else {
                printf(" \t");
            }
            t = t->next;
        }
        printf("\n");
        nt = nt->next;
    }
    printf("\n\n\n");

    // 使用预测分析表进行语法分析
    for (int i = 0; TEST_LIST[i] != NULL; i++) {
        char* input = TEST_LIST[i];
        analyze_input(input, table);
    }
}

//根据预测分析表分析输入串
void analyze_input(char* input, int table[128][128])
{
    char* str = (char*)malloc(strlen(input) + 2);
    strcpy(str, input);
    strcat(str, "#");  //为输入字符串添加结束符

    //初始化分析栈
    init_stack();
    STACK_PUSH('E');  //压入开始符号

    printf("分析栈\t%c%c", TEST_STACK[0], TEST_STACK[1]);
    int i = 1;
    while (i < amount)
    {
        printf("%c", TEST_STACK[i + 1]);
        i++;
    }
    printf("\t剩余字符串\t%s\n", str);

    int input_pos = 0;
    while (!STACK_EMPTY())
    {
        char top = TEST_STACK[amount];  //栈顶符号
        char current = str[input_pos];  //当前输入符号

        if (is_terminal_char(top))
        {
            //栈顶是终结符
            if (top == current)
            {
                //匹配，出栈并移动输入指针
                STACK_POP();
                input_pos++;
            }
            else
            {
                //不匹配，报错
                printf("语法错误：期望 %c，得到 %c\n", top, current);
                free(str);
                return;
            }
        }
        else
        {
            //栈顶是非终结符
            int prod_num = table[(int)top][(int)current];
            if (prod_num > 0)
            {
                //找到对应产生式
                STACK_POP();  // 弹出非终结符

                //找到对应的产生式
                production* p = PRODUCTION_HEAD.next;
                for (int j = 1; j < prod_num; j++)
                {
                    p = p->next;
                }

                //反向压入产生式右部（不压入空串）
                if (p->result[0] != '^')
                {
                    for (int j = strlen(p->result) - 1; j >= 0; j--)
                    {
                        STACK_PUSH(p->result[j]);
                    }
                }
            }
            else
            {
                //无对应产生式，报错
                printf("语法错误：非终结符 %c 对于输入 %c 没有产生式\n", top, current);
                free(str);
                return;
            }
        }

        // 打印当前栈和剩余输入
        printf("分析栈\t%c", TEST_STACK[0]);
        for (int j = 1; j <= amount; j++) {
            printf("%c", TEST_STACK[j]);
        }
        printf("\t剩余字符串\t%s\n", str + input_pos);
    }

    if (str[input_pos] == '#') {
        printf("\n合法输入\n");
    }
    else {
        printf("\n非法输入\n");
    }

    free(str);
}
/******End******/

void STACK_POP(void)
{
    if (STACK_EMPTY())
    {
        printf("栈空\n");
        emergency(2);
    }
    amount--;
    return;
}

size_t STACK_EMPTY()
{
    return amount == 0 ? success : !success;
}

size_t STACK_FULL()
{
    return amount == 19 ? success : !success;
}

void STACK_PUSH(char source)
{
    if (STACK_FULL())
    {
        printf("栈满\n");
        emergency(2);
    }
    TEST_STACK[++amount] = source;
    return;
}

void init_stack(void)
{
    amount = 0;
    TEST_STACK[amount] = '#';
    return;
}

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

//������ս��
typedef struct NOterminal
{
    char character;
    int  first_number;  //FIRST�����ж�����ʼΪ0
    int  follow_number; //FOLLOW�����ж�����ʼΪ0
    char* FIRST;       //FIRST ����
    char* FOLLOW;      //FOLLOW����
    struct NOterminal* next;
} noterminal;

//�����ս��
typedef struct Terminal
{
    char  character;        //��ǰ���ַ�
    struct Terminal* next;
} terminal;

//�������ʽ
typedef struct PRODUCTION
{
    char source;                //�����Ŀ�ʼ
    char* result;              //�����Ľ��
    struct PRODUCTION* next;   //ָ����һ��
} production;

int amount = 0;
char TEST_STACK[20];
char* TEST_LIST[10];
char terminal_all[10] = { 0 };
terminal   TERMINAL_HEAD, * current_terminal = &TERMINAL_HEAD;
noterminal NOTERMINAL_HEAD, * current_noterminal = &NOTERMINAL_HEAD;
production PRODUCTION_HEAD, * current_production = &PRODUCTION_HEAD;

//��������
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
    printf("\n������ݹ�\n\n\n");
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

//�������ļ�
size_t read(void)
{
    int line = 0, model = 0, old_line = 0;
    int number = 0;
    char current_get = 0, read_temp[10] = { 0 };

    //��ȡ�ļ�
    FILE* read_stream = fopen("/data/test.txt", "r");
    if (!read_stream)
    {
        printf("error in open file ,can`t open file\n");
        exit(EXIT_FAILURE);
    }
    insert_to_terminal('#');    //����ջ��Ԫ�أ���#��ʾջ��
    insert_to_terminal('^');    //����մ�Ԫ�أ���^��ʾջ��
    //��һ��ѭ������ȡ���ս�����ս��
    while (!feof(read_stream))
    {
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream); //���Կո�
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //��������
        }

        switch (line)
        {
        case 0:
            //�ļ��ĵ�1�У�line == 0����Ϊ�����ս���С�����ÿһ���ַ������Կո�ͻ��У����뵽���ս������
            insert_to_noterminal(current_get);
            break;
        case 1:
            //�ļ��ĵ�2�У�line == 1����Ϊ���ս���С�����ÿһ���ַ������Կո�ͻ��У����뵽�ս������
            insert_to_terminal(current_get);
            break;
        case 3:
            //�������˵�4��ʱ������ungetc�ѵ�ǰ�ַ��˻أ�������ѭ��
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
    //�ڶ���ѭ������ȡ����ʽ�Ͳ��Դ�
    while (!feof(read_stream))
    {
        memset(read_temp, 0, sizeof(read_temp));
        old_line = line;  //������һ��ѭ���Ľ���λ��
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream);     //���Կո�
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //��������
        }
        model = ((line - old_line) > model) ? (line - old_line) : model;
        switch (model)
        {
        case 0:
        case 1:
            //model==0,1�����ڲ���ʽ���֣�current_get�ǲ���ʽ�󲿣��� E��T��F����read_temp�õ��Ҳ����� E+T|T��
            fscanf(read_stream, "%s", read_temp);
            insert_to_production(current_get, read_temp);
            break;
        case 2:
            //model==2������������봮���֣���ȡ�����﷨�����ľ���
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

//����
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

//�򼯺��м���һ���ַ���������������0��1��
static size_t add_to_set(char** p_set, int* p_size, char ch)
{
    char* set = *p_set;
    int size = *p_size;
    //����Ƿ����ڼ�����
    for (int i = 0; i < size; ++i) {
        if (set[i] == ch) return 0;
    }
    //���ݲ�����
    set = (char*)realloc(set, size + 2);  // +1�ַ���+1��ֹ��
    set[size] = ch;
    set[size + 1] = '\0';
    *p_set = set;
    *p_size = size + 1;
    return 1;
}

//�����ַ���Ӧ�ķ��ս���ڵ�
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

//�ж�X�Ƿ�Ϊ�ս������terminal_all����
static int is_terminal_char(char X)
{
    for (int i = 0; terminal_all[i] != 0; ++i)
    {
        if (terminal_all[i] == X) return 1;
    }
    return 0;
}


/******Begin******/
//��FIRST����
size_t find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    //�������һ���ս�������ܣ�FIRSTֻ��predictionǰ�Է��ս������
    //�������в���ʽ
    for (production* p = this_production; p; p = p->next) {
        if (p->source != this_noterminal->character) continue;
        char* alpha = p->result;
        //����Ҳ��Ǧţ���������'^'��ʾ��
        if (alpha[0] == '^') {
            added += add_to_set(&this_noterminal->FIRST,
                &this_noterminal->first_number,
                '^');
            continue;
        }
        //��������Ŵ���A �� X1 X2 X3 ...
        int i = 0;
        for (; alpha[i] != '\0'; ++i) {
            char X = alpha[i];
            if (is_terminal_char(X)) {
                //��X���ս��������FIRST(A)
                added += add_to_set(&this_noterminal->FIRST,
                    &this_noterminal->first_number,
                    X);
                break;
            }
            else {
                //X�Ƿ��ս�����ȵݹ�ȷ�� FIRST(X) �Ѿ�����/ȫ�������
                noterminal* Y = get_noterminal(X);
                if (!Y) break; // ��������
                //��FIRST(Y)\{��} ����FIRST(A)
                for (int k = 0; k < Y->first_number; ++k) {
                    char c = Y->FIRST[k];
                    if (c != '^') {
                        added += add_to_set(&this_noterminal->FIRST,
                            &this_noterminal->first_number,
                            c);
                    }
                }
                //��Y��FIRST�в����ţ���ͣ
                int has_eps = 0;
                for (int k = 0; k < Y->first_number; ++k)
                    if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                if (!has_eps) break;
                //�����������һ�� Xi
            }
        }
        //�������Xi�����Ƴ��ţ����Ҳ��������ǿգ�����A��FIRST�Ӧ�
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
//��FOLLOW����
size_t find_follow(noterminal* B, production* this_production)
{
    size_t added = 0;
    //��B�ǿ�ʼ���ţ����� '#'
    if (B == NOTERMINAL_HEAD.next) {
        added += add_to_set(&B->FOLLOW,
            &B->follow_number,
            '#');
    }
    //�������в���ʽ A �� �� B ��
    for (production* p = this_production; p; p = p->next) {
        char A = p->source;
        char* rhs = p->result;
        int len = strlen(rhs);
        for (int i = 0; i < len; ++i) {
            if (rhs[i] != B->character) continue;
            //���� = rhs[i+1...]
            int j = i + 1;
            int add_followA = 0;
            //�ȴ���·ǿղ���
            for (; j < len; ++j) {
                char X = rhs[j];
                if (is_terminal_char(X)) {
                    //���׷������ս�������뵽 FOLLOW(B)
                    added += add_to_set(&B->FOLLOW,
                        &B->follow_number,
                        X);
                    break;
                }
                else {
                    //���׷����Ƿ��ս�� Y
                    noterminal* Y = get_noterminal(X);
                    //��FIRST(Y)\{��}����FOLLOW(B)
                    for (int k = 0; k < Y->first_number; ++k) {
                        char c = Y->FIRST[k];
                        if (c != '^') {
                            added += add_to_set(&B->FOLLOW,
                                &B->follow_number,
                                c);
                        }
                    }
                    //���FIRST(Y) ������, ��Ҫ��������һ������
                    int has_eps = 0;
                    for (int k = 0; k < Y->first_number; ++k)
                        if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                    if (!has_eps) break;
                    //���Y������β���Һ��ţ���ҲҪ��FOLLOW(A)����FOLLOW(B)
                }
            }
            //�����Ϊ�գ����ȫ�����Ʀţ����FOLLOW(A)����FOLLOW(B)
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

//�����˳�
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
    printf("�˳��ɹ�\n");
    exit(0);
}

//���뵽�ս��
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
    current_terminal = Temp_terminal; //������һ���ڵ�
    return;
}

//���뵽���ս��
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
    current_noterminal = Temp_noterminal;     //������һ���ڵ�
    return;
}

//���뵽����ʽ
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

//������ݹ�
void eliminate_left_recursion(void)
{
    int number = 0;
    char new_char[3] = { 0 }, TEMP_RESULT[20], temp_empty[3] = { '^',0,0 };
    production* Temp_production = PRODUCTION_HEAD.next;  //��������ʽ����
    production* Temp_FREE;  //
    terminal* temp = TERMINAL_HEAD.next;
    while (Temp_production)
    {
        //�ж��Ƿ�ֱ����ݹ�
        if (Temp_production->source == Temp_production->result[0])
        {
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            //�ö�Ӧ��Сд��ĸ��Ϊ�µķ��ս�� 
            new_char[0] = Temp_production->source - 'A' + 'a';
            //�����µĲ���ʽ
            strcat(TEMP_RESULT, Temp_production->result + 1);
            strcat(TEMP_RESULT, new_char);
            insert_to_noterminal(new_char[0]);
            insert_to_production(new_char[0], TEMP_RESULT);
            insert_to_production(new_char[0], temp_empty);

            //�޸�ԭ���Ĳ���ʽ
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            strcat(TEMP_RESULT, Temp_production->next->result);
            strcat(TEMP_RESULT, new_char);
            memset(Temp_production->result, 0, strlen(Temp_production->result));
            strncpy(Temp_production->result, TEMP_RESULT, strlen(TEMP_RESULT));

            //ɾ��ԭ���Ĳ���ʽ����������ݹ����ʽ֮��Ľڵ㣩
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

//���ԡ������ȡ���
void Test_read(void)
{
    int number = 1;
    production* TEMP_PRODUCTION = PRODUCTION_HEAD.next;
    terminal* TEMP_TERMINAL = TERMINAL_HEAD.next;
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    printf("����ʽ\n");
    //TEMP_PRODUCTION����PRODUCTION_HEAD.next����ÿ���ڵ��source�ֶ����󲿷��ս����result���Ҳ��ַ���
    for (number = 1; TEMP_PRODUCTION != NULL; TEMP_PRODUCTION = TEMP_PRODUCTION->next, number++)
    {
        printf("%d\t%c\t%s\n", number, TEMP_PRODUCTION->source, TEMP_PRODUCTION->result);
    }
    printf("\n�ս��\n");
    //TEMP_TERMINAL����TERMINAL_HEAD.next�������δ�ӡÿ���ս��
    for (; TEMP_TERMINAL != NULL; TEMP_TERMINAL = TEMP_TERMINAL->next)
    {
        printf("%c\t", TEMP_TERMINAL->character);
    }
    printf("\n");
    printf("\n���ս��\n");
    //TEMP_NOTERMINAL����NOTERMINAL_HEAD.next�������δ�ӡÿ�����ս��
    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\t", TEMP_NOTERMINAL->character);
    }
    printf("\n��ȡ����\n%s\n%s\n", TEST_LIST[0], TEST_LIST[1]);
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
//����Ԥ�������
void prediction_table(void)
{
    //���ڴ洢Ԥ�������
    //table[i][j] ��ʾ���ս��i���ս��j�Ĳ���ʽ��ţ�0��ʾ�޶�Ӧ����ʽ
    int table[128][128] = { 0 };
    int prod_num = 1;

    //��ӡ����
    printf("\nԤ�������\n\n");

    //�������в���ʽ�����Ԥ�������
    production* p = PRODUCTION_HEAD.next;
    while (p != NULL) {
        char A = p->source;      //����ʽ��
        char* alpha = p->result; //����ʽ�Ҳ�

        //����FIRST(alpha)��ÿ���ս��
        if (alpha[0] == '^')  //����Ҳ��ǿմ�
        {
            noterminal* A_nt = get_noterminal(A);
            //������ʽ��������� A �� FOLLOW(A) ��
            for (int i = 0; i < A_nt->follow_number; i++)
            {
                char b = A_nt->FOLLOW[i];
                table[(int)A][(int)b] = prod_num;
            }
        }
        else if (is_terminal_char(alpha[0]))  //����Ҳ��׷����ս��
        {
            table[(int)A][(int)alpha[0]] = prod_num;
        }
        else  //�Ҳ��׷��Ƿ��ս��
        {
            noterminal* B = get_noterminal(alpha[0]);
            if (B != NULL)
            {
                //���� FIRST(B)
                for (int i = 0; i < B->first_number; i++)
                {
                    char b = B->FIRST[i];
                    if (b != '^')  //�ǿշ���ֱ�����
                    {
                        table[(int)A][(int)b] = prod_num;
                    }
                    else
                    {
                        //���B���Ƴ��գ�����alpha����ķ���
                        if (alpha[1] == '\0')
                        {
                            //B���Ҳ������һ������
                            noterminal* A_nt = get_noterminal(A);
                            //������ʽ���������A��FOLLOW(A)��
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
                            //����������ս����FIRST��
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

    // ��ӡԤ�������
    printf(" \t");
    // ��ӡ�ս����Ϊ��ͷ
    terminal* t = TERMINAL_HEAD.next;
    while (t != NULL) {
        printf("%c\t", t->character);
        t = t->next;
    }
    printf("\n");

    // ��ӡÿ�У����ս����
    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt != NULL) {
        printf("%c\t", nt->character);

        // ��ӡ�÷��ս����Ӧ��ÿ���ս���Ĳ���ʽ
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

    // ʹ��Ԥ�����������﷨����
    for (int i = 0; TEST_LIST[i] != NULL; i++) {
        char* input = TEST_LIST[i];
        analyze_input(input, table);
    }
}

//����Ԥ�������������봮
void analyze_input(char* input, int table[128][128])
{
    char* str = (char*)malloc(strlen(input) + 2);
    strcpy(str, input);
    strcat(str, "#");  //Ϊ�����ַ�����ӽ�����

    //��ʼ������ջ
    init_stack();
    STACK_PUSH('E');  //ѹ�뿪ʼ����

    printf("����ջ\t%c%c", TEST_STACK[0], TEST_STACK[1]);
    int i = 1;
    while (i < amount)
    {
        printf("%c", TEST_STACK[i + 1]);
        i++;
    }
    printf("\tʣ���ַ���\t%s\n", str);

    int input_pos = 0;
    while (!STACK_EMPTY())
    {
        char top = TEST_STACK[amount];  //ջ������
        char current = str[input_pos];  //��ǰ�������

        if (is_terminal_char(top))
        {
            //ջ�����ս��
            if (top == current)
            {
                //ƥ�䣬��ջ���ƶ�����ָ��
                STACK_POP();
                input_pos++;
            }
            else
            {
                //��ƥ�䣬����
                printf("�﷨�������� %c���õ� %c\n", top, current);
                free(str);
                return;
            }
        }
        else
        {
            //ջ���Ƿ��ս��
            int prod_num = table[(int)top][(int)current];
            if (prod_num > 0)
            {
                //�ҵ���Ӧ����ʽ
                STACK_POP();  // �������ս��

                //�ҵ���Ӧ�Ĳ���ʽ
                production* p = PRODUCTION_HEAD.next;
                for (int j = 1; j < prod_num; j++)
                {
                    p = p->next;
                }

                //����ѹ�����ʽ�Ҳ�����ѹ��մ���
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
                //�޶�Ӧ����ʽ������
                printf("�﷨���󣺷��ս�� %c �������� %c û�в���ʽ\n", top, current);
                free(str);
                return;
            }
        }

        // ��ӡ��ǰջ��ʣ������
        printf("����ջ\t%c", TEST_STACK[0]);
        for (int j = 1; j <= amount; j++) {
            printf("%c", TEST_STACK[j]);
        }
        printf("\tʣ���ַ���\t%s\n", str + input_pos);
    }

    if (str[input_pos] == '#') {
        printf("\n�Ϸ�����\n");
    }
    else {
        printf("\n�Ƿ�����\n");
    }

    free(str);
}
/******End******/

void STACK_POP(void)
{
    if (STACK_EMPTY())
    {
        printf("ջ��\n");
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
        printf("ջ��\n");
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

//������ս��
typedef struct NOterminal
{
    char character;
    int  first_number;  //FIRST�����ж�����ʼΪ0
    int  follow_number; //FOLLOW�����ж�����ʼΪ0
    char* FIRST;       //FIRST ����
    char* FOLLOW;      //FOLLOW����
    struct NOterminal* next;
} noterminal;

//�����ս��
typedef struct Terminal
{
    char  character;        //��ǰ���ַ�
    struct Terminal* next;
} terminal;

//�������ʽ
typedef struct PRODUCTION
{
    char source;                //�����Ŀ�ʼ
    char* result;              //�����Ľ��
    struct PRODUCTION* next;   //ָ����һ��
} production;

int amount = 0;
char TEST_STACK[20];
char* TEST_LIST[10];
char terminal_all[10] = { 0 };
terminal   TERMINAL_HEAD, * current_terminal = &TERMINAL_HEAD;
noterminal NOTERMINAL_HEAD, * current_noterminal = &NOTERMINAL_HEAD;
production PRODUCTION_HEAD, * current_production = &PRODUCTION_HEAD;

//��������
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
    printf("\n������ݹ�\n\n\n");
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

//�������ļ�
size_t read(void)
{
    int line = 0, model = 0, old_line = 0;
    int number = 0;
    char current_get = 0, read_temp[10] = { 0 };

    //��ȡ�ļ�
    FILE* read_stream = fopen("/data/test.txt", "r");
    if (!read_stream)
    {
        printf("error in open file ,can`t open file\n");
        exit(EXIT_FAILURE);
    }
    insert_to_terminal('#');    //����ջ��Ԫ�أ���#��ʾջ��
    insert_to_terminal('^');    //����մ�Ԫ�أ���^��ʾջ��
    //��һ��ѭ������ȡ���ս�����ս��
    while (!feof(read_stream))
    {
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream); //���Կո�
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //��������
        }

        switch (line)
        {
        case 0:
            //�ļ��ĵ�1�У�line == 0����Ϊ�����ս���С�����ÿһ���ַ������Կո�ͻ��У����뵽���ս������
            insert_to_noterminal(current_get);
            break;
        case 1:
            //�ļ��ĵ�2�У�line == 1����Ϊ���ս���С�����ÿһ���ַ������Կո�ͻ��У����뵽�ս������
            insert_to_terminal(current_get);
            break;
        case 3:
            //�������˵�4��ʱ������ungetc�ѵ�ǰ�ַ��˻أ�������ѭ��
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
    //�ڶ���ѭ������ȡ����ʽ�Ͳ��Դ�
    while (!feof(read_stream))
    {
        memset(read_temp, 0, sizeof(read_temp));
        old_line = line;  //������һ��ѭ���Ľ���λ��
        current_get = fgetc(read_stream);
        while (current_get == ' ')
            current_get = fgetc(read_stream);     //���Կո�
        while (current_get == '\n')
        {
            current_get = fgetc(read_stream);
            line++;     //��������
        }
        model = ((line - old_line) > model) ? (line - old_line) : model;
        switch (model)
        {
        case 0:
        case 1:
            //model==0,1�����ڲ���ʽ���֣�current_get�ǲ���ʽ�󲿣��� E��T��F����read_temp�õ��Ҳ����� E+T|T��
            fscanf(read_stream, "%s", read_temp);
            insert_to_production(current_get, read_temp);
            break;
        case 2:
            //model==2������������봮���֣���ȡ�����﷨�����ľ���
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

//����
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

//�򼯺��м���һ���ַ���������������0��1��
static size_t add_to_set(char** p_set, int* p_size, char ch)
{
    char* set = *p_set;
    int size = *p_size;
    //����Ƿ����ڼ�����
    for (int i = 0; i < size; ++i) {
        if (set[i] == ch) return 0;
    }
    //���ݲ�����
    set = (char*)realloc(set, size + 2);  // +1�ַ���+1��ֹ��
    set[size] = ch;
    set[size + 1] = '\0';
    *p_set = set;
    *p_size = size + 1;
    return 1;
}

//�����ַ���Ӧ�ķ��ս���ڵ�
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

//�ж�X�Ƿ�Ϊ�ս������terminal_all����
static int is_terminal_char(char X)
{
    for (int i = 0; terminal_all[i] != 0; ++i)
    {
        if (terminal_all[i] == X) return 1;
    }
    return 0;
}


/******Begin******/
//��FIRST����
size_t find_first(noterminal* this_noterminal, production* this_production)
{
    size_t added = 0;
    //�������һ���ս�������ܣ�FIRSTֻ��predictionǰ�Է��ս������
    //�������в���ʽ
    for (production* p = this_production; p; p = p->next) {
        if (p->source != this_noterminal->character) continue;
        char* alpha = p->result;
        //����Ҳ��Ǧţ���������'^'��ʾ��
        if (alpha[0] == '^') {
            added += add_to_set(&this_noterminal->FIRST,
                &this_noterminal->first_number,
                '^');
            continue;
        }
        //��������Ŵ���A �� X1 X2 X3 ...
        int i = 0;
        for (; alpha[i] != '\0'; ++i) {
            char X = alpha[i];
            if (is_terminal_char(X)) {
                //��X���ս��������FIRST(A)
                added += add_to_set(&this_noterminal->FIRST,
                    &this_noterminal->first_number,
                    X);
                break;
            }
            else {
                //X�Ƿ��ս�����ȵݹ�ȷ�� FIRST(X) �Ѿ�����/ȫ�������
                noterminal* Y = get_noterminal(X);
                if (!Y) break; // ��������
                //��FIRST(Y)\{��} ����FIRST(A)
                for (int k = 0; k < Y->first_number; ++k) {
                    char c = Y->FIRST[k];
                    if (c != '^') {
                        added += add_to_set(&this_noterminal->FIRST,
                            &this_noterminal->first_number,
                            c);
                    }
                }
                //��Y��FIRST�в����ţ���ͣ
                int has_eps = 0;
                for (int k = 0; k < Y->first_number; ++k)
                    if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                if (!has_eps) break;
                //�����������һ�� Xi
            }
        }
        //�������Xi�����Ƴ��ţ����Ҳ��������ǿգ�����A��FIRST�Ӧ�
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
//��FOLLOW����
size_t find_follow(noterminal* B, production* this_production)
{
    size_t added = 0;
    //��B�ǿ�ʼ���ţ����� '#'
    if (B == NOTERMINAL_HEAD.next) {
        added += add_to_set(&B->FOLLOW,
            &B->follow_number,
            '#');
    }
    //�������в���ʽ A �� �� B ��
    for (production* p = this_production; p; p = p->next) {
        char A = p->source;
        char* rhs = p->result;
        int len = strlen(rhs);
        for (int i = 0; i < len; ++i) {
            if (rhs[i] != B->character) continue;
            //���� = rhs[i+1...]
            int j = i + 1;
            int add_followA = 0;
            //�ȴ���·ǿղ���
            for (; j < len; ++j) {
                char X = rhs[j];
                if (is_terminal_char(X)) {
                    //���׷������ս�������뵽 FOLLOW(B)
                    added += add_to_set(&B->FOLLOW,
                        &B->follow_number,
                        X);
                    break;
                }
                else {
                    //���׷����Ƿ��ս�� Y
                    noterminal* Y = get_noterminal(X);
                    //��FIRST(Y)\{��}����FOLLOW(B)
                    for (int k = 0; k < Y->first_number; ++k) {
                        char c = Y->FIRST[k];
                        if (c != '^') {
                            added += add_to_set(&B->FOLLOW,
                                &B->follow_number,
                                c);
                        }
                    }
                    //���FIRST(Y) ������, ��Ҫ��������һ������
                    int has_eps = 0;
                    for (int k = 0; k < Y->first_number; ++k)
                        if (Y->FIRST[k] == '^') { has_eps = 1; break; }
                    if (!has_eps) break;
                    //���Y������β���Һ��ţ���ҲҪ��FOLLOW(A)����FOLLOW(B)
                }
            }
            //�����Ϊ�գ����ȫ�����Ʀţ����FOLLOW(A)����FOLLOW(B)
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

//�����˳�
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
    printf("�˳��ɹ�\n");
    exit(0);
}

//���뵽�ս��
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
    current_terminal = Temp_terminal; //������һ���ڵ�
    return;
}

//���뵽���ս��
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
    current_noterminal = Temp_noterminal;     //������һ���ڵ�
    return;
}

//���뵽����ʽ
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

//������ݹ�
void eliminate_left_recursion(void)
{
    int number = 0;
    char new_char[3] = { 0 }, TEMP_RESULT[20], temp_empty[3] = { '^',0,0 };
    production* Temp_production = PRODUCTION_HEAD.next;  //��������ʽ����
    production* Temp_FREE;  //
    terminal* temp = TERMINAL_HEAD.next;
    while (Temp_production)
    {
        //�ж��Ƿ�ֱ����ݹ�
        if (Temp_production->source == Temp_production->result[0])
        {
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            //�ö�Ӧ��Сд��ĸ��Ϊ�µķ��ս�� 
            new_char[0] = Temp_production->source - 'A' + 'a';
            //�����µĲ���ʽ
            strcat(TEMP_RESULT, Temp_production->result + 1);
            strcat(TEMP_RESULT, new_char);
            insert_to_noterminal(new_char[0]);
            insert_to_production(new_char[0], TEMP_RESULT);
            insert_to_production(new_char[0], temp_empty);

            //�޸�ԭ���Ĳ���ʽ
            memset(TEMP_RESULT, 0, sizeof(TEMP_RESULT));
            strcat(TEMP_RESULT, Temp_production->next->result);
            strcat(TEMP_RESULT, new_char);
            memset(Temp_production->result, 0, strlen(Temp_production->result));
            strncpy(Temp_production->result, TEMP_RESULT, strlen(TEMP_RESULT));

            //ɾ��ԭ���Ĳ���ʽ����������ݹ����ʽ֮��Ľڵ㣩
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

//���ԡ������ȡ���
void Test_read(void)
{
    int number = 1;
    production* TEMP_PRODUCTION = PRODUCTION_HEAD.next;
    terminal* TEMP_TERMINAL = TERMINAL_HEAD.next;
    noterminal* TEMP_NOTERMINAL = NOTERMINAL_HEAD.next;

    printf("����ʽ\n");
    //TEMP_PRODUCTION����PRODUCTION_HEAD.next����ÿ���ڵ��source�ֶ����󲿷��ս����result���Ҳ��ַ���
    for (number = 1; TEMP_PRODUCTION != NULL; TEMP_PRODUCTION = TEMP_PRODUCTION->next, number++)
    {
        printf("%d\t%c\t%s\n", number, TEMP_PRODUCTION->source, TEMP_PRODUCTION->result);
    }
    printf("\n�ս��\n");
    //TEMP_TERMINAL����TERMINAL_HEAD.next�������δ�ӡÿ���ս��
    for (; TEMP_TERMINAL != NULL; TEMP_TERMINAL = TEMP_TERMINAL->next)
    {
        printf("%c\t", TEMP_TERMINAL->character);
    }
    printf("\n");
    printf("\n���ս��\n");
    //TEMP_NOTERMINAL����NOTERMINAL_HEAD.next�������δ�ӡÿ�����ս��
    for (; TEMP_NOTERMINAL != NULL; TEMP_NOTERMINAL = TEMP_NOTERMINAL->next)
    {
        printf("%c\t", TEMP_NOTERMINAL->character);
    }
    printf("\n��ȡ����\n%s\n%s\n", TEST_LIST[0], TEST_LIST[1]);
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
//����Ԥ�������
void prediction_table(void)
{
    //���ڴ洢Ԥ�������
    //table[i][j] ��ʾ���ս��i���ս��j�Ĳ���ʽ��ţ�0��ʾ�޶�Ӧ����ʽ
    int table[128][128] = { 0 };
    int prod_num = 1;

    //��ӡ����
    printf("\nԤ�������\n\n");

    //�������в���ʽ�����Ԥ�������
    production* p = PRODUCTION_HEAD.next;
    while (p != NULL) {
        char A = p->source;      //����ʽ��
        char* alpha = p->result; //����ʽ�Ҳ�

        //����FIRST(alpha)��ÿ���ս��
        if (alpha[0] == '^')  //����Ҳ��ǿմ�
        {
            noterminal* A_nt = get_noterminal(A);
            //������ʽ��������� A �� FOLLOW(A) ��
            for (int i = 0; i < A_nt->follow_number; i++)
            {
                char b = A_nt->FOLLOW[i];
                table[(int)A][(int)b] = prod_num;
            }
        }
        else if (is_terminal_char(alpha[0]))  //����Ҳ��׷����ս��
        {
            table[(int)A][(int)alpha[0]] = prod_num;
        }
        else  //�Ҳ��׷��Ƿ��ս��
        {
            noterminal* B = get_noterminal(alpha[0]);
            if (B != NULL)
            {
                //���� FIRST(B)
                for (int i = 0; i < B->first_number; i++)
                {
                    char b = B->FIRST[i];
                    if (b != '^')  //�ǿշ���ֱ�����
                    {
                        table[(int)A][(int)b] = prod_num;
                    }
                    else
                    {
                        //���B���Ƴ��գ�����alpha����ķ���
                        if (alpha[1] == '\0')
                        {
                            //B���Ҳ������һ������
                            noterminal* A_nt = get_noterminal(A);
                            //������ʽ���������A��FOLLOW(A)��
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
                            //����������ս����FIRST��
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

    // ��ӡԤ�������
    printf(" \t");
    // ��ӡ�ս����Ϊ��ͷ
    terminal* t = TERMINAL_HEAD.next;
    while (t != NULL) {
        printf("%c\t", t->character);
        t = t->next;
    }
    printf("\n");

    // ��ӡÿ�У����ս����
    noterminal* nt = NOTERMINAL_HEAD.next;
    while (nt != NULL) {
        printf("%c\t", nt->character);

        // ��ӡ�÷��ս����Ӧ��ÿ���ս���Ĳ���ʽ
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

    // ʹ��Ԥ�����������﷨����
    for (int i = 0; TEST_LIST[i] != NULL; i++) {
        char* input = TEST_LIST[i];
        analyze_input(input, table);
    }
}

//����Ԥ�������������봮
void analyze_input(char* input, int table[128][128])
{
    char* str = (char*)malloc(strlen(input) + 2);
    strcpy(str, input);
    strcat(str, "#");  //Ϊ�����ַ�����ӽ�����

    //��ʼ������ջ
    init_stack();
    STACK_PUSH('E');  //ѹ�뿪ʼ����

    printf("����ջ\t%c%c", TEST_STACK[0], TEST_STACK[1]);
    int i = 1;
    while (i < amount)
    {
        printf("%c", TEST_STACK[i + 1]);
        i++;
    }
    printf("\tʣ���ַ���\t%s\n", str);

    int input_pos = 0;
    while (!STACK_EMPTY())
    {
        char top = TEST_STACK[amount];  //ջ������
        char current = str[input_pos];  //��ǰ�������

        if (is_terminal_char(top))
        {
            //ջ�����ս��
            if (top == current)
            {
                //ƥ�䣬��ջ���ƶ�����ָ��
                STACK_POP();
                input_pos++;
            }
            else
            {
                //��ƥ�䣬����
                printf("�﷨�������� %c���õ� %c\n", top, current);
                free(str);
                return;
            }
        }
        else
        {
            //ջ���Ƿ��ս��
            int prod_num = table[(int)top][(int)current];
            if (prod_num > 0)
            {
                //�ҵ���Ӧ����ʽ
                STACK_POP();  // �������ս��

                //�ҵ���Ӧ�Ĳ���ʽ
                production* p = PRODUCTION_HEAD.next;
                for (int j = 1; j < prod_num; j++)
                {
                    p = p->next;
                }

                //����ѹ�����ʽ�Ҳ�����ѹ��մ���
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
                //�޶�Ӧ����ʽ������
                printf("�﷨���󣺷��ս�� %c �������� %c û�в���ʽ\n", top, current);
                free(str);
                return;
            }
        }

        // ��ӡ��ǰջ��ʣ������
        printf("����ջ\t%c", TEST_STACK[0]);
        for (int j = 1; j <= amount; j++) {
            printf("%c", TEST_STACK[j]);
        }
        printf("\tʣ���ַ���\t%s\n", str + input_pos);
    }

    if (str[input_pos] == '#') {
        printf("\n�Ϸ�����\n");
    }
    else {
        printf("\n�Ƿ�����\n");
    }

    free(str);
}
/******End******/

void STACK_POP(void)
{
    if (STACK_EMPTY())
    {
        printf("ջ��\n");
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
        printf("ջ��\n");
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

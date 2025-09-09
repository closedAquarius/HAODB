#include <iostream>
#include <vector>
#include <unordered_set>
#include <cctype>
#include <iomanip>
#include "dataType.h"
using namespace std;

// �ʷ�������
class Lexer {
private:
    string input;
    int pos;
    int line;
    int column;
    vector<Token> tokens;

    unordered_set<string> keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES",
        "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "DROP",
        "ALTER", "ADD", "PRIMARY", "KEY", "NOT", "NULL",
        "AND", "OR", "AS", "INT", "VARCHAR", "CHAR", "FLOAT", "DOUBLE"
    };

    unordered_set<char> operators = { '+', '-', '*', '/', '%', '=', '<', '>' };
    unordered_set<char> delimiters = { ',', ';', '(', ')', '.' };

public:
    Lexer(string src) : input(src), pos(0), line(1), column(1) {}

    vector<Token> analyze() {
        while (pos < input.size()) {
            char c = input[pos];

            if (isspace(c)) {
                if (c == '\n') {
                    line++;
                    column = 1;
                }
                else {
                    column++;
                }
                pos++;
                continue;
            }

            if (isalpha(c) || c == '_') { // ��ʶ����ؼ���
                int startCol = column;
                string word = readWord();
                string upperWord = word;
                for (auto& ch : upperWord) ch = toupper(ch);

                if (keywords.count(upperWord))
                    tokens.push_back({ 1, upperWord, line, startCol });
                else
                    tokens.push_back({ 2, word, line, startCol });
            }
            else if (isdigit(c)) { // ����
                int startCol = column;
                string number = readNumber();
                tokens.push_back({ 3, number, line, startCol });
            }
            else if (operators.count(c)) { // �����
                int startCol = column;
                tokens.push_back({ 4, string(1, c), line, startCol });
                pos++;
                column++;
            }
            else if (delimiters.count(c)) { // ���
                int startCol = column;
                tokens.push_back({ 5, string(1, c), line, startCol });
                pos++;
                column++;
            }
            else {
                cerr << "�ʷ����󣺷Ƿ��ַ� '" << c
                    << "' �� ��" << line << "�� ��" << column << "��" << endl;
                pos++;
                column++;
            }
        }
        return tokens;
    }

private:
    string readWord() {
        string result;
        while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
            result += input[pos];
            pos++;
            column++;
        }
        return result;
    }

    // ��ʶ�𸡵���
    string readNumber() {
        string result;
        bool hasDot = false;

        while (pos < input.size()) {
            char c = input[pos];
            if (isdigit(c)) {
                result += c;
                pos++;
                column++;
            }
            else if (c == '.' && !hasDot) {
                // �������һ��С����
                hasDot = true;
                result += c;
                pos++;
                column++;
            }
            else {
                break;
            }
        }
        return result;
    }

};

// ����������
int main() {
    string sql = R"( 
        SELECT name, age FROM students WHERE age > 18 AND gpa > 3.5;
    )";

    Lexer lexer(sql);
    vector<Token> tokens = lexer.analyze();

    cout << left << setw(10) << "�ֱ���"
        << setw(15) << "����"
        << setw(10) << "�к�"
        << setw(10) << "�к�" << endl;
    cout << "--------------------------------------------" << endl;

    for (auto& t : tokens) {
        cout << left << setw(10) << t.type
            << setw(15) << t.value
            << setw(10) << t.line
            << setw(10) << t.column << endl;
    }

    return 0;
}
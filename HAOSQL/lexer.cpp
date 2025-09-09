#include <iostream>
#include <vector>
#include <unordered_set>
#include <cctype>
#include <iomanip>
#include "dataType.h"
using namespace std;

// 词法分析器
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

            if (isalpha(c) || c == '_') { // 标识符或关键字
                int startCol = column;
                string word = readWord();
                string upperWord = word;
                for (auto& ch : upperWord) ch = toupper(ch);

                if (keywords.count(upperWord))
                    tokens.push_back({ 1, upperWord, line, startCol });
                else
                    tokens.push_back({ 2, word, line, startCol });
            }
            else if (isdigit(c)) { // 常数
                int startCol = column;
                string number = readNumber();
                tokens.push_back({ 3, number, line, startCol });
            }
            else if (operators.count(c)) { // 运算符
                int startCol = column;
                tokens.push_back({ 4, string(1, c), line, startCol });
                pos++;
                column++;
            }
            else if (delimiters.count(c)) { // 界符
                int startCol = column;
                tokens.push_back({ 5, string(1, c), line, startCol });
                pos++;
                column++;
            }
            else {
                cerr << "词法错误：非法字符 '" << c
                    << "' 在 第" << line << "行 第" << column << "列" << endl;
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

    // 可识别浮点数
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
                // 允许出现一次小数点
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

// 测试主函数
int main() {
    string sql = R"( 
        SELECT name, age FROM students WHERE age > 18 AND gpa > 3.5;
    )";

    Lexer lexer(sql);
    vector<Token> tokens = lexer.analyze();

    cout << left << setw(10) << "种别码"
        << setw(15) << "单词"
        << setw(10) << "行号"
        << setw(10) << "列号" << endl;
    cout << "--------------------------------------------" << endl;

    for (auto& t : tokens) {
        cout << left << setw(10) << t.type
            << setw(15) << t.value
            << setw(10) << t.line
            << setw(10) << t.column << endl;
    }

    return 0;
}
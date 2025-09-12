#include "lexer.h"

using namespace std;

// ���캯��ʵ��
Lexer::Lexer(string src) : input(src), pos(0), line(1), column(1) {
    keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES",
        "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "DROP",
        "ALTER", "ADD", "PRIMARY", "KEY", "NOT", "NULL",
        "AND", "OR", "AS", "INT", "VARCHAR", "CHAR", "FLOAT", "DOUBLE","UNIQUE","IF","EXISTS"
    };

    operators = { '+', '-', '*', '/', '%', '=', '<', '>' };
    delimiters = { ',', ';', '(', ')', '.' };
}

// analyze ʵ��
vector<Token> Lexer::analyze() {
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
        else if (c == '\'' || c == '"') {
            int startCol = column;
            string str = readString();
            tokens.push_back({ 6, str, line, startCol }); // 6 = �ַ�������
        }
        else if (isdigit(c)) { // ����
            int startCol = column;
            string number = readNumber();
            tokens.push_back({ 3, number, line, startCol });
        }
        else if (operators.count(c)) { // �����
            int startCol = column;

            string op(1, c);
            // �����һ���ַ�
            if (pos + 1 < input.size()) {
                char next = input[pos + 1];
                string twoCharOp = op + next;

                // ֧�ֳ�����˫�ַ������
                if (twoCharOp == ">=" || twoCharOp == "<=" ||
                    twoCharOp == "<>" || twoCharOp == "!=" ||
                    twoCharOp == "==") {
                    tokens.push_back({ 4, twoCharOp, line, startCol });
                    pos += 2;
                    column += 2;
                    continue;
                }
            }

            // ������ǵ��ַ������
            tokens.push_back({ 4, op, line, startCol });
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

// readWord ʵ��
string Lexer::readWord() {
    string result;
    while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
        result += input[pos];
        pos++;
        column++;
    }
    return result;
}

// readNumber ʵ�֣���ʶ�𸡵�����
string Lexer::readNumber() {
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

string Lexer::readString() {
    string result;
    char quote = input[pos]; // ' �� "
    int startCol = column;
    pos++;
    column++;
    while (pos < input.size() && input[pos] != quote) {
        result += input[pos];
        pos++;
        column++;
    }
    if (pos < input.size() && input[pos] == quote) {
        pos++;
        column++;
    }
    else {
        cerr << "�ʷ������ַ���δ�պ� �ڵ�" << line << "�� ��" << startCol << "��" << endl;
    }
    return result;
}

// ����������

/*int main() {
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
}*/
